/***********************************************************************
VRDeviceDaemon - Daemon for distributed VR device driver architecture.
Copyright (c) 2002-2026 Oliver Kreylos

This file is part of the Vrui VR Device Driver Daemon (VRDeviceDaemon).

The Vrui VR Device Driver Daemon is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Vrui VR Device Driver Daemon is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vrui VR Device Driver Daemon; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <Misc/FileNameExtensions.h>
#include <Misc/CommandLineParser.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Threads/MutexCond.h>
#include <Threads/EventDispatcher.h>
#include <Comm/Pipe.h>
#include <Vrui/Internal/Config.h>

#include <VRDeviceDaemon/Config.h>
#include <VRDeviceDaemon/VRDeviceManager.h>
#include <VRDeviceDaemon/VRDeviceServer.h>

VRDeviceServer* deviceServer=0;
bool shutdown=false;

void signalHandler(int signalId)
	{
	switch(signalId)
		{
		case SIGHUP:
			/* Restart server: */
			shutdown=false;
			deviceServer->stop();
			
			break;
		
		case SIGINT:
		case SIGTERM:
			/* Shut down server: */
			shutdown=true;
			deviceServer->stop();
			
			break;
		}
	
	return;
	}

int main(int argc,char* argv[])
	{
	/* Parse the command line: */
	Misc::CommandLineParser cmdLine;
	cmdLine.setDescription("Device driver and tracking server for a variety of VR-related input device types.");
	bool daemonize=false;
	cmdLine.addEnableOption("daemonize","D",daemonize,"Turn the server into a daemon after start-up.");
	std::string pidFileName="/var/run/VRDeviceDaemon.pid";
	cmdLine.addValueOption("pidFile","pf",pidFileName,"<path>","Path to the file where to store the VRDeviceDaemon's PID when daemonized.");
	std::string logFileName="/var/log/VRDeviceDaemon.log";
	cmdLine.addValueOption("logFile","lf",logFileName,"<path>","Path to the file to which to redirect the VRDeviceDaemon's output when daemonized.");
	std::string rootSectionName;
	cmdLine.addValueOption(0,"rootSection",rootSectionName,"<section name>","Sets the name of the configuration space's root section from which to read configuration data.");
	std::vector<std::string> mergeConfigFileNames;
	cmdLine.addListOption(0,"mergeConfig",mergeConfigFileNames,"<config file name>","Adds a the name of a configuration file to merge into the configuration space.");
	int httpListenPortId=-1;
	cmdLine.addValueOption("httpPort","p",httpListenPortId,"<TCP port number>","Sets the port of the TCP socket on which to listen for HTTP POST requests.");
	std::string configFileName(VRDEVICEDAEMON_CONFIG_CONFIGFILEDIR "/" VRDEVICEDAEMON_CONFIG_CONFIGFILENAME VRUI_INTERNAL_CONFIG_CONFIGFILESUFFIX);
	cmdLine.setArguments("[ <config file name> ]","Sets the name of the configuration file that forms the basis of the configuration space.");
	cmdLine.stopOnArguments();
	try
		{
		char** argPtr=argv;
		while(cmdLine.parse(argPtr,argv+argc))
			{
			/* Remember the configuration file name, then fail on additional arguments: */
			configFileName=*argPtr;
			++argPtr;
			cmdLine.failOnArguments();
			}
		}
	catch(const std::runtime_error& err)
		{
		std::cerr<<"VRDeviceDaemon: "<<err.what()<<std::endl;
		return 1;
		}
	if(cmdLine.hadHelp())
		return 0;
	
	if(daemonize)
		{
		/***************************
		Daemonize the device daemon:
		***************************/
		
		/* Fork once (and exit) to notify shell or caller that program is done: */
		int childPid=fork();
		if(childPid<0)
			{
			std::cerr<<"VRDeviceDaemon: Error during fork"<<std::endl<<std::flush;
			return 1; // Fork error
			}
		else if(childPid>0)
			{
			/* Save daemon's process ID to pid file: */
			int pidFd=open(pidFileName.c_str(),O_RDWR|O_CREAT|O_TRUNC,S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH);
			if(pidFd>=0)
				{
				/* Write process ID: */
				char pidBuffer[20];
				snprintf(pidBuffer,sizeof(pidBuffer),"%d\n",childPid);
				size_t pidLen=strlen(pidBuffer);
				if(write(pidFd,pidBuffer,pidLen)!=ssize_t(pidLen))
					std::cerr<<"VRDeviceDaemon: Could not write PID to file "<<pidFileName<<std::endl;
				close(pidFd);
				}
			return 0; // Parent process exits
			}
		
		/* Set new session ID to become independent process without controlling tty: */
		setsid();
		
		/* Close all inherited file descriptors: */
		for(int i=getdtablesize()-1;i>=0;--i)
			close(i);
		
		/* Redirect stdin, stdout and stderr to log file (this is ugly, but works because descriptors are assigned sequentially): */
		int nullFd=open("/dev/null",O_RDONLY);
		int logFd=open(logFileName.c_str(),O_RDWR|O_CREAT|O_TRUNC,S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH);
		if(nullFd!=0||logFd!=1||dup(logFd)!=2)
			std::cerr<<"VRDeviceDaemon: Error while rerouting output to log file"<<std::endl;
		
		/* Ignore most signals: */
		struct sigaction sigIgnore;
		sigIgnore.sa_handler=SIG_IGN;
		sigemptyset(&sigIgnore.sa_mask);
		sigIgnore.sa_flags=0x0;
		sigaction(SIGCHLD,&sigIgnore,0);
		sigaction(SIGTSTP,&sigIgnore,0);
		sigaction(SIGTTOU,&sigIgnore,0);
		sigaction(SIGTTIN,&sigIgnore,0);
		}
	
	/* Ignore SIGPIPE and leave handling of pipe errors to TCP sockets: */
	Comm::ignorePipeSignals();
	
	/* Install signal handler for SIGHUP to re-start the daemon: */
	struct sigaction sigHupAction;
	sigHupAction.sa_handler=signalHandler;
	sigemptyset(&sigHupAction.sa_mask);
	sigHupAction.sa_flags=0x0;
	sigaction(SIGHUP,&sigHupAction,0);
	
	/* Install signal handlers for SIGINT and SIGTERM to exit cleanly: */
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
	
	/* Create a shared event dispatcher: */
	Threads::EventDispatcher dispatcher;
	
	while(true)
		{
		/* Open configuration file: */
		#ifdef VERBOSE
		std::cout<<"VRDeviceDaemon: Reading configuration file "<<configFileName<<std::endl;
		#endif
		Misc::ConfigurationFile* configFile=0;
		try
			{
			configFile=new Misc::ConfigurationFile(configFileName.c_str());
			}
		catch(const std::runtime_error& err)
			{
			std::cerr<<"VRDeviceDaemon: Caught exception "<<err.what()<<" while reading configuration file "<<configFileName<<std::endl;
			return 1;
			}
		
		/* Merge additional requested configuration files: */
		for(std::vector<std::string>::iterator mcfnIt=mergeConfigFileNames.begin();mcfnIt!=mergeConfigFileNames.end();++mcfnIt)
			{
			/* Merge the configuration file: */
			#ifdef VERBOSE
			std::cout<<"VRDeviceDaemon: Merging configuration file "<<*mcfnIt<<std::endl;
			#endif
			try
				{
				/* Normalize the configuration file name: */
				std::string cfName;
				if(mcfnIt->empty()||(*mcfnIt)[0]!='/')
					{
					/* Anchor non-absolute paths to the configuration directory: */
					cfName=VRDEVICEDAEMON_CONFIG_CONFIGFILEDIR "/";
					}
				cfName.append(*mcfnIt);
				if(Misc::hasExtension(mcfnIt->c_str(),""))
					{
					/* Give names without an extension the default extension: */
					cfName.append(VRUI_INTERNAL_CONFIG_CONFIGFILESUFFIX);
					}
				
				configFile->merge(cfName.c_str());
				}
			catch(const std::runtime_error& err)
				{
				std::cerr<<"VRDeviceDaemon: Caught exception "<<err.what()<<" while merging configuration file "<<*mcfnIt<<std::endl;
				return 1;
				}
			}
		
		/* Set current section to given root section or name of current machine, or fall back to "localhost": */
		if(rootSectionName.empty()&&getenv("HOSTNAME")!=0)
			rootSectionName=getenv("HOSTNAME");
		if(rootSectionName.empty()&&getenv("HOST")!=0)
			rootSectionName=getenv("HOST");
		if(rootSectionName.empty())
			rootSectionName="localhost";
		configFile->setCurrentSection(rootSectionName.c_str());
		#ifdef VERBOSE
		std::cout<<"VRDeviceDaemon: Configuring from root section "<<rootSectionName<<std::endl<<std::flush;
		#endif
		
		/* Initialize devices: */
		#ifdef VERBOSE
		std::cout<<"VRDeviceDaemon: Initializing device manager"<<std::endl<<std::flush;
		#endif
		VRDeviceManager* deviceManager=0;
		configFile->setCurrentSection("./DeviceManager");
		try
			{
			deviceManager=new VRDeviceManager(dispatcher,*configFile);
			}
		catch(const std::runtime_error& err)
			{
			std::cerr<<"VRDeviceDaemon: Caught exception "<<err.what()<<" while initializing VR devices"<<std::endl<<std::flush;
			delete configFile;
			return 1;
			}
		configFile->setCurrentSection("..");
		
		/* Initialize device server: */
		#ifdef VERBOSE
		std::cout<<"VRDeviceDaemon: Initializing device server"<<std::endl<<std::flush;
		#endif
		configFile->setCurrentSection("./DeviceServer");
		if(httpListenPortId>=0)
			{
			/* Override the HTTP listen port setting in the configuration file: */
			configFile->storeValue("./httpPort",httpListenPortId);
			}
		try
			{
			deviceServer=new VRDeviceServer(dispatcher,deviceManager,*configFile);
			}
		catch(const std::runtime_error& err)
			{
			std::cerr<<"VRDeviceDaemon: Caught exception "<<err.what()<<" while initializing VR device server"<<std::endl<<std::flush;
			delete configFile;
			delete deviceManager;
			return 1;
			}
		configFile->setCurrentSection("..");
		
		/* Go back to root section: */
		configFile->setCurrentSection("..");
		
		/* Run the server's main loop: */
		deviceServer->run();
		
		/* Clean up: */
		delete deviceManager;
		deviceManager=0;
		delete deviceServer;
		deviceServer=0;
		delete configFile;
		configFile=0;
		
		/* Shut down the device daemon if SIGINT or SIGTERM were caught: */
		if(!daemonize||shutdown)
			{
			#ifdef VERBOSE
			std::cout<<"VRDeviceDaemon: Shutting down daemon"<<std::endl<<std::flush;
			#endif
			break;
			}
		else
			{
			#ifdef VERBOSE
			std::cout<<"VRDeviceDaemon: Restarting daemon"<<std::endl<<std::flush;
			#endif
			}
		}
	
	/* Exit: */
	#if 0
	if(daemonize)
		{
		/* Close all file descriptors and remove the log file: */
		for(int i=getdtablesize()-1;i>=0;--i)
			close(i);
		// unlink(logFileName.c_str());
		}
	#endif
	
	return 0;
	}
