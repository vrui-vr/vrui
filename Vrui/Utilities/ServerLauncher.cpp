/***********************************************************************
ServerLauncher - A small daemon to launch and monitor the servers needed
to operate a VR environment with a head-mounted display, using
VRDeviceDaemon for tracking and VRCompositingServer for rendering.
Copyright (c) 2025 Oliver Kreylos
***********************************************************************/

#include <utility>
#include <string>
#include <vector>
#include <Threads/EventDispatcher.h>
#include <IO/ValueSource.h>
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
	
	/* Private methods: */
	static NameValueList parsePostRequest(Comm::TCPPipe& pipe,const char* rootUrl); // Parses an HTTP POST request arriving on the given pipe
	static void newConnectionCallback(Threads::EventDispatcher::IOEvent& event);
	
	/* Constructors and destructors: */
	public:
	ServerLauncher(void);
	~ServerLauncher(void);
	
	/* Methods: */
	void run(void);
	};

/*******************************
Methods of class ServerLauncher:
*******************************/

ServerLauncher::NameValueList ServerLauncher::parsePostRequest(Comm::TCPPipe& pipe,const char* rootUrl)
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
	requestOk=requestOk&&request.isString(rootUrl);
	
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
	
	/* Parse the actual command name/value pairs: */
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
			
			/* Go to the next name/value pair: */
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
	
	/* Parse an incoming HTTP POST request: */
	NameValueList payload=parsePostRequest(pipe);
	
	/* Check that there's exactly one command in the POST request: */
	if(payload.size()==1&&payload.front().first=="command")
		{
		/* Handle the command: */
		
		}
	}

ServerLauncher::ServerLauncher(void)
	:listenSocket(8080,5)
	{
	/* Stop the launcher when a signal is received: */
	eventDispatcher.stopOnSignals();
	
	/* Handle events on the TCP listening socket: */
	listenSocketKey=eventDispatcher.addIOEventListener(listenSocket.getFd(),Threads::EventDispatcher::ReadOnly,ServerLauncher::newConnectionCallback,this);
	}

ServerLauncher::~ServerLauncher(void)
	{
	/* Stop handling events on the TCP listening socket: */
	eventDispatcher.removeIOEventListener(listenSocketKey);
	}

void ServerLauncher::run(void)
	{
	/* Dispatch I/O events until the dispatcher is shut down: */
	eventDispatcher.handleEvents();
	}

int main(void)
	{
	/* Create a server launcher and run it until it shuts down: */
	ServerLauncher serverLaucher;
	serverLauncher.run();
	
	return 0;
	}
