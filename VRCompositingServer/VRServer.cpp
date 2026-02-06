/***********************************************************************
VRServer - Prototype for a VR server offering compositing services to VR
application clients.
Copyright (c) 2022-2026 Oliver Kreylos

This file is part of the Vrui VR Compositing Server (VRCompositor).

The Vrui VR Compositing Server is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Vrui VR Compositing Server is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vrui VR Compositing Server; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <string>
#include <iostream>
#include <Misc/SizedTypes.h>
#include <Misc/SelfDestructPointer.h>
#include <Misc/CommandLineParser.h>
#include <Threads/Thread.h>
#include <Threads/EventDispatcher.h>
#include <IO/JsonEntityTypes.h>
#include <IO/OStream.h>
#include <Comm/Pipe.h>
#include <Comm/ListeningUNIXSocket.h>
#include <Comm/UNIXPipe.h>
#include <Comm/ListeningTCPSocket.h>
#include <Comm/TCPPipe.h>
#include <Comm/HttpPostRequest.h>
#include <vulkan/vulkan.h>
#include <Vulkan/Common.h>
#include <Vulkan/ApplicationInfo.h>
#include <Vulkan/Instance.h>
#include <Vulkan/DebugUtilsMessenger.h>
#include <Vrui/Internal/VRDeviceClient.h>
#include <Vrui/Internal/VRCompositorProtocol.h>

#include "Config.h"
#include "VRCompositor.h"

class VRServer
	{
	/* Elements: */
	private:
	Threads::EventDispatcher dispatcher; // An event dispatcher to handle I/O events
	Vrui::VRDeviceClient vrDeviceClient; // Client connected to a Vrui VR device server
	
	/* The VR compositor: */
	VRCompositor compositor; // The VR compositor object
	Threads::Thread compositorThread; // Thread running the compositor's main loop
	volatile bool compositorCrashed; // Flag if the compositor crashed due to an unhandled exception
	
	/* A UNIX socket and pipe to communicate with VR application clients: */
	Comm::ListeningUNIXSocket listenSocket; // UNIX socket listening for incoming client connections
	Comm::UNIXPipe* clientPipe; // UNIX pipe connected to the current client
	Comm::ListeningSocketPtr httpListenSocket; // Optional TCP socket listening for incoming HTTP requests
	Threads::EventDispatcher::ListenerKey stdioListener; // Key for listener for standard input
	Threads::EventDispatcher::ListenerKey listenSocketListener; // Key for listener for listening UNIX socket
	Threads::EventDispatcher::ListenerKey httpListenSocketListener; // Key for listener for listening TCP socket
	Threads::EventDispatcher::ListenerKey clientPipeListener; // Key for listener for the current client pipe
	Threads::EventDispatcher::ListenerKey vsyncSignalListener; // Key for listener for vsync events from the compositor
	
	/* Private methods: */
	void stdioCallback(Threads::EventDispatcher::IOEvent& event);
	void listenSocketCallback(Threads::EventDispatcher::IOEvent& event);
	void httpListenSocketCallback(Threads::EventDispatcher::IOEvent& event);
	void clientPipeCallback(Threads::EventDispatcher::IOEvent& event);
	void vsyncSignalCallback(Threads::EventDispatcher::SignalEvent& event);
	void* compositorThreadMethod(void);
	
	/* Constructors and destructors: */
	public:
	VRServer(const std::string& vrDeviceServerSocketName,bool vrDeviceServerSocketAbstract,int httpListenPortId,Vulkan::Instance& instance,const std::string& hmdName,double hmdFrameRate); // Creates a VR server for the HMD of the given name running at the given frame rate
	~VRServer(void); // Shuts down the server and releases all resources
	
	/* Methods: */
	void run(void); // Runs the server until interrupted
	void stop(void); // Stops the server
	bool didCrash(void) const // Returns true if the compositor crashed due to an unhandled exception
		{
		return compositorCrashed;
		}
	};

/*************************
Methods of class VRServer:
*************************/

void VRServer::stdioCallback(Threads::EventDispatcher::IOEvent& event)
	{
	/* Read everything available on stdin: */
	char buffer[1024];
	ssize_t readResult=read(STDIN_FILENO,buffer,sizeof(buffer));
	if(readResult>0)
		{
		/* Handle all read keypresses: */
		char* bufEnd=buffer+readResult;
		for(char* bufPtr=buffer;bufPtr!=bufEnd;++bufPtr)
			{
			switch(*bufPtr)
				{
				case 'Q':
				case 'q':
					/* Shut down the server: */
					dispatcher.stop();
					std::cout<<std::endl;
					break;
				
				case 'r':
					/* Toggle reprojection: */
					compositor.toggleReprojection();
					break;
				
				case 'a':
					/* Decrease exposure offset: */
					compositor.adjustExposeOffset(-1000000L); // 1ms less
					break;
				
				case 'd':
					/* Increase exposure offset: */
					compositor.adjustExposeOffset(1000000L); // 1ms more
					break;
				
				// DEBUGGING
				case 'p':
					compositor.pause(34000);
					break;
				}
			}
		}
	else if(readResult==0)
		{
		/* Stop listening on stdin: */
		event.removeListener();
		}
	}

void VRServer::listenSocketCallback(Threads::EventDispatcher::IOEvent& event)
	{
	Comm::UNIXPipe* tempPipe=0;
	try
		{
		/* Temporarily accept the connection: */
		tempPipe=new Comm::UNIXPipe(listenSocket);
		
		/* Check that there isn't already a connected client: */
		if(clientPipe==0)
			{
			/* Accept the connection: */
			std::cout<<"Accepting new client connection"<<std::endl;
			
			/* Send compositing information to the client: */
			tempPipe->writeFd(compositor.getSharedMemoryBlockFd());
			tempPipe->writeFd(compositor.getInputImageBlockFd());
			tempPipe->write(Vrui::VRCompositorProtocol::protocolVersion);
			tempPipe->write(size_t(compositor.getInputImageBlockSize()));
			for(unsigned int i=0;i<3;++i)
				tempPipe->write(size_t(compositor.getInputImageMemSize(i)));
			for(unsigned int i=0;i<3;++i)
				tempPipe->write(size_t(compositor.getInputImageMemOffset(i)));
			tempPipe->flush();
			
			/* Notify the compositor: */
			compositor.activate();
			
			/* Finalize the connection: */
			clientPipe=tempPipe;
			tempPipe=0;
			
			/* Start dispatching events from the client: */
			clientPipeListener=dispatcher.addIOEventListener(clientPipe->getFd(),Threads::EventDispatcher::Read,Threads::EventDispatcher::wrapMethod<VRServer,&VRServer::clientPipeCallback>,this);
			}
		else
			{
			/* Reject the connection to tell the client we're busy: */
			std::cout<<"Rejecting incoming client connection"<<std::endl;
			delete tempPipe;
			}
		}
	catch(const std::runtime_error& err)
		{
		/* Close the temporary connection and print an error message: */
		delete tempPipe;
		std::cout<<"Rejecting incoming client connection due to exception "<<err.what()<<std::endl;
		}
	}

void VRServer::httpListenSocketCallback(Threads::EventDispatcher::IOEvent& event)
	{
	try
		{
		/* Open a new TCP connection to the HTTP client: */
		Comm::PipePtr pipe(httpListenSocket->accept());
		
		/* Parse an HTTP POST request: */
		Comm::HttpPostRequest request(*pipe);
		const Comm::HttpPostRequest::NameValueList& nvl=request.getNameValueList();
		
		/* Check that there is a command in the POST request: */
		if(request.getActionUrl()=="/VRCompositingServer.cgi"&&nvl.size()>=1&&nvl.front().name=="command")
			{
			/* Compose the server's reply as a JSON-encoded object: */
			IO::JsonObjectPointer replyRoot=new IO::JsonObject;
			replyRoot->setProperty("command",nvl.front().value);
			
			/* Process the command: */
			if(nvl.front().value=="getServerStatus")
				{
				/* Compose the JSON object representing the current server state: */
				// Let's do that later
				
				replyRoot->setProperty("status","Success");
				}
			else
				replyRoot->setProperty("status","Invalid command");
			
			/* Send the server's reply as a json file embedded in an HTTP reply: */
			IO::OStream reply(pipe);
			reply<<"HTTP/1.1 200 OK\n";
			reply<<"Content-Type: application/json\n";
			reply<<"Access-Control-Allow-Origin: *\n";
			reply<<"\n";
			reply<<*replyRoot<<std::endl;
			
			/* Send the reply: */
			pipe->flush();
			}
		}
	catch(const std::runtime_error& err)
		{
		#if 0 // No, actually, don't :)
		/* Print an error message and carry on: */
		std::cout<<"Ignoring HTTP request due to exception "<<err.what()<<std::endl;
		#endif
		}
	}

void VRServer::clientPipeCallback(Threads::EventDispatcher::IOEvent& event)
	{
	/* Bail out if the client pipe has already been closed: */
	if(clientPipe==0)
		return;
	
	/* Read data and close the client connection if the client hung up: */
	try
		{
		/* Read and ignore anything the client sent: */
		char data[1024];
		clientPipe->readUpTo(data,sizeof(data));
		}
	catch(const std::runtime_error& err)
		{
		std::cout<<"Client closed connection"<<std::endl;
		
		/* Close the client pipe: */
		delete clientPipe;
		clientPipe=0;
		
		/* Remove this callback: */
		event.removeListener();
	
		/* Notify the compositor: */
		compositor.deactivate();
		}
	}

void VRServer::vsyncSignalCallback(Threads::EventDispatcher::SignalEvent& event)
	{
	/* Bail out if there is no client connected: */
	if(clientPipe==0)
		return;
	
	/* Send a vsync signal to the connected client: */
	try
		{
		clientPipe->write<unsigned char>(0);
		clientPipe->flush();
		}
	catch(const std::runtime_error& err)
		{
		/* Disconnect the client: */
		std::cout<<"Closing client connection due to exception "<<err.what()<<std::endl;
		
		/* Close the client pipe: */
		clientPipe->discard();
		delete clientPipe;
		clientPipe=0;
		
		/* Remove the client pipe callback: */
		dispatcher.removeIOEventListener(clientPipeListener);
	
		/* Notify the compositor: */
		compositor.deactivate();
		}
	}

void* VRServer::compositorThreadMethod(void)
	{
	try
		{
		/* Run the compositor's main loop: */
		compositor.run(vsyncSignalListener);
		}
	catch(const std::runtime_error& err)
		{
		std::cout<<"Shutting down compositor due to exception "<<err.what()<<std::endl;
		compositorCrashed=true;
		dispatcher.stop();
		}
	
	return 0;
	}

VRServer::VRServer(const std::string& vrDeviceServerSocketName,bool vrDeviceServerSocketAbstract,int httpListenPortId,Vulkan::Instance& instance,const std::string& hmdName,double hmdFrameRate)
	:vrDeviceClient(dispatcher,vrDeviceServerSocketName.c_str(),vrDeviceServerSocketAbstract),
	 compositor(dispatcher,vrDeviceClient,instance,hmdName,hmdFrameRate),compositorCrashed(false),
	 listenSocket(VRSERVER_SOCKET_NAME,5,VRSERVER_SOCKET_ABSTRACT),
	 clientPipe(0)
	{
	/* Set up the event dispatcher: */
	stdioListener=dispatcher.addIOEventListener(STDIN_FILENO,Threads::EventDispatcher::Read,Threads::EventDispatcher::wrapMethod<VRServer,&VRServer::stdioCallback>,this);
	listenSocketListener=dispatcher.addIOEventListener(listenSocket.getFd(),Threads::EventDispatcher::Read,Threads::EventDispatcher::wrapMethod<VRServer,&VRServer::listenSocketCallback>,this);
	vsyncSignalListener=dispatcher.addSignalListener(Threads::EventDispatcher::wrapMethod<VRServer,&VRServer::vsyncSignalCallback>,this);
	
	/* Check if we should listen for HTTP POST requests: */
	if(httpListenPortId>=0)
		{
		/* Create the HTTP socket and start listening on it: */
		httpListenSocket=new Comm::ListeningTCPSocket(httpListenPortId,5);
		httpListenSocketListener=dispatcher.addIOEventListener(httpListenSocket->getFd(),Threads::EventDispatcher::Read,Threads::EventDispatcher::wrapMethod<VRServer,&VRServer::httpListenSocketCallback>,this);
		}
	
	/* Ignore SIGPIPE and leave handling of pipe errors to TCP sockets: */
	Comm::ignorePipeSignals();
	
	/* Stop the launcher when a signal is received: */
	dispatcher.stopOnSignals();
	
	/* Start running the VR compositor in a background thread: */
	compositorThread.start(this,&VRServer::compositorThreadMethod);
	}

VRServer::~VRServer(void)
	{
	/* Shut down the VR compositor: */
	compositor.shutdown();
	compositorThread.join();
	
	/* Close a potentially remaining client connection: */
	delete clientPipe;
	}

void VRServer::run(void)
	{
	/* Dispatch events: */
	dispatcher.dispatchEvents();
	}

void VRServer::stop(void)
	{
	/* Stop the dispatcher to shut down the server: */
	dispatcher.stop();
	}

/****************
Main entry point:
****************/

int main(int argc,char* argv[])
	{
	/* Parse the command line: */
	Misc::CommandLineParser cmdLine;
	cmdLine.setDescription("Server to control the display of a VR head-mounted display and compose and reproject views rendered by client VR applications.");
	bool debug=false;
	cmdLine.addEnableOption("debug","d",debug,"Enables debugging mode on the Vulkan 3D graphics API.");
	bool listDisplays=false;
	cmdLine.addEnableOption("listDisplays","ld",listDisplays,"Lists all Vulkan displays and their video modes.");
	std::string deviceDaemonSocketName(VRDEVICEDAEMON_SOCKET_NAME);
	cmdLine.addValueOption("socket","s",deviceDaemonSocketName,"<UNIX socket name>","Sets the name of the VRDeviceDaemon's UNIX socket.");
	bool deviceDaemonSocketAbstract=VRDEVICEDAEMON_SOCKET_ABSTRACT;
	cmdLine.addEnableOption("abstract","a",deviceDaemonSocketAbstract,"Puts the VRDeviceDaemon's socket name in the abstract namespace.");
	cmdLine.addDisableOption("concrete","c",deviceDaemonSocketAbstract,"Puts the VRDeviceDaemon's socket name in the concrete namespace.");
	int httpListenPortId=-1;
	cmdLine.addValueOption("httpPort","p",httpListenPortId,"<TCP port number>","Sets the port of the TCP socket on which the VR compositor listens for HTTP POST requests.");
	std::string hmdName(VRSERVER_DEFAULT_HMD);
	cmdLine.addValueOption("hmd","hmd",hmdName,"<Vulkan display name>","Sets the name of the VR HMD / direct-mode display to be controlled.");
	double hmdFrameRate=VRSERVER_DEFAULT_HZ;
	cmdLine.addValueOption("frameRate","frameRate",hmdFrameRate,"<frame rate in Hz>","Sets the frame rate of the VR HMD / direct-mode display.");
	try
		{
		cmdLine.parse(argv,argv+argc);
		}
	catch(const std::runtime_error& err)
		{
		std::cerr<<"VRCompositingServer: "<<err.what()<<std::endl;
		return 1;
		}
	if(cmdLine.hadHelp())
		return 0;
	
	/* Disable line buffering on stdin: */
	struct termios originalTerm;
	tcgetattr(STDIN_FILENO,&originalTerm);
	struct termios term=originalTerm;
	term.c_lflag&=~ICANON;
	tcsetattr(STDIN_FILENO,TCSANOW,&term);
	
	int result=0;
	
	try
		{
		/* Create an info structure for this application: */
		Vulkan::ApplicationInfo appInfo(VRSERVER_APPNAME,VRSERVER_APPVERSION,VRSERVER_ENGINENAME,VRSERVER_ENGINEVERSION);
		
		/* Create a Vulkan instance: */
		Vulkan::CStringList instanceExtensions=VRCompositor::getInstanceExtensions();
		Vulkan::CStringList validationLayers;
		if(debug)
			validationLayers.push_back("VK_LAYER_KHRONOS_validation");
		Vulkan::Instance instance(appInfo,instanceExtensions,validationLayers);
		
		/* Create a Vulkan debug messenger if requested: */
		Misc::SelfDestructPointer<Vulkan::DebugUtilsMessenger> debugUtilsMessenger;
		if(debug)
			debugUtilsMessenger.setTarget(new Vulkan::DebugUtilsMessenger(instance));
		
		if(listDisplays)
			{
			/* Collect the list of all connected displays: */
			std::cout<<"Connected displays:"<<std::endl;
			std::vector<Vulkan::PhysicalDevice> physicalDevices=instance.getPhysicalDevices();
			for(std::vector<Vulkan::PhysicalDevice>::const_iterator pdIt=physicalDevices.begin();pdIt!=physicalDevices.end();++pdIt)
				{
				std::vector<VkDisplayPropertiesKHR> displayProperties=pdIt->getDisplayProperties();
				for(std::vector<VkDisplayPropertiesKHR>::iterator dpIt=displayProperties.begin();dpIt!=displayProperties.end();++dpIt)
					{
					/* Print display's basic information: */
					std::cout<<std::endl<<dpIt->displayName<<':'<<std::endl;
					VkExtent2D& pd=dpIt->physicalDimensions;
					std::cout<<"\tSize "<<pd.width<<"mm x "<<pd.height<<"mm"<<std::endl;
					VkExtent2D& pr=dpIt->physicalResolution;
					std::cout<<"\tPixel count "<<pr.width<<" x "<<pr.height<<std::endl;
					std::cout<<"\tResolution "<<double(pr.width)*25.4/double(pd.width)<<" dpi x "<<double(pr.height)*25.4/double(pd.height)<<" dpi"<<std::endl;
					
					/* Print display's modes: */
					std::cout<<"\tDisplay modes:"<<std::endl;
					std::vector<VkDisplayModePropertiesKHR> displayModeProperties=pdIt->getDisplayModeProperties(dpIt->display);
					for(std::vector<VkDisplayModePropertiesKHR>::iterator dmpIt=displayModeProperties.begin();dmpIt!=displayModeProperties.end();++dmpIt)
						{
						const VkDisplayModeParametersKHR& dmp=dmpIt->parameters;
						std::cout<<"\t\t"<<dmpIt->displayMode<<": "<<dmp.visibleRegion.width<<" x "<<dmp.visibleRegion.height<<" @ "<<double(dmp.refreshRate)/1000.0<<" Hz"<<std::endl;
						}
					}
				}
			}
		else
			{
			/* Create a VR server object: */
			VRServer server(deviceDaemonSocketName,deviceDaemonSocketAbstract,httpListenPortId,instance,hmdName,hmdFrameRate);
			
			/* Run the server's main loop until interrupted: */
			std::cout<<"Running server main loop"<<std::endl;
			server.run();
			std::cout<<"Server main loop exited"<<std::endl;
			result=server.didCrash()?1:0;
			}
		}
	catch(const std::runtime_error& err)
		{
		std::cerr<<"Caught exception "<<err.what()<<std::endl;
		result=1;
		}
	
	/* Restore original terminal state: */
	tcsetattr(STDIN_FILENO,TCSANOW,&originalTerm);
	
	return result;
	}
