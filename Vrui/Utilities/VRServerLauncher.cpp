/***********************************************************************
VRServerLauncher - A small daemon to launch and monitor the servers
needed to operate a VR environment with a head-mounted display, using
VRDeviceDaemon for tracking and VRCompositingServer for rendering.
Copyright (c) 2025-2026 Oliver Kreylos
***********************************************************************/

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

class VRServerLauncher
	{
	/* Elements: */
	private:
	Threads::EventDispatcher eventDispatcher; // Dispatcher to handle I/O events
	Comm::ListeningTCPSocket listenSocket; // Socket listening for incoming TCP connections
	Threads::EventDispatcher::ListenerKey listenSocketKey;
	static VRServerLauncher* sigThis; // Pointer to active server launcher object for signal processing
	Threads::EventDispatcher::ListenerKey sigChldKey; // Signal to notify the front-end that any of the sub-processes has terminated
	std::string serverNames[2]; // Names of the server processes to start
	std::string serverDisplayNames[2]; // Display names of the server processes to start
	std::string serverLogFileNames[2]; // Names of log files to which to redirect server output
	pid_t serverPids[2]; // Process IDs of the VRDeviceDaemon and VRCompositingServer sub-processes, respectively, or 0 if the process is not started
	std::string serverSocketNames[2]; // Names of the UNIX sockets on which the server processes listen for connections
	bool serverSocketAbstracts[2]; // Flags if the server processes' UNIX sockets are in the abstract namespace
	
	/* Private methods: */
	static void signalHandler(int sig); // Signal handler for SIGCHLD
	bool startServer(int serverIndex,Comm::TCPPipe& pipe);
	void collectServer(int serverIndex,bool noWait);
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

bool VRServerLauncher::startServer(int serverIndex,Comm::TCPPipe& pipe)
	{
	bool result=false;
	
	std::cout<<"VRServerLauncher::startServer: Starting "<<serverDisplayNames[serverIndex]<<std::endl;
	
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
		int logFd=open(serverLogFileNames[serverIndex].c_str(),O_RDWR|O_CREAT|O_TRUNC,S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH);
		if(nullFd!=0||logFd!=1||dup(logFd)!=2)
			{
			/* Kill this process and let the parent handle it: */
			exit(EXIT_FAILURE);
			}
		
		/* Run the server executable: */
		char* argv[20];
		int argc=0;
		argv[argc++]=const_cast<char*>(serverNames[serverIndex].c_str());
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
		std::cout<<"VRServerLauncher::startServer: Cannot fork "<<serverDisplayNames[serverIndex]<<std::endl;
		}
	else
		{
		/* Remember the child's process ID: */
		serverPids[serverIndex]=childPid;
		
		/* Try connecting to the just-started server's HTTP command socket until it succeeds or times out: */
		unsigned int attempts=10;
		while(attempts>0)
			{
			try
				{
				/* Sleep a bit: */
				usleep(500000);
				
				/* Open a connection to the server's HTTP port: */
				Comm::TCPPipe httpPipe("localhost",8081+serverIndex);
				httpPipe.ref();
				
				/* Send a malformed HTTP request: */
				IO::OStream request(&httpPipe);
				request<<"POST /Foo.cgi HTTP/1.1\n";
				request<<"Host: localhost:"<<8081+serverIndex<<"\n";
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
			std::cout<<"VRServerLauncher::startServer: "<<serverDisplayNames[serverIndex]<<" started successfully on PID "<<childPid<<std::endl;
			result=true;
			}
		else
			{
			/* Kill the server brutally, because something serious went wrong: */
			std::cout<<"VRServerLauncher::startServer: Cannot establish connection to "<<serverDisplayNames[serverIndex]<<std::endl;
			kill(serverPids[serverIndex],SIGKILL);
			}
		}
	
	return result;
	}

void VRServerLauncher::collectServer(int serverIndex,bool noWait)
	{
	/* Collect the server's exit status: */
	int waitStatus(0);
	pid_t termPid=waitpid(serverPids[serverIndex],&waitStatus,noWait?WNOHANG:0x0);
	if(termPid==serverPids[serverIndex])
		{
		/* Print a friendly status message: */
		if(WIFEXITED(waitStatus))
			std::cout<<"VRServerLauncher: "<<serverDisplayNames[serverIndex]<<" shut down cleanly with exit status "<<WEXITSTATUS(waitStatus)<<std::endl;
		else if(WIFSIGNALED(waitStatus))
			std::cout<<"VRServerLauncher: "<<serverDisplayNames[serverIndex]<<" shat the bed with signal "<<WTERMSIG(waitStatus)<<(WCOREDUMP(waitStatus)?" and dumped core":" but did not dump core")<<std::endl;
		
		/* Mark the sub-process as terminated: */
		serverPids[serverIndex]=pid_t(0);
		}
	else if(termPid==-1)
		{
		/* Print an error message and continue: */
		char strerrorBuffer[1024]; // Long enough according to spec
		char* errorString=strerror_r(errno,strerrorBuffer,sizeof(strerrorBuffer));
		std::cout<<"VRServerLauncher: Error "<<errno<<" ("<<errorString<<") while collecting "<<serverDisplayNames[serverIndex]<<std::endl;
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
				for(int i=0;i<2&&success;++i)
					{
					/* Start the server if it isn't already running: */
					if(thisPtr->serverPids[i]==pid_t(0))
						success=thisPtr->startServer(i,pipe);
					
					/* Add an entry for this server to the reply structure: */
					IO::JsonObjectPointer server=new IO::JsonObject;
					servers->addItem(*server);
					server->setProperty("name",thisPtr->serverDisplayNames[i]);
					server->setProperty("isRunning",success);
					}
				
				replyRoot->setProperty("status",success?"Success":"Failed");
				}
			else if(nvl.front().value=="stopServers")
				{
				/* Stop the two server sub-processes: */
				for(int i=1;i>=0;--i)
					if(thisPtr->serverPids[i]!=pid_t(0))
						{
						std::cout<<"VRServerLauncher: Stopping "<<thisPtr->serverDisplayNames[i]<<std::endl;
						kill(thisPtr->serverPids[i],SIGTERM);
						thisPtr->collectServer(i,false);
						}
				
				replyRoot->setProperty("status","Success");
				}
			else if(nvl.front().value=="getServerStatus")
				{
				/* Add an array with server running flags and PIDs to the reply structure: */
				replyRoot=new IO::JsonObject;
				IO::JsonArrayPointer servers=new IO::JsonArray;
				replyRoot->setProperty("servers",*servers);
				
				for(int i=0;i<2;++i)
					{
					/* Add an entry for this server to the reply structure: */
					IO::JsonObjectPointer server=new IO::JsonObject;
					servers->addItem(*server);
					server->setProperty("name",thisPtr->serverDisplayNames[i]);
					server->setProperty("isRunning",thisPtr->serverPids[i]!=pid_t(0));
					if(thisPtr->serverPids[i]!=pid_t(0))
						{
						server->setProperty("pid",int(thisPtr->serverPids[i]));
						server->setProperty("httpPort",8081+i);
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
	for(int i=0;i<2;++i)
		if(thisPtr->serverPids[i]!=pid_t(0))
			thisPtr->collectServer(i,true);
	}

VRServerLauncher::VRServerLauncher(int listenPortId)
	:listenSocket(listenPortId,5)
	{
	/* Set the names of the server executables: */
	serverNames[0]="/opt/Vrui-dev/bin/RunOpenVRTracker.sh";
	serverNames[1]="/opt/Vrui-dev/bin/RunVRCompositor.sh";
	
	serverDisplayNames[0]="VR tracking driver";
	serverDisplayNames[1]="VR compositing server";
	
	/* Set the names of the log files to which to direct server output: */
	#if 0
	serverLogFileNames[0]="/var/log/VRDeviceDaemon.log";
	serverLogFileNames[1]="/var/log/VRCompositingServer.log";
	#else
	serverLogFileNames[0]="/home/okreylos/VRDeviceDaemon.log";
	serverLogFileNames[1]="/home/okreylos/VRCompositingServer.log";
	#endif
	
	/* Set the names of the UNIX sockets on which the servers listen for connections: */
	serverSocketNames[0]="VRDeviceDaemon.socket";
	serverSocketAbstracts[0]=true;
	serverSocketNames[1]="VRCompositingServer.socket";
	serverSocketAbstracts[1]=true;
	
	/* Initialize the child process IDs: */
	serverPids[1]=serverPids[0]=pid_t(0);
	
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
	for(int i=1;i>=0;--i)
		if(serverPids[i]!=pid_t(0))
			{
			std::cout<<"VRServerLauncher: Shutting down "<<serverDisplayNames[i]<<std::endl;
			kill(serverPids[i],SIGTERM);
			collectServer(i,false);
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
		
		/* Redirect stdin, stdout and stderr to log file (this is ugly, but works because descriptors are assigned sequentially): */
		int logFd=open("/var/log/VRServerLauncher.log",O_RDWR|O_CREAT|O_TRUNC,S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH);
		if(logFd!=0||dup(logFd)!=1||dup(logFd)!=2)
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
		unlink("/var/run/VRServerLauncher.pid");
		}
	catch(const std::runtime_error& err)
		{
		std::cerr<<"VRServerLauncher: Shutting down with exception "<<err.what()<<std::endl;
		}
	
	return 0;
	}
