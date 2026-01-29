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
#include <Misc/StdError.h>
#include <Threads/EventDispatcher.h>
#include <IO/ValueSource.h>
#include <IO/OStream.h>
#include <IO/JsonEntityTypes.h>
#include <Comm/UNIXPipe.h>
#include <Comm/TCPPipe.h>
#include <Comm/ListeningTCPSocket.h>
#include <Comm/HttpPostRequest.h>
#include <Vrui/Internal/Config.h>

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
		pid_t pid; // Process ID for a running server, or 0
		std::string socketName; // Name of the server's UNIX domain socket
		bool socketAbstract; // Flag whether the server's UNIX domain socket is in the abstract namespace
		int httpPort; // Port number on which the server listens for HTTP requests
		};
	
	/* Elements: */
	private:
	Threads::EventDispatcher eventDispatcher; // Dispatcher to handle I/O events
	Comm::ListeningTCPSocket listenSocket; // Socket listening for incoming TCP connections
	Threads::EventDispatcher::ListenerKey listenSocketKey;
	static VRServerLauncher* sigThis; // Pointer to active server launcher object for signal processing
	Threads::EventDispatcher::ListenerKey sigChldKey; // Signal to notify the front-end that any of the sub-processes has terminated
	std::string pidFileDir,logFileDir; // Directories to store the servers' pid and log files
	Server servers[2]; // Array of server tracking structures
	
	/* Private methods: */
	static void signalHandler(int sig); // Signal handler for SIGCHLD
	bool startServer(Server& server,Comm::TCPPipe& pipe);
	void collectServer(Server& server,bool noWait);
	static void newConnectionCallback(Threads::EventDispatcher::IOEvent& event);
	static void childTerminatedCallback(Threads::EventDispatcher::SignalEvent& event);
	
	/* Constructors and destructors: */
	public:
	VRServerLauncher(int listenPortId);
	~VRServerLauncher(void);
	
	/* Methods: */
	void run(void);
	};

/***************************************
Static elements of class VRServerLauncher:
***************************************/

VRServerLauncher* VRServerLauncher::sigThis(0);

/*******************************
Methods of class VRServerLauncher:
*******************************/

void VRServerLauncher::signalHandler(int sig)
	{
	if(sigThis!=0&&sig==SIGCHLD)
		{
		/* Notify the server launcher that a child terminated: */
		sigThis->eventDispatcher.signal(sigThis->sigChldKey,0);
		}
	}

bool VRServerLauncher::startServer(VRServerLauncher::Server& server,Comm::TCPPipe& pipe)
	{
	bool result=false;
	
	std::cout<<"VRServerLauncher::startServer: Starting "<<server.displayName<<std::endl;
	
	/* Fork: */
	pid_t childPid=fork();
	if(childPid==pid_t(0))
		{
		/* Close the pipe in the child process: */
		pipe.~TCPPipe();
		
		/* Close all inherited file descriptors: */
		for(int i=getdtablesize()-1;i>=0;--i)
			close(i);
		
		/* Redirect stdin to /dev/null and stdout and stderr to log file (this is ugly, but works because descriptors are assigned sequentially): */
		int nullFd=open("/dev/null",O_RDONLY);
		std::string logFileName=logFileDir+"/"+server.name+".log";
		int logFd=open(logFileName.c_str(),O_RDWR|O_CREAT|O_TRUNC,S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH);
		if(nullFd!=0||logFd!=1||dup(logFd)!=2)
			{
			/* Kill this process and let the parent handle it: */
			exit(EXIT_FAILURE);
			}
		
		/* Run the server executable: */
		char* argv[20];
		int argc=0;
		argv[argc++]=const_cast<char*>(server.executableName.c_str());
		argv[argc]=0;
		if(execv(argv[0],argv)<0)
			{
			/* Kill this process and let the parent handle it: */
			exit(EXIT_FAILURE);
			}
		}
	else if(childPid==pid_t(-1))
		{
		/* 'Tis an error: */
		std::cout<<"VRServerLauncher::startServer: Cannot fork "<<server.displayName<<std::endl;
		}
	else
		{
		/* Remember the child's process ID: */
		server.pid=childPid;
		
		/* Save daemon's process ID to its pid file: */
		std::string pidFileName=pidFileDir+"/"+server.name+".pid";
		int pidFd=open(pidFileName.c_str(),O_RDWR|O_CREAT|O_TRUNC,S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH);
		if(pidFd>=0)
			{
			/* Write process ID: */
			char pidBuffer[20];
			snprintf(pidBuffer,sizeof(pidBuffer),"%d\n",server.pid);
			size_t pidLen=strlen(pidBuffer);
			if(write(pidFd,pidBuffer,pidLen)!=ssize_t(pidLen))
				std::cerr<<"VRServerLauncher::startServer: Cannot write "<<server.displayName<<"'s PID to PID file"<<std::endl;
			close(pidFd);
			}
		else
			std::cerr<<"VRServerLauncher::startServer: Cannot create "<<server.displayName<<"'s PID file"<<std::endl;
		
		/* Try connecting to the just-started server's HTTP command socket until it succeeds or times out: */
		unsigned int attempts=10;
		while(attempts>0)
			{
			try
				{
				/* Sleep a bit: */
				usleep(500000);
				
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
			std::cout<<"VRServerLauncher::startServer: Cannot establish connection to "<<server.displayName<<std::endl;
			kill(server.pid,SIGKILL);
			}
		}
	
	return result;
	}

void VRServerLauncher::collectServer(VRServerLauncher::Server& server,bool noWait)
	{
	/* Collect the server's exit status: */
	int waitStatus(0);
	pid_t termPid=waitpid(server.pid,&waitStatus,noWait?WNOHANG:0x0);
	if(termPid==server.pid)
		{
		/* Print a friendly status message: */
		if(WIFEXITED(waitStatus))
			std::cout<<"VRServerLauncher: "<<server.displayName<<" shut down cleanly with exit status "<<WEXITSTATUS(waitStatus)<<std::endl;
		else if(WIFSIGNALED(waitStatus))
			std::cout<<"VRServerLauncher: "<<server.displayName<<" shat the bed with signal "<<WTERMSIG(waitStatus)<<(WCOREDUMP(waitStatus)?" and dumped core":" but did not dump core")<<std::endl;
		
		/* Mark the sub-process as terminated: */
		server.pid=pid_t(0);
		}
	else if(termPid==pid_t(-1))
		{
		/* Print an error message and continue: */
		char strerrorBuffer[1024]; // Long enough according to spec
		char* errorString=strerror_r(errno,strerrorBuffer,sizeof(strerrorBuffer));
		std::cout<<"VRServerLauncher: Error "<<errno<<" ("<<errorString<<") while collecting "<<server.displayName<<std::endl;
		}
	}

void VRServerLauncher::newConnectionCallback(Threads::EventDispatcher::IOEvent& event)
	{
	VRServerLauncher* thisPtr=static_cast<VRServerLauncher*>(event.getUserData());
	
	try
		{
		// std::cout<<"VRServerLauncher: Handling new HTTP request"<<std::endl;
		
		/* Accept the next pending connection: */
		Comm::TCPPipe pipe(thisPtr->listenSocket);
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
				bool success=true;
				
				/* Add an array with server running flags to the reply structure: */
				replyRoot=new IO::JsonObject;
				IO::JsonArrayPointer servers=new IO::JsonArray;
				replyRoot->setProperty("servers",*servers);
				
				/* Start the two server sub-processes: */
				for(int serverIndex=0;serverIndex<2&&success;++serverIndex)
					{
					Server& server=thisPtr->servers[serverIndex];
					
					/* Start the server if it isn't already running: */
					if(server.pid==pid_t(0))
						success=thisPtr->startServer(server,pipe);
					
					/* Add an entry for this server's state to the reply structure: */
					IO::JsonObjectPointer serverState=new IO::JsonObject;
					servers->addItem(*serverState);
					serverState->setProperty("name",server.displayName);
					serverState->setProperty("isRunning",success);
					if(success)
						{
						serverState->setProperty("pid",int(server.pid));
						serverState->setProperty("httpPort",server.httpPort);
						}
					}
				
				replyRoot->setProperty("status",success?"Success":"Failed");
				}
			else if(nvl.front().value=="stopServers")
				{
				/* Stop the two server sub-processes: */
				for(int serverIndex=1;serverIndex>=0;--serverIndex)
					{
					Server& server=thisPtr->servers[serverIndex];
					
					if(server.pid!=pid_t(0))
						{
						std::cout<<"VRServerLauncher: Stopping "<<server.displayName<<std::endl;
						kill(server.pid,SIGTERM);
						thisPtr->collectServer(server,false);
						}
					}
				
				replyRoot->setProperty("status","Success");
				}
			else if(nvl.front().value=="getServerStatus")
				{
				/* Add an array with server running flags and PIDs to the reply structure: */
				replyRoot=new IO::JsonObject;
				IO::JsonArrayPointer servers=new IO::JsonArray;
				replyRoot->setProperty("servers",*servers);
				
				for(int serverIndex=0;serverIndex<2;++serverIndex)
					{
					Server& server=thisPtr->servers[serverIndex];
					
					/* Add an entry for this server's state to the reply structure: */
					IO::JsonObjectPointer serverState=new IO::JsonObject;
					servers->addItem(*serverState);
					serverState->setProperty("name",server.displayName);
					serverState->setProperty("isRunning",server.pid!=pid_t(0));
					if(server.pid!=pid_t(0))
						{
						serverState->setProperty("pid",int(server.pid));
						serverState->setProperty("httpPort",server.httpPort);
						}
					}
				
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
		#if 0 // Actually, don't :)
		/* Print an error message and carry on: */
		std::cout<<"VRServerLauncher: Ignoring HTTP request due to exception "<<err.what()<<std::endl;
		#endif
		}
	}

void VRServerLauncher::childTerminatedCallback(Threads::EventDispatcher::SignalEvent& event)
	{
	VRServerLauncher* thisPtr=static_cast<VRServerLauncher*>(event.getUserData());
	
	/* Reap any terminated child processes: */
	for(int serverIndex=0;serverIndex<2;++serverIndex)
		{
		Server& server=thisPtr->servers[serverIndex];
		
		if(server.pid!=pid_t(0))
			thisPtr->collectServer(server,true);
		}
	}

VRServerLauncher::VRServerLauncher(int listenPortId)
	:listenSocket(listenPortId,5)
	{
	/* Determine the directories where to write pid and log files: */
	uid_t userId=geteuid();
	if(userId==uid_t(0))
		{
		/* Store the files in the appropriate system directories if the server is run as root: */
		pidFileDir="/var/run";
		logFileDir="/var/log";
		}
	else
		{
		/* Store the files in the user's home directory: */
		char* home=getenv("HOME");
		if(home!=0)
			pidFileDir=home;
		else
			{
			/* Fall back to the tmp directory if no home directory is set: */
			pidFileDir="/tmp";
			}
		
		logFileDir=pidFileDir;
		}
	
	/*********************************************************************
	Initialize the server tracking structures:
	*********************************************************************/
	
	/* Server 0: VRDeviceDaemon: */
	servers[0].name="VRDeviceDaemon";
	servers[0].displayName="VR tracking driver";
	servers[0].executableName=VRUI_INTERNAL_CONFIG_EXECUTABLEDIR "/RunOpenVRTracker.sh";
	servers[0].pid=pid_t(0);
	servers[0].socketName="VRDeviceDaemon.socket";
	servers[0].socketAbstract=true;
	servers[0].httpPort=8081;
	
	/* Server 1: VRCompositingServer: */
	servers[1].name="VRCompositingServer";
	servers[1].displayName="VR compositing server";
	servers[1].executableName=VRUI_INTERNAL_CONFIG_EXECUTABLEDIR "/RunVRCompositor.sh";
	servers[1].pid=pid_t(0);
	servers[1].socketName="VRCompositingServer.socket";
	servers[1].socketAbstract=true;
	servers[1].httpPort=8082;
	
	/* Ignore SIGPIPE and leave handling of pipe errors to TCP sockets: */
	Comm::ignorePipeSignals();
	
	/* Stop the launcher when a signal is received: */
	eventDispatcher.stopOnSignals();
	
	/* Handle events on the TCP listening socket: */
	listenSocketKey=eventDispatcher.addIOEventListener(listenSocket.getFd(),Threads::EventDispatcher::Read,VRServerLauncher::newConnectionCallback,this);
	
	/* Create a signal to receive notifications when one of the sub-processes terminates: */
	sigChldKey=eventDispatcher.addSignalListener(childTerminatedCallback,this);
	}

VRServerLauncher::~VRServerLauncher(void)
	{
	/* Stop handling events on the TCP listening socket: */
	eventDispatcher.removeIOEventListener(listenSocketKey);
	
	/* Delete the sub-process termination signals: */
	eventDispatcher.removeSignalListener(sigChldKey);
	}

void VRServerLauncher::run(void)
	{
	std::cout<<"VRServerLauncher: Starting server launcher"<<std::endl;
	
	/* Catch SIGCHLD signals: */
	sigThis=this;
	struct sigaction sigChldAction;
	memset(&sigChldAction,0,sizeof(struct sigaction));
	sigChldAction.sa_handler=signalHandler;
	if(sigaction(SIGCHLD,&sigChldAction,0)<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot intercept SIGCHLD");
	
	/* Dispatch I/O events until the dispatcher is shut down: */
	std::cout<<"VRServerLauncher: Servicing HTTP POST requests on TCP port 8080"<<std::endl;
	eventDispatcher.dispatchEvents();
	
	/* Shut down the sub-processes: */
	for(int serverIndex=1;serverIndex>=0;--serverIndex)
		{
		Server& server=servers[serverIndex];
		if(server.pid!=pid_t(0))
			{
			std::cout<<"VRServerLauncher: Shutting down "<<server.displayName<<std::endl;
			kill(server.pid,SIGTERM);
			collectServer(server,false);
			server.pid=pid_t(0);
			}
		}
	
	std::cout<<"VRServerLauncher: Shutting down server launcher"<<std::endl;
	}

int main(int argc,char* argv[])
	{
	/* Parse the command line: */
	bool daemonize=argc>1&&strcmp(argv[1],"-D")==0;
	int listenPortId=8080;
	
	/* Turn the server into a daemon if requested: */
	if(daemonize)
		{
		/* Fork once (and exit) to notify shell or caller that the program is done: */
		int childPid=fork();
		if(childPid<0)
			{
			std::cerr<<"VRServerLauncher: Error during fork"<<std::endl<<std::flush;
			return 1; // Fork error
			}
		else if(childPid>0)
			{
			/* Print the daemon's process ID: */
			std::cout<<"VRServerLauncher: Started daemon with PID "<<childPid<<std::endl;
			
			/* Save daemon's process ID to pid file: */
			int pidFd=open("/var/run/VRServerLauncher.pid",O_RDWR|O_CREAT|O_TRUNC,S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH);
			if(pidFd>=0)
				{
				/* Write process ID: */
				char pidBuffer[20];
				snprintf(pidBuffer,sizeof(pidBuffer),"%d\n",childPid);
				size_t pidLen=strlen(pidBuffer);
				if(write(pidFd,pidBuffer,pidLen)!=ssize_t(pidLen))
					std::cerr<<"VRServerLauncher: Cannot write PID to PID file"<<std::endl;
				close(pidFd);
				}
			else
				std::cerr<<"VRServerLauncher: Cannot create PID file"<<std::endl;
			
			return 0; // Parent process exits
			}
		
		/* Set new session ID to become independent process without controlling tty: */
		setsid();
		
		/* Close all inherited file descriptors: */
		for(int i=getdtablesize()-1;i>=0;--i)
			close(i);
		
		/* Redirect stdin to /dev/null and stdout and stderr to log file (this is ugly, but works because descriptors are assigned sequentially): */
		int nullFd=open("/dev/null",O_RDONLY);
		int logFd=open("/var/log/VRServerLauncher.log",O_RDWR|O_CREAT|O_TRUNC,S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH);
		if(nullFd!=0||logFd!=1||dup(logFd)!=2)
			{
			/* We can't write an error message at this point, but we can quit: */
			return 1;
			}
		}
	
	try
		{
		/* Create a server launcher and run it until it shuts down: */
		VRServerLauncher serverLauncher(listenPortId);
		serverLauncher.run();
		
		/* If the server was daemonized, remove the pid file: */
		if(daemonize)
			unlink("/var/run/VRServerLauncher.pid");
		}
	catch(const std::runtime_error& err)
		{
		std::cerr<<"VRServerLauncher: Shutting down with exception "<<err.what()<<std::endl;
		}
	
	return 0;
	}
