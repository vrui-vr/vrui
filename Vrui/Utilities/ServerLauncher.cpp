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
#include <iostream>
#include <Misc/StdError.h>
#include <Threads/EventDispatcher.h>
#include <IO/ValueSource.h>
#include <IO/OStream.h>
#include <Comm/TCPPipe.h>
#include <Comm/ListeningTCPSocket.h>

class ServerLauncher
	{
	/* Embedded classes: */
	private:
	typedef std::pair<std::string,std::string> NameValue; // Type for a name=value pair parsed from an HTTP POST request
	typedef std::vector<NameValue> NameValueList; // Type for lists of name=value pairs
	
	/* Elements: */
	private:
	Threads::EventDispatcher eventDispatcher; // Dispatcher to handle I/O events
	Comm::ListeningTCPSocket listenSocket; // Socket listening for incoming TCP connections
	Threads::EventDispatcher::ListenerKey listenSocketKey;
	static ServerLauncher* sigThis; // Pointer to active server launcher object for signal processing
	Threads::EventDispatcher::ListenerKey sigChldKey; // Signal to notify the front-end that any of the sub-processes has terminated
	pid_t serverPids[2]; // Process IDs of the VRDeviceDaemon and VRCompositingServer sub-processes, respectively, or 0 if the process is not started
	
	/* Private methods: */
	static void signalHandler(int sig); // Signal handler for SIGCHLD
	void startVRDeviceDaemon(void);
	void startVRCompositingServer(void);
	static NameValueList parsePostRequest(Comm::TCPPipe& pipe,const char* actionUrl); // Parses an HTTP POST request arriving on the given pipe
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

void ServerLauncher::startVRDeviceDaemon(void)
	{
	/* Fork: */
	pid_t childPid=fork();
	if(childPid==pid_t(0))
		{
		/* Run the OpenVRTracker.sh script: */
		char* argv[20];
		int argc=0;
		argv[argc++]=const_cast<char*>("/opt/Vrui-dev/bin/RunOpenVRTracker.sh");
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
		serverPids[0]=childPid;
		}
	}

void ServerLauncher::startVRCompositingServer(void)
	{
	/* Fork: */
	pid_t childPid=fork();
	if(childPid==pid_t(0))
		{
		/* Run the RunVRCompositor.sh script: */
		char* argv[20];
		int argc=0;
		argv[argc++]=const_cast<char*>("/opt/Vrui-dev/bin/RunVRCompositor.sh.sh");
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
		serverPids[1]=childPid;
		}
	}

ServerLauncher::NameValueList ServerLauncher::parsePostRequest(Comm::TCPPipe& pipe,const char* actionUrl)
	{
	/* Parse the request header: */
	bool requestOk=true;
	unsigned int contentLength=0;
	
	{
	/* Attach a value source to the connection to parse the client's request: */
	IO::ValueSource request(&pipe);
	request.setPunctuation("\n");
	request.setWhitespace(" \r");
	request.skipWs();
	
	/* Check for the POST keyword: */
	requestOk=requestOk&&request.isString("POST");
	
	/* Check for a valid root URL: */
	requestOk=requestOk&&request.isString(actionUrl);
	
	/* Check for the protocol identifier: */
	requestOk=requestOk&&request.isString("HTTP/1.1")&&request.isLiteral('\n');
	
	/* Parse request data fields: */
	request.setPunctuation(":\n");
	bool haveContentType=false;
	while(!request.eof()&&requestOk)
		{
		/* Bail out if the line is empty: */
		if(request.peekc()=='\n')
			break;
		
		/* Read a data field: */
		std::string fieldName;
		if(requestOk)
			fieldName=request.readString();
		requestOk=requestOk&&request.isLiteral(':');
		std::string fieldValue;
		if(requestOk)
			{
			fieldValue=request.readLine();
			request.skipWs();
			
			/* Strip trailing whitespace from the field value: */
			while(isspace(fieldValue.back()))
				fieldValue.pop_back();
			}
		
		/* Parse the data field: */
		if(requestOk)
			{
			if(fieldName=="Content-Type")
				{
				requestOk=fieldValue=="application/x-www-form-urlencoded";
				haveContentType=true;
				}
			else if(fieldName=="Content-Length")
				{
				contentLength=atoi(fieldValue.c_str());
				}
			}
		}
	requestOk=requestOk&&haveContentType&&request.isLiteral('\n');
	}
	
	/* Parse the request's name=value pairs: */
	NameValueList result;
	if(requestOk)
		{
		void* readBuffer;
		size_t readSize=pipe.readInBuffer(readBuffer,contentLength);
		char* postStart=static_cast<char*>(readBuffer);
		char* postEnd=postStart+readSize;
		char* pPtr=postStart;
		while(pPtr!=postEnd)
			{
			/* Extract the next name=value pair: */
			char* nameStart=pPtr;
			char* nameEnd;
			for(nameEnd=nameStart;nameEnd!=postEnd&&*nameEnd!='=';++nameEnd)
				;
			if(nameEnd==postEnd)
				break;
			
			char* valueStart=nameEnd+1;
			char* valueEnd;
			for(valueEnd=valueStart;valueEnd!=postEnd&&*valueEnd!='&';++valueEnd)
				;
			
			/* Add the name=value pair to the result list: */
			result.push_back(NameValue(std::string(nameStart,nameEnd),std::string(valueStart,valueEnd)));
			
			/* Go to the next potential name=value pair: */
			pPtr=valueEnd;
			if(pPtr!=postEnd)
				++pPtr;
			}
		}
	
	return result;
	}

void ServerLauncher::newConnectionCallback(Threads::EventDispatcher::IOEvent& event)
	{
	ServerLauncher* thisPtr=static_cast<ServerLauncher*>(event.getUserData());
	
	/* Accept the next pending connection: */
	Comm::TCPPipe pipe(thisPtr->listenSocket);
	pipe.ref();
	
	/* Parse an incoming HTTP POST request: */
	NameValueList payload=parsePostRequest(pipe,"/ServerLauncher.cgi");
	
	/* Check that there is exactly one command in the POST request: */
	if(payload.size()==1&&payload.front().first=="command")
		{
		/* Handle the command: */
		if(payload.front().second=="startServers")
			{
			/* Start the VRDeviceDaemon sub-process: */
			if(thisPtr->serverPids[0]==pid_t(0))
				{
				std::cout<<"ServerLauncher: Starting VR tracking driver"<<std::endl;
				thisPtr->startVRDeviceDaemon();
				}
			}
		else if(payload.front().second=="stopServers")
			{
			
			/* Stop the VRDeviceDaemon sub-process: */
			if(thisPtr->serverPids[0]!=pid_t(0))
				{
				std::cout<<"ServerLauncher: Stopping VR tracking driver"<<std::endl;
				kill(thisPtr->serverPids[0],SIGTERM);
				}
			}
		}
	
	/* Reply to the request: */
	IO::OStream reply(&pipe);
	reply<<"HTTP/1.1 200 OK\n";
	reply<<"\n"<<std::endl;
	
	pipe.flush();
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
		{
		int waitStatus(0);
		pid_t termPid=waitpid(thisPtr->serverPids[i],&waitStatus,WNOHANG);
		if(termPid==thisPtr->serverPids[i])
			{
			/* Print a friendly status message: */
			if(WIFEXITED(waitStatus))
				std::cout<<"ServerLauncher: "<<subProcessNames[i]<<" shut down cleanly with exit status "<<WEXITSTATUS(waitStatus)<<std::endl;
			else if(WIFSIGNALED(waitStatus))
				std::cout<<"ServerLauncher: "<<subProcessNames[i]<<" shat the bed with signal "<<WTERMSIG(waitStatus)<<(WCOREDUMP(waitStatus)?"and dumped core":" but did not dump core")<<std::endl;
			
			/* Mark the sub-process as terminated: */
			thisPtr->serverPids[i]=pid_t(0);
			}
		}
	}

ServerLauncher::ServerLauncher(void)
	:listenSocket(8080,5)
	{
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
	/* Create a server launcher and run it until it shuts down: */
	ServerLauncher serverLauncher;
	serverLauncher.run();
	
	return 0;
	}
