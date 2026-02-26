/***********************************************************************
VRServerLauncher - A small daemon to launch and monitor the servers
needed to operate a VR environment with a head-mounted display, using
VRDeviceDaemon for tracking and VRCompositingServer for rendering.
Copyright (c) 2025-2026 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <utility>
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <Misc/Autopointer.h>
#include <Misc/StdError.h>
#include <Misc/PrintInteger.h>
#include <Misc/FileTests.h>
#include <Misc/CommandLineParser.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Threads/FunctionCalls.h>
#include <Threads/RunLoop.h>
#include <DBus/Connection.h>
#include <IO/OStream.h>
#include <IO/JsonEntityTypes.h>
#include <Comm/UNIXPipe.h>
#include <Comm/TCPPipe.h>
#include <Comm/ListeningTCPSocket.h>
#include <Comm/HttpPostRequest.h>
#include <Vrui/Internal/Config.h>

namespace {

/****************
Helper functions:
****************/

void redirectIO(const std::string& logFileName,bool closeFds)
	{
	/* Redirect stdin to /dev/null and stdout and stderr to log file: */
	int nullFd=open("/dev/null",O_RDONLY);
	if(nullFd<0)
		throw Misc::makeLibcErr(0,errno,"Cannot open /dev/null");
	int logFd=open(logFileName.c_str(),O_RDWR|O_CREAT|O_TRUNC,S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH);
	if(logFd<0)
		throw Misc::makeLibcErr(0,errno,"Cannot open log file %s",logFileName.c_str());
	dup2(nullFd,0); // Redirect stdin to /dev/null
	close(nullFd);
	dup2(logFd,1); // Redirect stdout to the log file
	dup2(logFd,2); // Redirect stderr to the log file
	close(logFd);
	
	int fdTableSize=int(sysconf(_SC_OPEN_MAX));
	if(closeFds)
		{
		/* Close all other file descriptors: */
		for(int fd=3;fd<fdTableSize;++fd)
			{
			/* This will fail silently for ones that aren't actually open: */
			close(fd);
			}
		}
	else
		{
		/* Set the CLOEXEC flag on all other file descriptors to be shut down when we execute the server: */
		for(int fd=3;fd<fdTableSize;++fd)
			{
			/* This will fail silently for ones that aren't actually open: */
			fcntl(fd,F_SETFD,FD_CLOEXEC);
			}
		}
	}

}

class VRServerLauncher
	{
	/* Embedded classes: */
	private:
	struct Server // Structure keeping track of a launched server
		{
		/* Elements: */
		public:
		std::string name; // Short server name, used for pid and log files
		std::string displayName; // Display name for the server
		std::string executableName; // Path to the server's executable
		std::vector<std::string> arguments; // Arguments to be passed on the server's command line
		pid_t pid; // Process ID for a running server, or 0
		std::string pidFileName; // The name of the server's PID file
		std::string logFileName; // The name of the server's log file
		std::string socketName; // Name of the server's UNIX domain socket
		bool socketAbstract; // Flag whether the server's UNIX domain socket is in the abstract namespace
		int httpPort; // Port number on which the server listens for HTTP requests
		std::vector<std::string> cleanupFiles; // Files that have to be removed explicitly when a server shits the bed
		};
	
	struct Environment // Structure to store named spatial environments that can be loaded into VRDeviceDaemon at run-time
		{
		/* Elements: */
		public:
		std::string name; // The spatial environment file's descriptive name
		std::string path; // The spatial environment file's absolute path
		
		/* Constructors and destructors: */
		Environment(const std::string& sName,const std::string& sPath)
			:name(sName),path(sPath)
			{
			}
		};
	
	/* Elements: */
	private:
	Threads::RunLoop& runLoop; // Reference to the main thread's run loop
	DBus::Connection systemBus; // A connection to the system DBus to track active sessions and manage sleep inhibition locks
	std::string seatPath; // The DBus path for the seat to which this launcher server is attached
	std::string activeSessionPath; // The DBus path for the currently active session
	std::string activeDisplay; // The X11 display string for the display attached with the current session
	int sleepInhibitorFd; // A UNIX file descriptor inhibiting the system from going to sleep
	Misc::Autopointer<Comm::ListeningTCPSocket> httpListenSocket; // Socket listening for incoming HTTP connections
	Threads::RunLoop::IOWatcherOwner httpListenSocketWatcher; // I/O watcher for the HTTP listening socket
	Threads::RunLoop::SignalHandlerOwner sigChldHandler; // Signal handler for SIGCHLD signals
	Server servers[2]; // Array of server tracking structures
	std::vector<Environment> environments; // A list of pre-defined spatial environments that can be loaded into VRDeviceDaemon at run-time
	
	/* Private methods: */
	bool collectServer(Server& server,bool noWait);
	bool startServer(Server& server);
	void sendServerStatus(IO::JsonObject& replyRoot);
	void sendEnvironments(IO::JsonObject& replyRoot);
	void stopServers(void);
	void newConnectionCallback(Threads::RunLoop::IOWatcher::Event& event); // Callback called when a new connection is available on the HTTP listening socket
	void childTerminatedCallback(Threads::RunLoop::SignalHandler::Event& event); // Callback called when a child process terminates
	
	/* DBus method calls and message handlers: */
	void requestSleepInhibitorReply(DBus::Message& message);
	void requestSleepInhibitor(void);
	void querySessionDisplayReplyHandler(DBus::Message& message);
	void queryActiveSessionReplyHandler(DBus::Message& message);
	void querySeatPathReplyHandler(DBus::Message& message);
	void querySeatIdReplyHandler(DBus::Message& message);
	void systemBusSignalHandler(DBus::Message& message);
	
	/* Constructors and destructors: */
	public:
	VRServerLauncher(Threads::RunLoop& sRunLoop,int httpPort,const std::string& homeDir,const std::string& defaultPidFileDir,const std::string& defaultLogFileDir);
	~VRServerLauncher(void);
	};

bool VRServerLauncher::collectServer(VRServerLauncher::Server& server,bool noWait)
	{
	bool result=false;
	
	/* Collect the server's exit status: */
	pid_t termPid(0);
	int waitStatus(0);
	if(noWait)
		{
		/* Collect the server's exit status immediately, in response to receiving a SIGCHLD signal: */
		termPid=waitpid(server.pid,&waitStatus,WNOHANG);
		}
	else
		{
		/* Wait for the server to terminate after a kill request, but don't wait for too long: */
		int numTries=10;
		while(numTries>0&&(termPid=waitpid(server.pid,&waitStatus,WNOHANG))==0)
			{
			/* Wait for a bit, then try again: */
			usleep(100000);
			--numTries;
			}
		}
	
	if(termPid==server.pid)
		{
		/* Remove the server's pid file: */
		unlink(server.pidFileName.c_str());
		
		/* Print a friendly status message: */
		if(WIFEXITED(waitStatus))
			std::cout<<"VRServerLauncher::collectServer: "<<server.displayName<<" shut down cleanly with exit status "<<WEXITSTATUS(waitStatus)<<std::endl;
		else if(WIFSIGNALED(waitStatus))
			{
			std::cerr<<"VRServerLauncher::collectServer: "<<server.displayName<<" shat the bed with signal "<<WTERMSIG(waitStatus)<<(WCOREDUMP(waitStatus)?" and dumped core":" but did not dump core")<<std::endl;
			
			/* Remove files that the server may have left behind: */
			for(std::vector<std::string>::iterator cfIt=server.cleanupFiles.begin();cfIt!=server.cleanupFiles.end();++cfIt)
				{
				if(unlink(cfIt->c_str())==0)
					std::cout<<"VRServerLauncher::collectServer: Removed dangling file "<<*cfIt<<std::endl;
				else if(errno!=ENOENT)
					std::cerr<<Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,errno,"Cannot remove dangling file %s",cfIt->c_str())<<"; manual clean-up required"<<std::endl;
				}
			}
		
		/* Mark the sub-process as terminated: */
		server.pid=pid_t(0);
		result=true;
		}
	else if(termPid==pid_t(-1))
		{
		/* Print an error message and continue: */
		std::cerr<<Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,errno,"Cannot collect %s",server.displayName.c_str())<<std::endl;
		}
	
	return result;
	}

bool VRServerLauncher::startServer(VRServerLauncher::Server& server)
	{
	bool result=false;
	
	std::cout<<"VRServerLauncher::startServer: Starting "<<server.displayName<<std::endl;
	
	/* Fork: */
	pid_t childPid=fork();
	if(childPid==pid_t(0))
		{
		try
			{
			/* Redirect stdin to /dev/null and stdout and stderr to the appropriate log file and mark all other file descriptors to be closed on execv: */
			redirectIO(server.logFileName,false);
			}
		catch(const std::runtime_error& err)
			{
			/* Print an error message to what still is the original stderr, then kill this process and let the parent handle it: */
			std::cerr<<"VRServerLauncher::startServer: Cannot redirect I/O for "<<server.displayName<<" due to "<<err.what()<<std::endl;
			exit(EXIT_FAILURE);
			}
		
		/* Construct the server executable's command line: */
		char* argv[20];
		int argc=0;
		argv[argc++]=const_cast<char*>(server.executableName.c_str());
		for(std::vector<std::string>::iterator argIt=server.arguments.begin();argIt!=server.arguments.end();++argIt)
			argv[argc++]=const_cast<char*>(argIt->c_str());
		argv[argc]=0;
		
		/* Construct the server executable's environment: */
		char* envp[20];
		int envc=0;
		std::string display="DISPLAY=";
		display.append(activeDisplay);
		envp[envc++]=const_cast<char*>(display.c_str());
		envp[envc]=0;
		
		/* Run the server executable: */
		if(execve(argv[0],argv,envp)<0)
			{
			/* Print an error message to what is now the server's log file, then kill this process and let the parent handle it: */
			std::cerr<<Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,errno,"Cannot execute %s for %s",server.executableName.c_str(),server.displayName.c_str())<<std::endl;
			exit(EXIT_FAILURE);
			}
		}
	else if(childPid==pid_t(-1))
		{
		/* Print an error message and carry on: */
		std::cerr<<Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,errno,"Cannot fork for %s",server.displayName.c_str())<<std::endl;
		}
	else
		{
		/* Remember the child's process ID: */
		server.pid=childPid;
		
		/* Save daemon's process ID to its pid file: */
		int pidFd=open(server.pidFileName.c_str(),O_RDWR|O_CREAT|O_TRUNC,S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH);
		if(pidFd>=0)
			{
			/* Write process ID: */
			char pidBuffer[20];
			snprintf(pidBuffer,sizeof(pidBuffer),"%d\n",server.pid);
			size_t pidLen=strlen(pidBuffer);
			if(write(pidFd,pidBuffer,pidLen)!=ssize_t(pidLen))
				std::cerr<<Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,errno,"Cannot write %s's PID to file %s",server.displayName.c_str(),server.pidFileName.c_str())<<std::endl;
			close(pidFd);
			}
		else
			std::cerr<<Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,errno,"Cannot create %s's PID file %s",server.displayName.c_str(),server.pidFileName.c_str())<<std::endl;
		
		/* Try connecting to the just-started server's HTTP command socket until it succeeds or times out: */
		unsigned int attempts=10;
		while(attempts>0)
			{
			try
				{
				/* Sleep a bit: */
				usleep(250000);
				
				/* Open a connection to the server's HTTP port: */
				Comm::TCPPipe httpPipe("localhost",server.httpPort);
				httpPipe.ref();
				
				/* Send a malformed HTTP request: */
				IO::OStream request(&httpPipe);
				request<<"POST /Foo.cgi HTTP/1.1\n";
				request<<"Host: localhost:"<<server.httpPort<<"\n";
				request<<"Content-Type: application/x-www-form-urlencoded\n";
				request<<"Content-Length: 0\n";
				request<<"\n";
				request<<std::endl;
				
				httpPipe.flush();
				
				/* Read the HTTP reply: */
				void* buffer;
				while(!httpPipe.eof())
					httpPipe.readInBuffer(buffer);
				
				/* Bail out 'cuz it worked: */
				break;
				}
			catch(const std::runtime_error& err)
				{
				/* Ignore the error and try again: */
				--attempts;
				}
			}
		
		/* Check if the connection worked: */
		if(attempts>0)
			{
			std::cout<<"VRServerLauncher::startServer: "<<server.displayName<<" started successfully on PID "<<childPid<<std::endl;
			result=true;
			}
		else
			{
			/* Kill the server brutally, because something serious went wrong: */
			std::cerr<<"VRServerLauncher::startServer: Cannot establish connection to "<<server.displayName<<std::endl;
			kill(server.pid,SIGKILL);
			collectServer(server,false);
			}
		}
	
	return result;
	}

void VRServerLauncher::sendServerStatus(IO::JsonObject& replyRoot)
	{
	/* Add an array with server running flags and PIDs to the reply structure: */
	IO::JsonArrayPointer serverStates=new IO::JsonArray;
	replyRoot.setProperty("servers",*serverStates);
	
	for(int serverIndex=0;serverIndex<2;++serverIndex)
		{
		Server& server=servers[serverIndex];
		
		/* Add an entry for this server's state to the reply structure: */
		IO::JsonObjectPointer serverState=new IO::JsonObject;
		serverStates->addItem(*serverState);
		serverState->setProperty("name",server.displayName);
		serverState->setProperty("isRunning",server.pid!=pid_t(0));
		if(server.pid!=pid_t(0))
			{
			serverState->setProperty("pid",int(server.pid));
			serverState->setProperty("logFileName",server.logFileName);
			serverState->setProperty("httpPort",server.httpPort);
			}
		}
	}

void VRServerLauncher::sendEnvironments(IO::JsonObject& replyRoot)
	{
	/* Add an array with spatial environment names and files to the reply structure: */
	IO::JsonArrayPointer evs=new IO::JsonArray;
	replyRoot.setProperty("environments",*evs);
	
	for(std::vector<Environment>::iterator eIt=environments.begin();eIt!=environments.end();++eIt)
		{
		/* Add an entry for this environment to the reply structure: */
		IO::JsonObjectPointer ev=new IO::JsonObject;
		evs->addItem(*ev);
		ev->setProperty("name",eIt->name);
		ev->setProperty("path",eIt->path);
		}
	}

void VRServerLauncher::stopServers(void)
	{
	/* Stop the two server sub-processes: */
	bool waitABit=false;
	for(int serverIndex=1;serverIndex>=0;--serverIndex)
		{
		Server& server=servers[serverIndex];
		
		if(server.pid!=pid_t(0))
			{
			std::cout<<"VRServerLauncher::stopServers: Stopping "<<server.displayName<<std::endl;
			if(waitABit)
				usleep(250000);
			kill(server.pid,SIGTERM);
			if(!collectServer(server,false))
				{
				/* The server did not shut down cleanly; kill it: */
				std::cerr<<"VRServerLauncher::stopServers: "<<server.displayName<<" did not shut down; killing process"<<std::endl;
				kill(server.pid,SIGKILL);
				collectServer(server,false);
				}
			
			/* Wait a bit between shutting down servers: */
			waitABit=true;
			}
		}
	}

void VRServerLauncher::newConnectionCallback(Threads::RunLoop::IOWatcher::Event& event)
	{
	try
		{
		/* Don't print this; it seems the browser sends many invalid requests for every valid one: */
		// std::cout<<"VRServerLauncher: Handling new HTTP request"<<std::endl;
		
		/* Accept the next pending connection: */
		Comm::TCPPipe pipe(*httpListenSocket);
		pipe.ref();
		
		/* Parse an incoming HTTP POST request: */
		Comm::HttpPostRequest request(pipe);
		const Comm::HttpPostRequest::NameValueList& nvl=request.getNameValueList();
		
		/* Check that there is exactly one command in the POST request: */
		if(request.getActionUrl()=="/VRServerLauncher.cgi"&&nvl.size()==1&&nvl.front().name=="command")
			{
			/* Compose the server's reply as a JSON-encoded object: */
			IO::JsonObjectPointer replyRoot=new IO::JsonObject;
			replyRoot->setProperty("command",nvl.front().value);
			
			/* Process the command: */
			if(nvl.front().value=="isAlive")
				{
				/* Just send a flag: */
				replyRoot->setProperty("isRunning",true);
				
				replyRoot->setProperty("status","Success");
				}
			else if(nvl.front().value=="startServers")
				{
				/* Only start the servers if there is an active X11 display: */
				bool success=!activeDisplay.empty();
				
				/* Start the two server sub-processes: */
				for(int serverIndex=0;serverIndex<2&&success;++serverIndex)
					{
					/* Start the server if it isn't already running: */
					if(servers[serverIndex].pid==pid_t(0))
						success=startServer(servers[serverIndex]);
					}
				
				/* Send the resulting server status: */
				sendServerStatus(*replyRoot);
				
				replyRoot->setProperty("status",success?"Success":"Failed");
				}
			else if(nvl.front().value=="stopServers")
				{
				/* Shut down the server sub-processes: */
				stopServers();
				
				replyRoot->setProperty("status","Success");
				}
			else if(nvl.front().value=="getServerStatus")
				{
				/* Send the current server status: */
				sendServerStatus(*replyRoot);
				
				replyRoot->setProperty("status","Success");
				}
			else if(nvl.front().value=="getEnvironments")
				{
				/* Send the list of loadable named spatial environments: */
				sendEnvironments(*replyRoot);
				
				replyRoot->setProperty("status","Success");
				}
			else
				replyRoot->setProperty("status","Invalid command");
			
			/* Send the server's reply as a json file embedded in an HTTP reply: */
			IO::OStream reply(&pipe);
			reply<<"HTTP/1.1 200 OK\n";
			reply<<"Content-Type: application/json\n";
			reply<<"Access-Control-Allow-Origin: *\n";
			reply<<"\n";
			reply<<*replyRoot<<std::endl;
			}
		else
			{
			/* Send an HTTP error code: */
			IO::OStream reply(&pipe);
			reply<<"HTTP/1.1 400 Bad Request\n";
			reply<<"\n";
			reply<<std::endl;
			}
		
		/* Send the reply: */
		pipe.flush();
		}
	catch(const std::runtime_error& err)
		{
		/* Don't print this; it seems the browser sends many invalid requests for every valid one: */
		// std::cout<<"VRServerLauncher: Ignoring HTTP request due to exception "<<err.what()<<std::endl;
		}
	}

void VRServerLauncher::childTerminatedCallback(Threads::RunLoop::SignalHandler::Event& event)
	{
	/* Reap any terminated child processes: */
	for(int serverIndex=0;serverIndex<2;++serverIndex)
		{
		if(servers[serverIndex].pid!=pid_t(0))
			collectServer(servers[serverIndex],true);
		}
	}

void VRServerLauncher::requestSleepInhibitorReply(DBus::Message& message)
	{
	/* Parse the reply: */
	DBus::Message::ReadIterator it=message.getReadIterator();
	
	/* Extract the sleep inhibitor's file descriptor: */
	sleepInhibitorFd=it.readUnixFd();
	}

void VRServerLauncher::requestSleepInhibitor(void)
	{
	/* Acquire an inhibitor lock by sending a message to the logind service: */
	DBus::Message request=DBus::Message::createMethodCall("org.freedesktop.login1","/org/freedesktop/login1","org.freedesktop.login1.Manager","Inhibit");
	request.appendString("shutdown:sleep");
	request.appendString("VRServerLauncher");
	request.appendString("Shut down VR devices");
	request.appendString("delay");
	systemBus.sendWithReply(request,-1,*Threads::createFunctionCall(this,&VRServerLauncher::requestSleepInhibitorReply));
	}

void VRServerLauncher::querySessionDisplayReplyHandler(DBus::Message& message)
	{
	/* Parse the reply: */
	DBus::Message::ReadIterator it=message.getReadIterator();
	
	/* Recurse into the root variant: */
	DBus::Message::ReadIterator vIt=it.recurse();
	
	/* Read the X11 display string: */
	activeDisplay=vIt.readString();
	
	if(!activeDisplay.empty())
		std::cout<<"VRServerLauncher: Active session now "<<activeSessionPath<<" with X11 display "<<activeDisplay<<std::endl;
	else
		std::cout<<"VRServerLauncher: Active session now "<<activeSessionPath<<" without an X11 display "<<std::endl;
	}

void VRServerLauncher::queryActiveSessionReplyHandler(DBus::Message& message)
	{
	/* Parse the reply: */
	DBus::Message::ReadIterator it=message.getReadIterator();
	
	/* Recurse into the root variant: */
	DBus::Message::ReadIterator vIt=it.recurse();
	
	/* Recurse into the property structure: */
	DBus::Message::ReadIterator rIt=vIt.recurse();
	
	/* Read the active session ID and path: */
	const char* sessionId=rIt.readString();
	++rIt;
	activeSessionPath=rIt.readObjectPath();
	
	if(activeSessionPath!="/")
		{
		/* Query the active session's X11 display string: */
		DBus::Message request=DBus::Message::createMethodCall("org.freedesktop.login1",activeSessionPath.c_str(),"org.freedesktop.DBus.Properties","Get");
		request.appendString("org.freedesktop.login1.Session");
		request.appendString("Display");
		systemBus.sendWithReply(request,-1,*Threads::createFunctionCall(this,&VRServerLauncher::querySessionDisplayReplyHandler));
		}
	else
		{
		/* No active session, no display: */
		activeDisplay.clear();
		
		std::cout<<"VRServerLauncher: No active session"<<std::endl;
		}
	}

void VRServerLauncher::querySeatPathReplyHandler(DBus::Message& message)
	{
	/* Parse the reply: */
	DBus::Message::ReadIterator it=message.getReadIterator();
	
	/* Read the seat's path: */
	seatPath=it.readObjectPath();
	std::cout<<"VRServerLauncher: Attached to seat "<<seatPath<<std::endl;
	
	/* Query the seat's active session: */
	DBus::Message request=DBus::Message::createMethodCall("org.freedesktop.login1",seatPath.c_str(),"org.freedesktop.DBus.Properties","Get");
	request.appendString("org.freedesktop.login1.Seat");
	request.appendString("ActiveSession");
	systemBus.sendWithReply(request,-1,*Threads::createFunctionCall(this,&VRServerLauncher::queryActiveSessionReplyHandler));
	}

void VRServerLauncher::querySeatIdReplyHandler(DBus::Message& message)
	{
	/* Parse the reply: */
	DBus::Message::ReadIterator it=message.getReadIterator();
	
	/* Recurse into the root variant: */
	DBus::Message::ReadIterator vIt=it.recurse();
	
	/* Read the seat's ID: */
	const char* seatId=vIt.readString();
	
	/* Query the seat's path: */
	DBus::Message request=DBus::Message::createMethodCall("org.freedesktop.login1","/org/freedesktop/login1","org.freedesktop.login1.Manager","GetSeat");
	request.appendString(seatId);
	systemBus.sendWithReply(request,-1,*Threads::createFunctionCall(this,&VRServerLauncher::querySeatPathReplyHandler));
	}

void VRServerLauncher::systemBusSignalHandler(DBus::Message& message)
	{
	/* Bail out if the message isn't a signal: */
	if(message.getType()!=DBus::Message::Signal)
		return;
	
	/* Check if the message is a sleep/wake-up notification: */
	if(message.hasInterface("org.freedesktop.login1.Manager")&&(message.hasMember("PrepareForSleep")||message.hasMember("PrepareForShutdown")))
		{
		/* Retrieve the sleep/shutdown flag: */
		DBus::Message::ReadIterator it=message.getReadIterator();
		if(it.read<bool>())
			{
			/* Check if we have a sleep inhibitor, which means that the servers are currently running: */
			if(sleepInhibitorFd>=0)
				{
				/* Shut down the server sub-processes: */
				std::cout<<"VRServerLauncher: Stopping servers because system is going to sleep/shutting down"<<std::endl;
				stopServers();
				
				/* Release the sleep inhibitor: */
				close(sleepInhibitorFd);
				sleepInhibitorFd=-1;
				}
			}
		else
			{
			/* We could request another sleep inhibitor right after waking up, but we can also wait until we actually start the servers */
			requestSleepInhibitor();
			}
		}
	
	/* Check if the message is an active session change notification: */
	if(message.hasPath(seatPath.c_str())&&message.hasInterface("org.freedesktop.DBus.Properties")&&message.hasMember("PropertiesChanged"))
		{
		/* Retrieve the interface name: */
		DBus::Message::ReadIterator it=message.getReadIterator();
		const char* interfaceName=it.readString();
		if(strcmp(interfaceName,"org.freedesktop.login1.Seat")==0)
			{
			++it;
			
			/* Retrieve the array of changed properties: */
			for(DBus::Message::ReadIterator aIt=it.recurse();aIt.valid();++aIt)
				{
				/* Read the dictionary entry: */
				DBus::Message::ReadIterator dIt=aIt.recurse();
				const char* propertyName=dIt.readString();
				++dIt;
				if(strcmp(propertyName,"ActiveSession")==0)
					{
					/* Read the entry value: */
					DBus::Message::ReadIterator vIt=dIt.recurse();
					DBus::Message::ReadIterator rIt=vIt.recurse();
					const char* sessionId=rIt.readString();
					++rIt;
					const char* sessionPath=rIt.readObjectPath();
					
					/* Check that the active session actually changed: */
					if(activeSessionPath!=sessionPath)
						{
						/* Shut down the server sub-processes: */
						std::cout<<"VRServerLauncher: Stopping servers because the active session changed"<<std::endl;
						stopServers();
						
						/* Activate the new session: */
						activeSessionPath=sessionPath;
						if(activeSessionPath!="/")
							{
							/* Query the active session's X11 display string: */
							DBus::Message request=DBus::Message::createMethodCall("org.freedesktop.login1",activeSessionPath.c_str(),"org.freedesktop.DBus.Properties","Get");
							request.appendString("org.freedesktop.login1.Session");
							request.appendString("Display");
							systemBus.sendWithReply(request,-1,*Threads::createFunctionCall(this,&VRServerLauncher::querySessionDisplayReplyHandler));
							}
						else
							{
							/* No active session, no display: */
							activeDisplay.clear();
							
							std::cout<<"VRServerLauncher: No active session"<<std::endl;
							}
						}
					}
				}
			}
		}
	}

VRServerLauncher::VRServerLauncher(Threads::RunLoop& sRunLoop,int httpPort,const std::string& homeDir,const std::string& defaultPidFileDir,const std::string& defaultLogFileDir)
	:runLoop(sRunLoop),
	 systemBus(DBUS_BUS_SYSTEM),
	 sleepInhibitorFd(-1)
	{
	/* Watch the system bus connection using the run loop: */
	systemBus.watchConnection(runLoop);
	
	/* Add a match rule to receive signals from the logind service: */
	systemBus.addMatchRule("type='signal',sender='org.freedesktop.login1'",false);
	systemBus.addFilter(*Threads::createFunctionCall(this,&VRServerLauncher::systemBusSignalHandler));
	
	/* Send a message to the system bus to query the ID our our seat, and its currently active session and display: */
	{
	#if 0 // Systemd v 255 no longer has /self seat
	DBus::Message request=DBus::Message::createMethodCall("org.freedesktop.login1","/org/freedesktop/login1/seat/self","org.freedesktop.DBus.Properties","Get");
	request.appendString("org.freedesktop.login1.Seat");
	request.appendString("Id");
	systemBus.sendWithReply(request,-1,*Threads::createFunctionCall(this,&VRServerLauncher::querySeatIdReplyHandler));
	#else
	/* Query the seat's path: */
	DBus::Message request=DBus::Message::createMethodCall("org.freedesktop.login1","/org/freedesktop/login1","org.freedesktop.login1.Manager","GetSeat");
	request.appendString("seat0");
	systemBus.sendWithReply(request,-1,*Threads::createFunctionCall(this,&VRServerLauncher::querySeatPathReplyHandler));
	#endif
	}
	
	/* Request a sleep inhibitor: */
	requestSleepInhibitor();
	
	/* Load the VRServerLauncher configuration file: */
	Misc::ConfigurationFile configFile(VRUI_INTERNAL_CONFIG_SYSCONFIGDIR "/VRServerLauncher" VRUI_INTERNAL_CONFIG_CONFIGFILESUFFIX);
	Misc::ConfigurationFileSection cfg=configFile.getSection("/VRServerLauncher");
	
	/* If the HTTP port has not been given on the command line, retrieve it from the configuration file: */
	if(httpPort<0)
		httpPort=cfg.retrieveValue<int>("./httpPort",8080);
	
	/* Override the PID and log file directories from the configuration file: */
	std::string pidFileDir=defaultPidFileDir;
	if(cfg.hasTag("./pidFileDir"))
		{
		/* Retrieve the string from the configuration file and replace a potential beginning "~" with the determined home directory: */
		pidFileDir=cfg.retrieveString("./pidFileDir");
		if(!pidFileDir.empty()&&pidFileDir[0]=='~')
			{
			pidFileDir.erase(pidFileDir.begin());
			pidFileDir.insert(pidFileDir.begin(),homeDir.begin(),homeDir.end());
			}
		}
	std::string logFileDir=defaultLogFileDir;
	if(cfg.hasTag("./logFileDir"))
		{
		/* Retrieve the string from the configuration file and replace a potential beginning "~" with the determined home directory: */
		logFileDir=cfg.retrieveString("./logFileDir");
		if(!logFileDir.empty()&&logFileDir[0]=='~')
			{
			logFileDir.erase(logFileDir.begin());
			logFileDir.insert(logFileDir.begin(),homeDir.begin(),homeDir.end());
			}
		}
	
	/*********************************************************************
	Initialize the server tracking structures:
	*********************************************************************/
	
	char httpPortBuffer[9];
	
	/* Server 0: VRDeviceDaemon: */
	servers[0].name="VRDeviceDaemon";
	servers[0].displayName="VR tracking driver";
	servers[0].executableName=VRUI_INTERNAL_CONFIG_EXECUTABLEDIR "/RunOpenVRTracker.sh";
	servers[0].pid=pid_t(0);
	servers[0].pidFileName=pidFileDir+"/"+servers[0].name+".pid";
	servers[0].logFileName=logFileDir+"/"+servers[0].name+".log";
	servers[0].socketName="VRDeviceDaemon.socket";
	servers[0].socketAbstract=true;
	servers[0].httpPort=cfg.retrieveValue<int>("./deviceDaemonHttpPort",httpPort+1);
	servers[0].arguments.push_back("--httpPort");
	servers[0].arguments.push_back(Misc::print(servers[0].httpPort,httpPortBuffer+8));
	servers[0].cleanupFiles.push_back("/dev/shm/VRDeviceManagerDeviceState.shmem");
	
	/* Server 1: VRCompositingServer: */
	servers[1].name="VRCompositingServer";
	servers[1].displayName="VR compositing server";
	servers[1].executableName=VRUI_INTERNAL_CONFIG_EXECUTABLEDIR "/RunVRCompositor.sh";
	servers[1].pid=pid_t(0);
	servers[1].pidFileName=pidFileDir+"/"+servers[1].name+".pid";
	servers[1].logFileName=logFileDir+"/"+servers[1].name+".log";
	servers[1].socketName="VRCompositingServer.socket";
	servers[1].socketAbstract=true;
	servers[1].httpPort=cfg.retrieveValue<int>("./compositingServerHttpPort",servers[0].httpPort+1);
	servers[1].arguments.push_back("--httpPort");
	servers[1].arguments.push_back(Misc::print(servers[1].httpPort,httpPortBuffer+8));
	servers[1].cleanupFiles.push_back("/dev/shm/VRCompositingServer.shmem");
	
	/* Retrieve the list of named spatial environment files: */
	std::string environmentFileDir=cfg.retrieveString("./environmentFileDir",VRUI_INTERNAL_CONFIG_SYSCONFIGDIR);
	typedef std::vector<std::string> StringList;
	typedef std::vector<StringList> StringListList;
	StringListList environmentFiles;
	cfg.updateValue("./environmentFiles",environmentFiles);
	for(StringListList::iterator efIt=environmentFiles.begin();efIt!=environmentFiles.end();++efIt)
		{
		/* Check that the inner array has two elements: */
		if(efIt->size()==2)
			{
			/* Root relative paths in the environment file directory: */
			std::string path=(*efIt)[1];
			if(path.empty()||path[0]!='/')
				path=environmentFileDir+"/"+path;
			
			/* Check that the file actually exists: */
			if(Misc::isFileReadable(path.c_str()))
				{
				/* Store the environment file: */
				std::cout<<"VRServerLauncher: Offering spatial environment "<<(*efIt)[0]<<" from file "<<path<<std::endl;
				environments.push_back(Environment((*efIt)[0],path));
				}
			else
				{
				/* Print an error message and ignore the environment file: */
				std::cerr<<"VRServerLauncher: Ignoring spatial environment "<<(*efIt)[0]<<" because file "<<path<<" is not readable"<<std::endl;
				}
			}
		else
			std::cerr<<"VRServerLauncher: Format error in spatial environment configuration"<<std::endl;
		}
	
	/* Open the HTTP listening socket: */
	httpListenSocket=new Comm::ListeningTCPSocket(httpPort,5);
	
	/* Register an I/O watcher for the HTTP listening socket: */
	httpListenSocketWatcher=runLoop.createIOWatcher(httpListenSocket->getFd(),Threads::RunLoop::IOWatcher::Read,true,*Threads::createFunctionCall(this,&VRServerLauncher::newConnectionCallback));
	
	std::cout<<"VRServerLauncher: Servicing HTTP POST requests on TCP port "<<httpPort<<std::endl;
	
	/* Install a handler for SIGCHLD to receive a notification when one of the sub-processes dies: */
	sigChldHandler=runLoop.createSignalHandler(SIGCHLD,true,*Threads::createFunctionCall(this,&VRServerLauncher::childTerminatedCallback));
	}

VRServerLauncher::~VRServerLauncher(void)
	{
	/* Stop the servers in case they are still running: */
	stopServers();
	
	/* Release any sleep inhibitors: */
	if(sleepInhibitorFd>=0)
		close(sleepInhibitorFd);
	}

void sigHandlerFunction(Threads::RunLoop::SignalHandler::Event& event,bool& flag)
	{
	/* Mark that we received the signal: */
	flag=false;
	
	/* Stop the run loop: */
	event.getRunLoop().stop();
	}

int main(int argc,char* argv[])
	{
	/* Parse the command line: */
	Misc::CommandLineParser cmdLine;
	cmdLine.setDescription("Server to start and monitor VRDeviceDaemon (Vrui VR tracking driver) and VRCompositingServer (Vrui HMD display driver) servers.");
	int httpPort=-1; // Don't force an HTTP port unless asked
	cmdLine.addValueOption("httpPort","p",httpPort,"<TCP port number>","Number of TCP port on which to listen for HTTP POST requests.");
	bool daemonize=false;
	cmdLine.addEnableOption("daemonize","D",daemonize,"Turn the server into a daemon after start-up.");
	try
		{
		cmdLine.parse(argv,argv+argc);
		}
	catch(const std::runtime_error& err)
		{
		std::cerr<<"VRServerLauncher: "<<err.what()<<std::endl;
		return 1;
		}
	if(cmdLine.hadHelp())
		return 0;
	
	/* Determine the directories where to write pid and log files: */
	char* homeDirString=getenv("HOME");
	std::string homeDir=homeDirString!=0?homeDirString:"/tmp"; // Fall back to "/tmp" if no home directory is set
	std::string pidFileDir,logFileDir;
	if(geteuid()==uid_t(0))
		{
		/* Store the files in the appropriate system directories if the server is run as root: */
		pidFileDir="/var/run";
		logFileDir="/var/log";
		}
	else
		{
		/* Store the files in the user's home directory, or in /tmp if that fails: */
		pidFileDir=homeDir;
		logFileDir=homeDir;
		}
	
	/* Turn the server into a daemon if requested: */
	std::string pidFileName;
	if(daemonize)
		{
		/* Fork once (and exit) to notify shell or caller that the program is done: */
		int childPid=fork();
		if(childPid<0)
			{
			std::cerr<<Misc::makeLibcErrMsg("VRServerLauncher",errno,"Cannot fork daemon")<<std::endl;
			return 1; // Fork error
			}
		else if(childPid>0)
			{
			/* Print the daemon's process ID: */
			std::cout<<"VRServerLauncher: Started daemon with PID "<<childPid<<std::endl;
			
			/* Save daemon's process ID to pid file: */
			pidFileName=pidFileDir+"/VRServerLauncher.pid";
			int pidFd=open(pidFileName.c_str(),O_RDWR|O_CREAT|O_TRUNC,S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH);
			if(pidFd>=0)
				{
				/* Write process ID: */
				char pidBuffer[20];
				snprintf(pidBuffer,sizeof(pidBuffer),"%d\n",childPid);
				size_t pidLen=strlen(pidBuffer);
				if(write(pidFd,pidBuffer,pidLen)!=ssize_t(pidLen))
					std::cerr<<Misc::makeLibcErrMsg("VRServerLauncher",errno,"Cannot write daemon PID to file %s",pidFileName.c_str())<<std::endl;
				close(pidFd);
				}
			else
				std::cerr<<Misc::makeLibcErrMsg("VRServerLauncher",errno,"Cannot create daemon PID file %s",pidFileName.c_str())<<std::endl;
			
			return 0; // Parent process exits
			}
		
		/* Set new session ID to become independent process without controlling tty: */
		setsid();
		
		try
			{
			/* Redirect I/O and close all open file descriptors: */
			redirectIO(logFileDir+"/VRServerLauncher.log",true);
			}
		catch(const std::runtime_error& err)
			{
			/* Write an error message to the original stderr and quit: */
			std::cerr<<"VRServerLauncher: Cannot redirect I/O for daemon due to "<<err.what()<<std::endl;
			return EXIT_FAILURE;
			}
		}
	
	/* Ignore SIGPIPE and leave handling of pipe errors to TCP sockets: */
	Comm::ignorePipeSignals();
	
	/* Create a run loop to dispatch events: */
	Threads::RunLoop runLoop;
	
	/* Install a handler for SIGHUP that restarts the server launcher (and reloads its configuration file), but keeps running: */
	bool dummy=false; // A dummy flag for the SIGHUP handler
	Threads::RunLoop::SignalHandlerPtr sigHupHandler=runLoop.createSignalHandler(SIGHUP,true,*Threads::createFunctionCall(sigHandlerFunction,dummy));
	
	/* Install handlers for SIGINT and SIGTERM that shut down the daemon: */
	bool keepRunning=true;
	Misc::Autopointer<Threads::RunLoop::SignalHandler::EventHandler> intTermHandler=Threads::createFunctionCall(sigHandlerFunction,keepRunning);
	Threads::RunLoop::SignalHandlerPtr sigIntHandler=runLoop.createSignalHandler(SIGINT,true,*intTermHandler);
	Threads::RunLoop::SignalHandlerPtr sigTermHandler=runLoop.createSignalHandler(SIGTERM,true,*intTermHandler);
	
	int exitCode=0;
	
	try
		{
		/* Run until shut down: */
		while(keepRunning)
			{
			/* Create a server launcher: */
			std::cout<<"VRServerLauncher: Creating server launcher object"<<std::endl;
			VRServerLauncher* serverLauncher=new VRServerLauncher(runLoop,httpPort,homeDir,pidFileDir,logFileDir);
			
			/* Handle events until shut down: */
			runLoop.run();
			
			/* Destroy the server launcher: */
			std::cout<<"VRServerLauncher: Destroying server launcher object"<<std::endl;
			delete serverLauncher;
			
			/* If we're about to restart, wait for a bit to let the server launcher's HTTP socket close down: */
			if(keepRunning)
				usleep(1000000);
			}
		}
	catch(const std::runtime_error& err)
		{
		std::cerr<<"VRServerLauncher: Shutting down with exception "<<err.what()<<std::endl;
		exitCode=EXIT_FAILURE;
		}
	
	/* If the server was daemonized, remove the pid file: */
	if(daemonize)
		unlink(pidFileName.c_str());
	
	return exitCode;
	}
