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
#include <Misc/StdError.h>
#include <Misc/FileNameExtensions.h>
#include <Misc/CommandLineParser.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Threads/FunctionCalls.h>
#include <Threads/RunLoop.h>
#include <Comm/Pipe.h>
#include <Vrui/Internal/Config.h>

#include <VRDeviceDaemon/Config.h>
#include <VRDeviceDaemon/VRDeviceManager.h>
#include <VRDeviceDaemon/VRDeviceServer.h>

void signalHandlerFunction(Threads::RunLoop::SignalHandler::Event& event,bool& flag)
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
		return EXIT_FAILURE;
		}
	if(cmdLine.hadHelp())
		return 0;
	
	/* Turn VRDeviceDaemon into a daemon if requested: */
	if(daemonize)
		{
		/* Fork once (and exit) to notify shell or caller that the program is done: */
		int childPid=fork();
		if(childPid<0)
			{
			std::cerr<<Misc::makeLibcErrMsg("VRDeviceDaemon",errno,"Cannot fork daemon")<<std::endl;
			return EXIT_FAILURE; // Fork error
			}
		else if(childPid>0)
			{
			/* Print the daemon's process ID: */
			#ifdef VERBOSE
			std::cout<<"VRDeviceDaemon: Started daemon with PID "<<childPid<<std::endl;
			#endif
			
			return 0; // Parent process exits
			}
		
		/* Set new session ID to become independent process without controlling tty: */
		setsid();
		
		try
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
					{
					close(pidFd);
					throw Misc::makeLibcErr(0,errno,"Cannot write daemon PID to file %s",pidFileName.c_str());
					}
				close(pidFd);
				}
			else
				throw Misc::makeLibcErr(0,errno,"Cannot create daemon PID file %s",pidFileName.c_str());
			
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
			
			/* Close all other open file descriptors: */
			int fdTableSize=int(sysconf(_SC_OPEN_MAX));
			for(int fd=3;fd<fdTableSize;++fd)
				{
				/* This will fail silently for ones that aren't actually open: */
				close(fd);
				}
			}
		catch(const std::runtime_error& err)
			{
			/* Write an error message to the original stderr and quit: */
			std::cerr<<"VRDeviceDaemon: Shutting down with exception "<<err.what()<<std::endl;
			unlink(pidFileName.c_str());
			return EXIT_FAILURE;
			}
		}
	
	int exitCode=0;
	try
		{
		/* Ignore SIGPIPE and leave handling of pipe errors to TCP sockets: */
		Comm::ignorePipeSignals();
		
		/* Create a run loop to dispatch events: */
		Threads::RunLoop runLoop;
		
		/* Install a handler for SIGHUP that restarts the server launcher (and reloads its configuration file), but keeps running: */
		bool dummy=false; // A dummy flag for the SIGHUP handler
		runLoop.createSignalHandler(SIGHUP,true,*Threads::createFunctionCall(signalHandlerFunction,dummy));
		
		/* Install handlers for SIGINT and SIGTERM that shut down the daemon: */
		bool keepRunning=true;
		Misc::Autopointer<Threads::RunLoop::SignalHandler::EventHandler> intTermHandler=Threads::createFunctionCall(signalHandlerFunction,keepRunning);
		runLoop.createSignalHandler(SIGINT,true,*intTermHandler);
		runLoop.createSignalHandler(SIGTERM,true,*intTermHandler);
		
		/* Run until shut down: */
		while(keepRunning)
			{
			/* Open configuration file: */
			#ifdef VERBOSE
			std::cout<<"VRDeviceDaemon: Reading configuration file "<<configFileName<<std::endl;
			#endif
			Misc::ConfigurationFile configFile(configFileName.c_str());
			
			/* Merge additional requested configuration files: */
			for(std::vector<std::string>::iterator mcfnIt=mergeConfigFileNames.begin();mcfnIt!=mergeConfigFileNames.end();++mcfnIt)
				{
				/* Merge the configuration file: */
				#ifdef VERBOSE
				std::cout<<"VRDeviceDaemon: Merging configuration file "<<*mcfnIt<<std::endl;
				#endif
				
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
				
				configFile.merge(cfName.c_str());
				}
			
			/* Set current section to given root section or name of current machine, or fall back to "localhost": */
			if(rootSectionName.empty()&&getenv("HOSTNAME")!=0)
				rootSectionName=getenv("HOSTNAME");
			if(rootSectionName.empty()&&getenv("HOST")!=0)
				rootSectionName=getenv("HOST");
			if(rootSectionName.empty())
				rootSectionName="localhost";
			#ifdef VERBOSE
			std::cout<<"VRDeviceDaemon: Configuring from root section "<<rootSectionName<<std::endl<<std::flush;
			#endif
			configFile.setCurrentSection(rootSectionName.c_str());
			
			/* Initialize devices: */
			#ifdef VERBOSE
			std::cout<<"VRDeviceDaemon: Initializing device manager"<<std::endl<<std::flush;
			#endif
			configFile.setCurrentSection("./DeviceManager");
			VRDeviceManager deviceManager(runLoop,configFile);
			configFile.setCurrentSection("..");
			
			/* Initialize device server: */
			#ifdef VERBOSE
			std::cout<<"VRDeviceDaemon: Initializing device server"<<std::endl<<std::flush;
			#endif
			configFile.setCurrentSection("./DeviceServer");
			if(httpListenPortId>=0)
				{
				/* Override the HTTP listen port setting in the configuration file: */
				configFile.storeValue("./httpPort",httpListenPortId);
				}
			VRDeviceServer deviceServer(runLoop,deviceManager,configFile);
			configFile.setCurrentSection("..");
			
			/* Go back to root section: */
			configFile.setCurrentSection("..");
			
			/* Run the server's main loop: */
			deviceServer.run();
			
			/* Shut down the device daemon if SIGINT or SIGTERM were caught: */
			if(!keepRunning)
				{
				#ifdef VERBOSE
				std::cout<<"VRDeviceDaemon: Shutting down daemon"<<std::endl<<std::flush;
				#endif
				}
			else
				{
				#ifdef VERBOSE
				std::cout<<"VRDeviceDaemon: Restarting daemon"<<std::endl<<std::flush;
				#endif
				}
			}
		}
	catch(const std::runtime_error& err)
		{
		std::cerr<<"VRDeviceDaemon: Shutting down with exception "<<err.what()<<std::endl;
		exitCode=EXIT_FAILURE;
		}
	
	/* If the server was daemonized, remove the pid file: */
	if(daemonize)
		unlink(pidFileName.c_str());
	
	return exitCode;
	}
