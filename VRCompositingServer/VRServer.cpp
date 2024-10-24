/***********************************************************************
VRServer - Prototype for a VR server offering compositing services to VR
application clients.
Copyright (c) 2022-2024 Oliver Kreylos

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
#include <signal.h>
#include <string>
#include <iostream>
#include <Misc/SizedTypes.h>
#include <Misc/SelfDestructPointer.h>
#include <Threads/Thread.h>
#include <Threads/EventDispatcher.h>
#include <Comm/Pipe.h>
#include <Comm/ListeningUNIXSocket.h>
#include <Comm/UNIXPipe.h>
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
	
	/* A UNIX socket and pipe to communicate with VR application clients: */
	Comm::ListeningUNIXSocket listenSocket; // UNIX socket listening for incoming client connections
	Comm::UNIXPipe* clientPipe; // UNIX pipe connected to the current client
	Threads::EventDispatcher::ListenerKey stdioListener; // Key for listener for standard input
	Threads::EventDispatcher::ListenerKey listenSocketListener; // Key for listener for listening UNIX socket
	Threads::EventDispatcher::ListenerKey clientPipeListener; // Key for listener for the current client pipe
	Threads::EventDispatcher::ListenerKey vsyncSignalListener; // Key for listener for vsync events from the compositor
	
	/* Private methods: */
	void stdioCallback(Threads::EventDispatcher::IOEvent& event);
	void listenSocketCallback(Threads::EventDispatcher::IOEvent& event);
	void clientPipeCallback(Threads::EventDispatcher::IOEvent& event);
	void vsyncSignalCallback(Threads::EventDispatcher::SignalEvent& event);
	void* compositorThreadMethod(void);
	
	/* Constructors and destructors: */
	public:
	VRServer(const std::string& vrDeviceServerSocketName,bool vrDeviceServerSocketAbstract,Vulkan::Instance& instance,const std::string& hmdName,double hmdFrameRate); // Creates a VR server for the HMD of the given name running at the given frame rate
	~VRServer(void); // Shuts down the server and releases all resources
	
	/* Methods: */
	void run(void); // Runs the server until interrupted
	void stop(void); // Stops the server
	};

/*************************
Methods of class VRServer:
*************************/

void VRServer::stdioCallback(Threads::EventDispatcher::IOEvent& event)
	{
	/* Read everything available on stdin: */
	char buffer[1024];
	ssize_t readResult=read(STDIN_FILENO,buffer,sizeof(buffer));
	if(readResult>=0)
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
	}

void VRServer::listenSocketCallback(Threads::EventDispatcher::IOEvent& event)
	{
	/* Check that there isn't already a connected client: */
	if(clientPipe==0)
		{
		/* Accept the connection: */
		std::cout<<"Accepting new client connection"<<std::endl;
		clientPipe=new Comm::UNIXPipe(listenSocket);
		
		/* Send compositing information to the client: */
		clientPipe->writeFd(compositor.getSharedMemoryBlockFd());
		clientPipe->writeFd(compositor.getInputImageBlockFd());
		clientPipe->write(Vrui::VRCompositorProtocol::protocolVersion);
		clientPipe->write(size_t(compositor.getInputImageBlockSize()));
		for(unsigned int i=0;i<3;++i)
			clientPipe->write(size_t(compositor.getInputImageMemSize(i)));
		for(unsigned int i=0;i<3;++i)
			clientPipe->write(size_t(compositor.getInputImageMemOffset(i)));
		clientPipe->flush();
		
		/* Start dispatching events from the client: */
		clientPipeListener=dispatcher.addIOEventListener(clientPipe->getFd(),Threads::EventDispatcher::Read,Threads::EventDispatcher::wrapMethod<VRServer,&VRServer::clientPipeCallback>,this);
		
		/* Notify the compositor: */
		compositor.activate();
		}
	else
		{
		/* Accept the connection and immediately close it again to tell the client we're busy: */
		std::cout<<"Rejecting incoming client connection"<<std::endl;
		Comm::UNIXPipe tempClientPipe(listenSocket);
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
	/* Run the compositor's main loop: */
	compositor.run(vsyncSignalListener);
	
	return 0;
	}

VRServer::VRServer(const std::string& vrDeviceServerSocketName,bool vrDeviceServerSocketAbstract,Vulkan::Instance& instance,const std::string& hmdName,double hmdFrameRate)
	:vrDeviceClient(dispatcher,vrDeviceServerSocketName.c_str(),vrDeviceServerSocketAbstract),
	 compositor(dispatcher,vrDeviceClient,instance,hmdName,hmdFrameRate),
	 listenSocket(VRSERVER_SOCKET_NAME,5,VRSERVER_SOCKET_ABSTRACT),
	 clientPipe(0)
	{
	/* Set up the event dispatcher: */
	stdioListener=dispatcher.addIOEventListener(STDIN_FILENO,Threads::EventDispatcher::Read,Threads::EventDispatcher::wrapMethod<VRServer,&VRServer::stdioCallback>,this);
	listenSocketListener=dispatcher.addIOEventListener(listenSocket.getFd(),Threads::EventDispatcher::Read,Threads::EventDispatcher::wrapMethod<VRServer,&VRServer::listenSocketCallback>,this);
	vsyncSignalListener=dispatcher.addSignalListener(Threads::EventDispatcher::wrapMethod<VRServer,&VRServer::vsyncSignalCallback>,this);
	
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

VRServer* serverPtr=0; // Pointer to the VR compositing server

void signalHandler(int signalId)
	{
	switch(signalId)
		{
		case SIGHUP:
		case SIGINT:
		case SIGTERM:
			/* Shut down the VR compositing server: */
			if(serverPtr!=0)
				serverPtr->stop();
			
			break;
		}
	
	return;
	}

int main(int argc,char* argv[])
	{
	/* Parse the command line: */
	bool debug=false;
	bool listDisplays=false;
	const char* deviceDaemonSocketName=VRDEVICEDAEMON_SOCKET_NAME;
	bool deviceDaemonSocketAbstract=VRDEVICEDAEMON_SOCKET_ABSTRACT;
	const char* hmdName=VRSERVER_DEFAULT_HMD;
	double hmdFrameRate=VRSERVER_DEFAULT_HZ;
	for(int argi=1;argi<argc;++argi)
		{
		if(argv[argi][0]=='-')
			{
			if(strcasecmp(argv[argi]+1,"debug")==0)
				debug=true;
			else if(strcasecmp(argv[argi]+1,"listDisplays")==0||strcasecmp(argv[argi]+1,"ld")==0)
				listDisplays=true;
			else if(strcasecmp(argv[argi]+1,"socket")==0)
				{
				++argi;
				deviceDaemonSocketName=argv[argi];
				}
			else if(strcasecmp(argv[argi]+1,"abstract")==0)
				deviceDaemonSocketAbstract=true;
			else if(strcasecmp(argv[argi]+1,"concrete")==0)
				deviceDaemonSocketAbstract=false;
			else if(strcasecmp(argv[argi]+1,"hmd")==0)
				{
				++argi;
				hmdName=argv[argi];
				}
			else if(strcasecmp(argv[argi]+1,"frameRate")==0)
				{
				++argi;
				hmdFrameRate=atof(argv[argi]);
				}
			}
		}
	
	/* Install signal handlers for SIGHUP, SIGINT, and SIGTERM to exit cleanly: */
	struct sigaction sigHupAction;
	sigHupAction.sa_handler=signalHandler;
	sigemptyset(&sigHupAction.sa_mask);
	sigHupAction.sa_flags=0x0;
	sigaction(SIGHUP,&sigHupAction,0);
	struct sigaction sigIntAction;
	sigIntAction.sa_handler=signalHandler;
	sigemptyset(&sigIntAction.sa_mask);
	sigIntAction.sa_flags=0x0;
	sigaction(SIGINT,&sigIntAction,0);
	struct sigaction sigTermAction;
	sigTermAction.sa_handler=signalHandler;
	sigemptyset(&sigTermAction.sa_mask);
	sigTermAction.sa_flags=0x0;
	sigaction(SIGTERM,&sigTermAction,0);
	
	/* Ignore SIGPIPE and leave handling of pipe errors to TCP sockets: */
	Comm::ignorePipeSignals();
	
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
			VRServer server(deviceDaemonSocketName,deviceDaemonSocketAbstract,instance,hmdName,hmdFrameRate);
			serverPtr=&server;
			
			/* Run the server's main loop until interrupted: */
			std::cout<<"Running server main loop"<<std::endl;
			server.run();
			std::cout<<"Server main loop exited"<<std::endl;
			
			serverPtr=0;
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
