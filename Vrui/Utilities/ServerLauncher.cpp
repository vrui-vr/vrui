/***********************************************************************
ServerLauncher - A small daemon to launch and monitor the servers needed
to operate a VR environment with a head-mounted display, using
VRDeviceDaemon for tracking and VRCompositingServer for rendering.
Copyright (c) 2025 Oliver Kreylos
***********************************************************************/

#include <string.h>
#include <errno.h>
#include <unistd.h>
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
#include <Comm/TCPPipe.h>
#include <Comm/ListeningTCPSocket.h>
#include <Comm/HttpPostRequest.h>

class ServerLauncher
	{
	/* Elements: */
	private:
	Threads::EventDispatcher eventDispatcher; // Dispatcher to handle I/O events
	Comm::ListeningTCPSocket listenSocket; // Socket listening for incoming TCP connections
	Threads::EventDispatcher::ListenerKey listenSocketKey;
	static ServerLauncher* sigThis; // Pointer to active server launcher object for signal processing
	Threads::EventDispatcher::ListenerKey sigChldKey; // Signal to notify the front-end that any of the sub-processes has terminated
	std::string serverNames[2]; // Names of the server processes to start
	pid_t serverPids[2]; // Process IDs of the VRDeviceDaemon and VRCompositingServer sub-processes, respectively, or 0 if the process is not started
	
	/* Private methods: */
	static void signalHandler(int sig); // Signal handler for SIGCHLD
	void startServer(int serverIndex,Comm::TCPPipe& pipe);
	static void newConnectionCallback(Threads::EventDispatcher::IOEvent& event);
	static void childTerminatedCallback(Threads::EventDispatcher::SignalEvent& event);
	
	/* Constructors and destructors: */
	public:
	ServerLauncher(void);
	~ServerLauncher(void);
	
	/* Methods: */
	void run(void);
	};

/***************************************
Static elements of class ServerLauncher:
***************************************/

ServerLauncher* ServerLauncher::sigThis(0);

/*******************************
Methods of class ServerLauncher:
*******************************/

void ServerLauncher::signalHandler(int sig)
	{
	if(sigThis!=0&&sig==SIGCHLD)
		{
		/* Notify the server launcher that a child terminated: */
		sigThis->eventDispatcher.signal(sigThis->sigChldKey,0);
		}
	}

void ServerLauncher::startServer(int serverIndex,Comm::TCPPipe& pipe)
	{
	/* Fork: */
	pid_t childPid=fork();
	if(childPid==pid_t(0))
		{
		/* Close the pipe in the child process: */
		pipe.~TCPPipe();
		
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
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot fork process");
		}
	else
		{
		/* Remember the child's process ID: */
		serverPids[serverIndex]=childPid;
		}
	}

void ServerLauncher::newConnectionCallback(Threads::EventDispatcher::IOEvent& event)
	{
	ServerLauncher* thisPtr=static_cast<ServerLauncher*>(event.getUserData());
	
	try
		{
		// std::cout<<"ServerLauncher: Handling new HTTP request"<<std::endl;
		
		/* Accept the next pending connection: */
		Comm::TCPPipe pipe(thisPtr->listenSocket);
		pipe.ref();
		
		/* Parse an incoming HTTP POST request: */
		Comm::HttpPostRequest request(pipe);
		const Comm::HttpPostRequest::NameValueList& nvl=request.getNameValueList();
		
		/* Optionally create an entity to send a reply to the client: */
		IO::JsonObjectPointer replyRoot;
		
		/* Check that there is exactly one command in the POST request: */
		if(request.getActionUrl()=="/ServerLauncher.cgi"&&nvl.size()==1&&nvl.front().name=="command")
			{
			/* Handle the command: */
			if(nvl.front().value=="isAlive")
				{
				/* Do nothing; just send an HTTP reply */
				}
			else if(nvl.front().value=="startServers")
				{
				/* Start the VRDeviceDaemon sub-process: */
				if(thisPtr->serverPids[0]==pid_t(0))
					{
					std::cout<<"ServerLauncher: Starting VR tracking driver"<<std::endl;
					thisPtr->startServer(0,pipe);
					}
				}
			else if(nvl.front().value=="stopServers")
				{
				/* Stop the VRDeviceDaemon sub-process: */
				if(thisPtr->serverPids[0]!=pid_t(0))
					{
					std::cout<<"ServerLauncher: Stopping VR tracking driver"<<std::endl;
					kill(thisPtr->serverPids[0],SIGTERM);
					}
				}
			else if(nvl.front().value=="getServerStatus")
				{
				replyRoot=new IO::JsonObject;
				
				IO::JsonArrayPointer servers=new IO::JsonArray;
				replyRoot->setProperty("servers",*servers);
				
				static const char* serverNames[2]=
					{
					"VRDeviceDaemon","VRCompositingServer"
					};
				for(int i=0;i<2;++i)
					{
					IO::JsonObjectPointer server=new IO::JsonObject;
					servers->addItem(*server);
					
					server->setProperty("name",serverNames[i]);
					server->setProperty("isRunning",thisPtr->serverPids[i]!=pid_t(0));
					if(thisPtr->serverPids[i]!=pid_t(0))
						server->setProperty("pid",int(thisPtr->serverPids[i]));
					}
				}
			}
		
		/* Reply to the request: */
		IO::OStream reply(&pipe);
		reply<<"HTTP/1.1 200 OK\n";
		if(replyRoot!=0)
			{
			reply<<"Content-Type: application/json\n";
			reply<<"Access-Control-Allow-Origin: *\n";
			}
		reply<<"\n";
		if(replyRoot!=0)
			reply<<*replyRoot;
		reply<<std::endl;
		
		pipe.flush();
		
		// std::cout<<"ServerLauncher: Done with HTTP request"<<std::endl;
		}
	catch(const std::runtime_error& err)
		{
		// std::cerr<<"ServerLauncher: Ignoring HTTP request due to exception "<<err.what()<<std::endl;
		}
	}

void ServerLauncher::childTerminatedCallback(Threads::EventDispatcher::SignalEvent& event)
	{
	ServerLauncher* thisPtr=static_cast<ServerLauncher*>(event.getUserData());
	
	/* Reap any terminated child processes: */
	static const char* subProcessNames[]={
		"VR tracking driver",
		"VR compositing server"
		};
	for(int i=0;i<2;++i)
		if(thisPtr->serverPids[i]!=pid_t(0))
			{
			int waitStatus(0);
			pid_t termPid=waitpid(thisPtr->serverPids[i],&waitStatus,WNOHANG);
			if(termPid==thisPtr->serverPids[i])
				{
				/* Print a friendly status message: */
				if(WIFEXITED(waitStatus))
					std::cout<<"ServerLauncher: "<<subProcessNames[i]<<" shut down cleanly with exit status "<<WEXITSTATUS(waitStatus)<<std::endl;
				else if(WIFSIGNALED(waitStatus))
					std::cout<<"ServerLauncher: "<<subProcessNames[i]<<" shat the bed with signal "<<WTERMSIG(waitStatus)<<(WCOREDUMP(waitStatus)?" and dumped core":" but did not dump core")<<std::endl;
				
				/* Mark the sub-process as terminated: */
				thisPtr->serverPids[i]=pid_t(0);
				}
			}
	}

ServerLauncher::ServerLauncher(void)
	:listenSocket(8080,5)
	{
	/* Set the names of the server executables: */
	serverNames[0]="/opt/Vrui-dev/bin/RunOpenVRTracker.sh";
	serverNames[1]="/opt/Vrui-dev/bin/RunVRCompositor.sh";
	
	/* Initialize the child process IDs: */
	serverPids[1]=serverPids[0]=pid_t(0);
	
	/* Stop the launcher when a signal is received: */
	eventDispatcher.stopOnSignals();
	
	/* Handle events on the TCP listening socket: */
	listenSocketKey=eventDispatcher.addIOEventListener(listenSocket.getFd(),Threads::EventDispatcher::Read,ServerLauncher::newConnectionCallback,this);
	
	/* Create a signal to receive notifications when one of the sub-processes terminates: */
	sigChldKey=eventDispatcher.addSignalListener(childTerminatedCallback,this);
	}

ServerLauncher::~ServerLauncher(void)
	{
	/* Stop handling events on the TCP listening socket: */
	eventDispatcher.removeIOEventListener(listenSocketKey);
	
	/* Delete the sub-process termination signals: */
	eventDispatcher.removeSignalListener(sigChldKey);
	}

void ServerLauncher::run(void)
	{
	/* Catch SIGCHLD signals: */
	sigThis=this;
	struct sigaction sigChldAction;
	memset(&sigChldAction,0,sizeof(struct sigaction));
	sigChldAction.sa_handler=signalHandler;
	if(sigaction(SIGCHLD,&sigChldAction,0)<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot intercept SIGCHLD");
	
	/* Dispatch I/O events until the dispatcher is shut down: */
	std::cout<<"ServerLauncher: Servicing HTTP POST requests on TCP port 8080"<<std::endl;
	eventDispatcher.dispatchEvents();
	
	/* Stop catching SIGCHLD signals: */
	sigThis=0;
	memset(&sigChldAction,0,sizeof(struct sigaction));
	sigChldAction.sa_handler=SIG_IGN;
	sigaction(SIGCHLD,&sigChldAction,0);
	
	/* Shut down the sub-processes: */
	std::cout<<"ServerLauncher: Shutting down sub-processes"<<std::endl;
	for(int i=1;i>=0;--i)
		{
		if(serverPids[i]!=pid_t(0))
			{
			kill(serverPids[i],SIGTERM);
			waitpid(serverPids[i],0,0x0);
			}
		}
	}

int main(void)
	{
	try
		{
		/* Create a server launcher and run it until it shuts down: */
		ServerLauncher serverLauncher;
		serverLauncher.run();
		}
	catch(const std::runtime_error& err)
		{
		std::cerr<<"ServerLauncher: Shutting down with exception "<<err.what()<<std::endl;
		}
	
	return 0;
	}
