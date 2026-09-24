/***********************************************************************
Environment-dependent part of Vrui virtual reality development toolkit.
Copyright (c) 2000-2026 Oliver Kreylos

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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <stdexcept>
#include <Misc/Utility.h>
#include <Misc/Size.h>
#include <Misc/StringPrintf.h>
#include <Misc/StdError.h>
#include <Misc/FdSet.h>
#include <Misc/File.h>
#include <Misc/Timer.h>
#include <Misc/StringMarshaller.h>
#include <Misc/GetCurrentDirectory.h>
#include <Misc/FileNameExtensions.h>
#include <Misc/FileTests.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Misc/ConfigurationFile.icpp>
#include <Realtime/Time.h>
#include <Threads/Thread.h>
#include <Threads/Barrier.h>
#include <Cluster/Multiplexer.h>
#include <Cluster/MulticastPipe.h>
#include <Cluster/ThreadSynchronizer.h>
#include <Cluster/Opener.h>
#include <Math/Constants.h>
#include <Geometry/Point.h>
#include <Geometry/Plane.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/AffineTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/Config.h>
#include <GL/GLValueCoders.h>
#include <GL/GLContextData.h>
#include <GLMotif/Event.h>
#include <GLMotif/Popup.h>
#include <AL/Config.h>
#include <AL/ALContextData.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/Internal/InputDeviceAdapterMouse.h>
#include <Vrui/Internal/InputDeviceAdapterMultitouch.h>
#include <Vrui/CoordinateManager.h>
#include <Vrui/VRWindow.h>
#include <Vrui/SoundContext.h>
#include <Vrui/ToolManager.h>
#include <Vrui/VisletManager.h>
#include <Vrui/ViewSpecification.h>
#include <Vrui/Application.h>

#include <Vrui/Internal/Vrui.h>
#include <Vrui/Internal/Config.h>
#include <Vrui/Internal/WindowGroup.h>

namespace Vrui {

/*****************************
Private Vrui global variables:
*****************************/

bool vruiVerbose=false;
bool vruiMaster=true;

std::ostream& operator<<(std::ostream& os,const VruiErrorHeader& veh)
	{
	/* Check if this is a cluster environment: */
	if(vruiState!=0&&vruiState->multiplexer!=0)
		os<<"Vrui: (node "<<vruiState->multiplexer->getNodeIndex()<<"): ";
	else
		os<<"Vrui: ";
	
	return os;
	}

VruiErrorHeader vruiErrorHeader;

namespace {

/***********************************
Workbench-specific global variables:
***********************************/

Threads::IOWatcherOwner vruiStdinWatcher;
Threads::IOWatcherOwner vruiCommandPipeWatcher;
int vruiCommandPipe=-1;
int vruiCommandPipeHolder=-1;
Misc::ConfigurationFile* vruiConfigFile=0;
char* vruiConfigRootSectionName=0;
char* vruiApplicationName=0;
int vruiNumWindows=0;
VRWindow** vruiWindows=0;
int vruiNumWindowGroups=0;
WindowGroup* vruiWindowGroups=0;
int vruiTotalNumWindows=0;
int vruiFirstLocalWindowIndex=0;
VRWindow** vruiTotalWindows=0;
bool vruiRenderInParallel=false;
Threads::Thread* vruiRenderingThreads=0;
Threads::Barrier vruiRenderingBarrier;
volatile bool vruiStopRenderingThreads=false;
int vruiNumSoundContexts=0;
SoundContext** vruiSoundContexts=0;
SoundContext* vruiRecordingSoundContext=0;
Cluster::Multiplexer* vruiMultiplexer=0;
Cluster::MulticastPipe* vruiPipe=0;
int vruiNumSlaves=0;
pid_t* vruiSlavePids=0;
int vruiSlaveArgc=0;
char** vruiSlaveArgv=0;
char** vruiSlaveArgvShadow=0;

/*****************************************
Workbench-specific private Vrui functions:
*****************************************/

/* Generic cleanup function called in case of an error: */
void vruiErrorShutdown(bool signalError)
	{
	if(signalError)
		{
		if(vruiMultiplexer!=0)
			{
			/* Signal a fatal error to all nodes and let them die: */
			//vruiMultiplexer->fatalError();
			}
		
		/* Return with an error condition: */
		exit(1);
		}
	
	/* Clean up: */
	vruiState->finishMainLoop();
	GLContextData::shutdownThingManager();
	if(vruiRenderingThreads!=0)
		{
		/* Shut down all rendering threads: */
		vruiStopRenderingThreads=true;
		vruiRenderingBarrier.synchronize();
		for(int i=0;i<vruiNumWindowGroups;++i)
			{
			// vruiRenderingThreads[i].cancel();
			vruiRenderingThreads[i].join();
			}
		delete[] vruiRenderingThreads;
		vruiRenderingThreads=0;
		}
	if(vruiWindows!=0)
		{
		/* Release all window groups' and window's OpenGL state: */
		for(int i=0;i<vruiNumWindowGroups;++i)
			vruiWindowGroups[i].releaseGLState();
		
		/* Delete all windows: */
		for(int i=0;i<vruiNumWindows;++i)
			delete vruiWindows[i];
		delete[] vruiWindows;
		vruiWindows=0;
		delete[] vruiWindowGroups;
		vruiWindowGroups=0;
		delete[] vruiTotalWindows;
		vruiTotalWindows=0;
		vruiTotalNumWindows=0;
		}
	ALContextData::shutdownThingManager();
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	if(vruiSoundContexts!=0)
		{
		/* Destroy all sound contexts: */
		for(int i=0;i<vruiNumSoundContexts;++i)
			delete vruiSoundContexts[i];
		delete[] vruiSoundContexts;
		}
	#endif
	delete[] vruiApplicationName;
	delete vruiState;
	
	if(vruiMultiplexer!=0)
		{
		bool master=vruiMultiplexer->isMaster();
		
		/* Unregister the multiplexer from the Cluster::Opener object: */
		Cluster::Opener::getOpener()->setMultiplexer(0);
		
		/* Destroy the multiplexer: */
		delete vruiPipe;
		delete vruiMultiplexer;
		
		if(master&&vruiSlavePids!=0)
			{
			/* Wait for all slaves to terminate: */
			for(int i=0;i<vruiNumSlaves;++i)
				waitpid(vruiSlavePids[i],0,0);
			delete[] vruiSlavePids;
			vruiSlavePids=0;
			}
		if(!master&&vruiSlaveArgv!=0)
			{
			/* Delete the slave command line: */
			for(int i=0;i<vruiSlaveArgc;++i)
				delete[] vruiSlaveArgv[i];
			delete[] vruiSlaveArgv;
			vruiSlaveArgv=0;
			delete[] vruiSlaveArgvShadow;
			vruiSlaveArgvShadow=0;
			}
		}
	
	/* Close the configuration file: */
	delete vruiConfigFile;
	delete[] vruiConfigRootSectionName;
	
	/* Close the command pipe: */
	if(vruiCommandPipe>=0)
		{
		close(vruiCommandPipeHolder);
		close(vruiCommandPipe);
		}
	}

int vruiXErrorHandler(Display* display,XErrorEvent* event)
	{
	/* X protocol errors are not considered fatal; log an error message and carry on: */
	std::cerr<<vruiErrorHeader<<"Caught X11 protocol error ";
	char errorString[257];
	XGetErrorText(display,event->error_code,errorString,sizeof(errorString));
	std::cerr<<errorString<<", seq# "<<event->serial<<", request "<<int(event->request_code)<<"."<<int(event->minor_code)<<std::endl;
	
	return 0;
	}

int vruiXIOErrorHandler(Display* display)
	{
	/* X I/O errors are considered fatal; shut down the Vrui application: */
	std::cerr<<vruiErrorHeader<<"Caught X11 I/O error; shutting down"<<std::endl;
	shutdown();
	
	return 0;
	}

bool vruiMergeConfigurationFile(const char* directory,const std::string& fileName)
	{
	/* Assemble the full configuration file name: */
	std::string configFileName;
	if(directory!=0)
		{
		configFileName.append(directory);
		configFileName.push_back('/');
		}
	configFileName.append(fileName);
	
	try
		{
		/* Merge in the given configuration file: */
		if(vruiVerbose&&vruiMaster)
			std::cout<<"Vrui: Merging configuration file "<<configFileName<<"..."<<std::flush;
		vruiConfigFile->merge(configFileName.c_str());
		if(vruiVerbose&&vruiMaster)
			std::cout<<" Ok"<<std::endl;
		
		return true;
		}
	catch(const Misc::File::OpenError& err)
		{
		/* Ignore the error and continue */
		if(vruiVerbose&&vruiMaster)
			std::cout<<" does not exist"<<std::endl;
		
		return false;
		}
	catch(const std::runtime_error& err)
		{
		/* Bail out on errors in user configuration file: */
		if(vruiVerbose&&vruiMaster)
			std::cout<<" error"<<std::endl;
		std::cerr<<vruiErrorHeader<<"Caught exception "<<err.what()<<" while merging configuration file "<<configFileName.c_str()<<std::endl;
		vruiErrorShutdown(true);
		
		return false;
		}
	}

void vruiMergeConfigurationFileLayered(const char* userConfigDir,const char* fileName)
	{
	/* Assemble the full name of the configuration file to merge: */
	std::string configFileName=fileName;
	
	/* Ensure that the configuration file name has the .cfg suffix: */
	if(!Misc::hasExtension(fileName,VRUI_INTERNAL_CONFIG_CONFIGFILESUFFIX))
		configFileName.append(VRUI_INTERNAL_CONFIG_CONFIGFILESUFFIX);
	
	bool foundConfigFile=false;
	
	/* Check if the configuration file name is a relative path: */
	if(fileName[0]!='/')
		{
		/* Try loading the configuration file from the global systemwide configuration directory: */
		foundConfigFile=vruiMergeConfigurationFile(VRUI_INTERNAL_CONFIG_SYSCONFIGDIR,configFileName)||foundConfigFile;
		
		if(userConfigDir!=0)
			{
			/* Try loading the configuration file from the global per-user configuration directory: */
			foundConfigFile=vruiMergeConfigurationFile(userConfigDir,configFileName)||foundConfigFile;
			}
		}
	
	/* Try loading the configuration file as given: */
	foundConfigFile=vruiMergeConfigurationFile(0,configFileName)||foundConfigFile;
	
	/* Check if at least one configuration file was merged: */
	if(!foundConfigFile)
		std::cerr<<"Vrui::init: Requested configuration file "<<fileName<<" not found"<<std::endl;
	}

void vruiOpenConfigurationFile(const char* userConfigDir,const char* appPath,const std::vector<std::string>& earlyMerges)
	{
	/* Create the configuration file name: */
	std::string configFileName(VRUI_INTERNAL_CONFIG_CONFIGFILENAME);
	configFileName.append(VRUI_INTERNAL_CONFIG_CONFIGFILESUFFIX);
	
	try
		{
		/* Open the system-wide configuration file: */
		std::string systemConfigFileName(VRUI_INTERNAL_CONFIG_SYSCONFIGDIR);
		systemConfigFileName.push_back('/');
		systemConfigFileName.append(configFileName);
		if(vruiVerbose&&vruiMaster)
			std::cout<<"Vrui: Reading system-wide configuration file "<<systemConfigFileName<<std::endl;
		vruiConfigFile=new Misc::ConfigurationFile(systemConfigFileName.c_str());
		}
	catch(const std::runtime_error& err)
		{
		/* Bail out: */
		std::cerr<<vruiErrorHeader<<"Caught exception "<<err.what()<<" while reading system-wide configuration file "<<VRUI_INTERNAL_CONFIG_SYSCONFIGDIR<<'/'<<configFileName<<std::endl;
		vruiErrorShutdown(true);
		}
	
	/* Merge the global per-user configuration file if given: */
	if(userConfigDir!=0)
		vruiMergeConfigurationFile(userConfigDir,configFileName);
	
	/* Merge all requested early merges: */
	for(std::vector<std::string>::const_iterator emIt=earlyMerges.begin();emIt!=earlyMerges.end();++emIt)
		vruiMergeConfigurationFileLayered(userConfigDir,emIt->c_str());
	
	/* Create the app-specific configuration file name: */
	std::string appConfigFileName(vruiApplicationName);
	appConfigFileName.append(VRUI_INTERNAL_CONFIG_CONFIGFILESUFFIX);
	
	/* Merge the system-wide per-application configuration file if it exists: */
	std::string systemAppConfigDir(VRUI_INTERNAL_CONFIG_SYSCONFIGDIR);
	systemAppConfigDir.push_back('/');
	systemAppConfigDir.append(VRUI_INTERNAL_CONFIG_APPCONFIGDIR);
	vruiMergeConfigurationFile(systemAppConfigDir.c_str(),appConfigFileName);
	
	/* Merge the global per-user per-application configuration file if given: */
	if(userConfigDir!=0)
		{
		std::string userAppConfigDir(userConfigDir);
		userAppConfigDir.push_back('/');
		userAppConfigDir.append(VRUI_INTERNAL_CONFIG_APPCONFIGDIR);
		vruiMergeConfigurationFile(userAppConfigDir.c_str(),appConfigFileName);
		}
	
	/* Get the name of the local configuration file: */
	const char* localConfigFileName=getenv("VRUI_CONFIGFILE");
	if(localConfigFileName!=0&&localConfigFileName[0]!='\0')
		{
		/* Merge the local configuration file: */
		vruiMergeConfigurationFile(0,localConfigFileName);
		}
	else
		{
		/* Merge the default local configuration file: */
		vruiMergeConfigurationFile(".",configFileName);
		}
	}

void vruiGoToRootSection(const char*& rootSectionName,bool verbose)
	{
	try
		{
		/* Fall back to simulator mode if the root section does not exist: */
		bool rootSectionFound=false;
		if(rootSectionName==0)
			rootSectionName=VRUI_INTERNAL_CONFIG_DEFAULTROOTSECTION;
		Misc::ConfigurationFile::SectionIterator rootIt=vruiConfigFile->getRootSection().getSection("/Vrui");
		for(Misc::ConfigurationFile::SectionIterator sIt=rootIt.beginSubsections();sIt!=rootIt.endSubsections();++sIt)
			if(sIt.getName()==rootSectionName)
				{
				rootSectionFound=true;
				break;
				}
		if(!rootSectionFound)
			{
			if(verbose&&vruiMaster)
				std::cout<<"Vrui: Requested root section /Vrui/"<<rootSectionName<<" does not exist"<<std::endl;
			rootSectionName=VRUI_INTERNAL_CONFIG_DEFAULTROOTSECTION;
			}
		}
	catch(...)
		{
		/* Bail out if configuration file does not contain the Vrui section: */
		std::cerr<<"Vrui: Configuration file does not contain /Vrui section"<<std::endl;
		vruiErrorShutdown(true);
		}
	
	/* Go to the given root section: */
	if(verbose&&vruiMaster)
		std::cout<<"Vrui: Going to root section /Vrui/"<<rootSectionName<<std::endl;
	vruiConfigFile->setCurrentSection("/Vrui");
	vruiConfigFile->setCurrentSection(rootSectionName);
	}

void* vruiRenderingThreadFunction(int windowGroupIndex)
	{
	Threads::Thread::setCancelState(Threads::Thread::CANCEL_ENABLE);
	// Threads::Thread::setCancelType(Threads::Thread::CANCEL_ASYNCHRONOUS);
	
	if(vruiVerbose)
		std::cout<<"Vrui: Started rendering thread for window group "<<windowGroupIndex<<std::endl;
	
	/* Keep track of how many rendering barriers this thread still has to pass, in order to clean up properly should an exception occur: */
	int numBarriers=0;
	
	try
		{
		/* Synchronize all rendering threads: */
		vruiRenderingBarrier.synchronize();
		
		WindowGroup& windowGroup=vruiWindowGroups[windowGroupIndex];
		
		/* Enter the rendering loop and redraw all windows until interrupted: */
		while(true)
			{
			/* Wait for the start of the rendering cycle: */
			vruiRenderingBarrier.synchronize();
			
			/* Check for shutdown: */
			if(vruiStopRenderingThreads)
				break;
			
			numBarriers=3;
			
			/* Draw the window group and wail until all windows are done rendering: */
			windowGroup.draw();
			windowGroup.wait();
			
			/* Wait until all other threads are done rendering: */
			vruiRenderingBarrier.synchronize();
			
			if(vruiState->multiplexer)
				{
				/* Wait until all other nodes are done rendering: */
				vruiRenderingBarrier.synchronize();
				}
			
			numBarriers=1;
			
			/* Present all windows' rendering results: */
			windowGroup.present();
			
			/* Wait until all threads are done presenting rendering results: */
			vruiRenderingBarrier.synchronize();
			}
		}
	catch(const std::runtime_error& err)
		{
		/* Print an error message: */
		std::cout<<"Vrui: Shutting down rendering thread for window group "<<windowGroupIndex<<" due to exception "<<err.what()<<std::endl;
		
		/* Re-synchronize with the other rendering threads: */
		if(numBarriers>=3)
			{
			/* Wait until all other threads are done rendering: */
			vruiRenderingBarrier.synchronize();
			
			if(vruiState->multiplexer)
				{
				/* Wait until all other nodes are done rendering: */
				vruiRenderingBarrier.synchronize();
				}
			}
		if(numBarriers>=1)
			{
			/* Wait until all threads are done presenting rendering results: */
			vruiRenderingBarrier.synchronize();
			}
		
		/* Keep pretending to render until the application shuts down: */
		while(true)
			{
			/* Wait for the start of the rendering cycle: */
			vruiRenderingBarrier.synchronize();
			
			/* Check for shutdown: */
			if(vruiStopRenderingThreads)
				break;
			
			/* Wait until all other threads are done rendering: */
			vruiRenderingBarrier.synchronize();
			
			if(vruiState->multiplexer)
				{
				/* Wait until all other nodes are done rendering: */
				vruiRenderingBarrier.synchronize();
				}
			
			/* Wait until all threads are done presenting rendering results: */
			vruiRenderingBarrier.synchronize();
			}
		}
	
	if(vruiVerbose)
		std::cout<<"Vrui: Shutting down rendering thread for window group "<<windowGroupIndex<<std::endl;
	
	return 0;
	}

}

/**********************************
Call-in functions for user program:
**********************************/

void init(int& argc,char**& argv,char**&)
	{
	typedef std::vector<std::string> StringList;
	
	/* Determine whether this node is the master or a slave: */
	if(argc==8&&strcmp(argv[1],"-vruiMultipipeSlave")==0)
		{
		/********************
		This is a slave node:
		********************/
		
		vruiMaster=false;
		
		/* Read multipipe settings from the command line: */
		int numSlaves=atoi(argv[2]);
		int nodeIndex=atoi(argv[3]);
		char* master=argv[4];
		int masterPort=atoi(argv[5]);
		char* multicastGroup=argv[6];
		int multicastPort=atoi(argv[7]);
		
		/* Connect back to the master: */
		try
			{
			/* Create the multicast multiplexer: */
			vruiMultiplexer=new Cluster::Multiplexer(numSlaves,nodeIndex,master,masterPort,multicastGroup,multicastPort);
			
			/* Wait until the entire cluster is connected: */
			vruiMultiplexer->waitForConnection();
			
			/* Open a multicast pipe: */
			vruiPipe=new Cluster::MulticastPipe(vruiMultiplexer);
			
			/* Read the verbosity flag: */
			vruiVerbose=vruiPipe->read<char>()!=0;
			
			/* Read the entire configuration file and the root section name: */
			vruiConfigFile=new Misc::ConfigurationFile(*vruiPipe);
			vruiConfigRootSectionName=Misc::readCString(*vruiPipe);
			
			/* Go to the given root section: */
			vruiConfigFile->setCurrentSection("/Vrui");
			vruiConfigFile->setCurrentSection(vruiConfigRootSectionName);
			
			/* Read the application's name and command line: */
			vruiApplicationName=Misc::readCString(*vruiPipe);
			vruiSlaveArgc=vruiPipe->read<int>();
			vruiSlaveArgv=new char*[vruiSlaveArgc+1];
			vruiSlaveArgvShadow=new char*[vruiSlaveArgc+1];
			for(int i=0;i<=vruiSlaveArgc;++i)
				vruiSlaveArgv[i]=0;
			for(int i=0;i<vruiSlaveArgc;++i)
				vruiSlaveArgv[i]=Misc::readCString(*vruiPipe);
			for(int i=0;i<=vruiSlaveArgc;++i)
				vruiSlaveArgvShadow[i]=vruiSlaveArgv[i];
			
			/* Override the actual command line provided by the caller: */
			argc=vruiSlaveArgc;
			argv=vruiSlaveArgvShadow;
			
			/* Register Vrui's cluster multiplexer with the Opener object of the Cluster library: */
			Cluster::Opener::getOpener()->setMultiplexer(vruiMultiplexer);
			}
		catch(const std::runtime_error& err)
			{
			std::cerr<<"Vrui (node "<<nodeIndex<<"): Caught exception "<<err.what()<<" while initializing cluster communication"<<std::endl;
			vruiErrorShutdown(true);
			}
		}
	else
		{
		/***********************
		This is the master node:
		***********************/
		
		/* Extract the application name: */
		const char* appNameStart=argv[0];
		const char* cPtr;
		for(cPtr=appNameStart;*cPtr!='\0';++cPtr)
			if(*cPtr=='/')
				appNameStart=cPtr+1;
		vruiApplicationName=new char[cPtr-appNameStart+1];
		memcpy(vruiApplicationName,appNameStart,cPtr-appNameStart);
		vruiApplicationName[cPtr-appNameStart]='\0';
		
		/* Check the command line for -vruiVerbose, -vruiHelp, and -mergeConfigEarly flags: */
		std::vector<std::string> earlyMerges;
		for(int i=1;i<argc;++i)
			{
			if(strcasecmp(argv[i],"-vruiVerbose")==0)
				{
				std::cout<<"Vrui: Entering verbose mode"<<std::endl;
				vruiVerbose=true;
				
				/* Print information about the Vrui run-time installation: */
				std::cout<<"Vrui: Run-time version ";
				char prevFill=std::cout.fill('0');
				std::cout<<VRUI_INTERNAL_CONFIG_VERSION/1000000<<'.'<<(VRUI_INTERNAL_CONFIG_VERSION/1000)%1000<<'-'<<std::setw(3)<<VRUI_INTERNAL_CONFIG_VERSION%1000;
				std::cout.fill(prevFill);
				std::cout<<" installed in:"<<std::endl;
				std::cout<<"        libraries   : "<<VRUI_INTERNAL_CONFIG_LIBDIR<<std::endl;
				std::cout<<"        executables : "<<VRUI_INTERNAL_CONFIG_EXECUTABLEDIR<<std::endl;
				std::cout<<"        plug-ins    : "<<VRUI_INTERNAL_CONFIG_PLUGINDIR<<std::endl;
				std::cout<<"        config files: "<<VRUI_INTERNAL_CONFIG_ETCDIR<<std::endl;
				std::cout<<"        shared files: "<<VRUI_INTERNAL_CONFIG_SHAREDIR<<std::endl;
				
				/* Remove parameter from argument list: */
				argc-=1;
				for(int j=i;j<argc;++j)
					argv[j]=argv[j+1];
				--i;
				}
			else if(strcasecmp(argv[i]+1,"vruiHelp")==0)
				{
				/* Print information about Vrui command line options: */
				std::cout<<"Vrui-wide command line options:"<<std::endl;
				std::cout<<"  -vruiHelp"<<std::endl;
				std::cout<<"     Prints this help message"<<std::endl;
				std::cout<<"  -vruiVerbose"<<std::endl;
				std::cout<<"     Logs details about Vrui's startup and shutdown procedures to"<<std::endl;
				std::cout<<"     stdout."<<std::endl;
				std::cout<<"  -mergeConfigEarly <configuration file name>"<<std::endl;
				std::cout<<"     Merges the configuration file of the given name into Vrui's"<<std::endl;
				std::cout<<"     configuration space early during Vrui's initialization."<<std::endl;
				std::cout<<"  -mergeConfig <configuration file name>"<<std::endl;
				std::cout<<"     Merges the configuration file of the given name into Vrui's"<<std::endl;
				std::cout<<"     configuration space."<<std::endl;
				std::cout<<"  -setConfig <tag>[=<value>]"<<std::endl;
				std::cout<<"     Overrides a tag value, or removes tag if no =<value> is present, in"<<std::endl;
				std::cout<<"     the current Vrui configuration space. Tag names are relative to the"<<std::endl;
				std::cout<<"     root section in effect when the option is encountered."<<std::endl;
				std::cout<<"  -dumpConfig <configuration file name>"<<std::endl;
				std::cout<<"     Writes the current state of Vrui's configuration space, including"<<std::endl;
				std::cout<<"     all previously merged configuration files, to the configuration"<<std::endl;
				std::cout<<"     file of the given name."<<std::endl;
				std::cout<<"  -rootSection <root section name>"<<std::endl;
				std::cout<<"     Overrides the default root section name."<<std::endl;
				std::cout<<"  -loadInputGraph <input graph file name>"<<std::endl;
				std::cout<<"     Loads the input graph contained in the given file after"<<std::endl;
				std::cout<<"     initialization."<<std::endl;
				std::cout<<"  -addToolClass <tool class name>"<<std::endl;
				std::cout<<"     Adds the tool class of the given name to the tool manager and the"<<std::endl;
				std::cout<<"     tool selection menu."<<std::endl;
				std::cout<<"  -addTool <tool configuration file section name>"<<std::endl;
				std::cout<<"     Adds the tool defined in the given tool configuration section."<<std::endl;
				std::cout<<"  -vislet <vislet class name> [vislet option 1] ... [vislet option n] ;"<<std::endl;
				std::cout<<"     Loads a vislet of the given class name, with the given vislet"<<std::endl;
				std::cout<<"     arguments. Argument list must be terminated with a semicolon."<<std::endl;
				std::cout<<"  -setLinearUnit <unit name> <unit scale factor>"<<std::endl;
				std::cout<<"     Sets the coordinate unit of the Vrui application's navigation space"<<std::endl;
				std::cout<<"     to the given unit name and scale factor."<<std::endl;
				std::cout<<"  -loadView <viewpoint file name>"<<std::endl;
				std::cout<<"     Loads the initial viewing position from the given viewpoint file."<<std::endl;
				
				/* Remove parameter from argument list: */
				argc-=1;
				for(int j=i;j<argc;++j)
					argv[j]=argv[j+1];
				--i;
				}
			else if(strcasecmp(argv[i]+1,"mergeConfigEarly")==0)
				{
				/* Next parameter is name of another configuration file to merge: */
				if(i+1<argc)
					{
					/* Add the name of the configuration file to the list of early merges: */
					earlyMerges.push_back(argv[i+1]);
					
					/* Remove parameters from argument list: */
					argc-=2;
					for(int j=i;j<argc;++j)
						argv[j]=argv[j+2];
					--i;
					}
				else
					{
					/* Ignore the mergeConfigEarly parameter: */
					std::cerr<<"Vrui::init: No configuration file name given after -mergeConfigEarly option"<<std::endl;
					--argc;
					}
				}
			}
		
		/* Get the full name of the global per-user configuration file: */
		const char* userConfigDir=0;
		
		#if VRUI_INTERNAL_CONFIG_HAVE_USERCONFIGFILE
		std::string userConfigDirString;
		
		const char* home=getenv("HOME");
		if(home!=0&&home[0]!='\0')
			{
			userConfigDirString=home;
			userConfigDirString.push_back('/');
			userConfigDirString.append(VRUI_INTERNAL_CONFIG_USERCONFIGDIR);
			
			userConfigDir=userConfigDirString.c_str();
			}
		
		#endif
		
		/* Open the global and user configuration files: */
		vruiOpenConfigurationFile(userConfigDir,argv[0],earlyMerges);
		
		/* Get the root section name: */
		const char* rootSectionName=getenv("VRUI_ROOTSECTION");
		if(rootSectionName==0||rootSectionName[0]=='\0')
			rootSectionName=getenv("HOSTNAME");
		if(rootSectionName==0||rootSectionName[0]=='\0')
			rootSectionName=getenv("HOST");
		
		/* Apply configuration-related arguments from the command line: */
		for(int i=1;i<argc;++i)
			if(argv[i][0]=='-')
				{
				if(strcasecmp(argv[i]+1,"mergeConfig")==0)
					{
					/* Next parameter is name of another configuration file to merge: */
					if(i+1<argc)
						{
						/* Merge the requested configuration file from system-wide and per-user locations: */
						vruiMergeConfigurationFileLayered(userConfigDir,argv[i+1]);
						
						/* Remove parameters from argument list: */
						argc-=2;
						for(int j=i;j<argc;++j)
							argv[j]=argv[j+2];
						--i;
						}
					else
						{
						/* Ignore the mergeConfig parameter: */
						std::cerr<<"Vrui::init: No configuration file name given after -mergeConfig option"<<std::endl;
						--argc;
						}
					}
				else if(strcasecmp(argv[i]+1,"setConfig")==0)
					{
					/* Next parameter is a tag=value pair: */
					if(i+1<argc)
						{
						/* Extract the tag name: */
						const char* tagStart=argv[i+1];
						const char* tagEnd;
						for(tagEnd=tagStart;*tagEnd!='\0'&&*tagEnd!='=';++tagEnd)
							;
						std::string tag(tagStart,tagEnd);
						
						/* Go to the current root section, but be quiet about it: */
						vruiGoToRootSection(rootSectionName,false);
						
						/* Check if there is a value: */
						if(*tagEnd=='=')
							{
							const char* valueStart=tagEnd+1;
							const char* valueEnd;
							for(valueEnd=valueStart;*valueEnd!='\0';++valueEnd)
								;
							std::string value(valueStart,valueEnd);
							
							/* Set the tag's value in the current configuration: */
							vruiConfigFile->storeString(tag.c_str(),value);
							}
						else
							{
							/* Remove the tag from the current configuration: */
							vruiConfigFile->getCurrentSection().removeTag(tag);
							}
						
						/* Remove parameters from argument list: */
						argc-=2;
						for(int j=i;j<argc;++j)
							argv[j]=argv[j+2];
						--i;
						}
					else
						{
						/* Ignore the setConfig parameter: */
						std::cerr<<"Vrui::init: No <tag>[=<value>] given after -setConfig option"<<std::endl;
						--argc;
						}
					}
				else if(strcasecmp(argv[i]+1,"dumpConfig")==0)
					{
					/* Next parameter is name of configuration file to create: */
					if(i+1<argc)
						{
						/* Save the current configuration to the given configuration file: */
						if(vruiVerbose)
							std::cout<<"Vrui: Dumping current configuration space to configuration file "<<argv[i+1]<<"..."<<std::flush;
						vruiConfigFile->saveAs(argv[i+1]);
						if(vruiVerbose)
							std::cout<<" Ok"<<std::endl;
						
						/* Remove parameters from argument list: */
						argc-=2;
						for(int j=i;j<argc;++j)
							argv[j]=argv[j+2];
						--i;
						}
					else
						{
						/* Ignore the dumpConfig parameter: */
						std::cerr<<"Vrui::init: No configuration file name given after -dumpConfig option"<<std::endl;
						--argc;
						}
					}
				else if(strcasecmp(argv[i]+1,"rootSection")==0)
					{
					/* Next parameter is name of root section to use: */
					if(i+1<argc)
						{
						/* Save root section name: */
						rootSectionName=argv[i+1];
						
						/* Remove parameters from argument list: */
						argc-=2;
						for(int j=i;j<argc;++j)
							argv[j]=argv[j+2];
						--i;
						}
					else
						{
						/* Ignore the rootSection parameter: */
						std::cerr<<"Vrui::init: No root section name given after -rootSection option"<<std::endl;
						--argc;
						}
					}
				}
		
		/* Go to the configuration's root section and save the root section name for later: */
		vruiGoToRootSection(rootSectionName,vruiVerbose);
		size_t rsnLength=strlen(rootSectionName);
		vruiConfigRootSectionName=new char[rsnLength+1];
		memcpy(vruiConfigRootSectionName,rootSectionName,rsnLength+1);
		
		/* Check if this is a multipipe environment: */
		if(vruiConfigFile->retrieveValue("./enableMultipipe",false))
			{
			try
				{
				if(vruiVerbose)
					std::cout<<"Vrui: Entering cluster mode"<<std::endl;
				
				/* Read multipipe settings from configuration file: */
				std::string master=vruiConfigFile->retrieveString("./multipipeMaster");
				int masterPort=vruiConfigFile->retrieveValue<int>("./multipipeMasterPort",0);
				StringList slaves=vruiConfigFile->retrieveValue<StringList>("./multipipeSlaves");
				vruiNumSlaves=slaves.size();
				std::string multicastGroup=vruiConfigFile->retrieveString("./multipipeMulticastGroup");
				int multicastPort=vruiConfigFile->retrieveValue<int>("./multipipeMulticastPort");
				unsigned int multicastSendBufferSize=vruiConfigFile->retrieveValue<unsigned int>("./multipipeSendBufferSize",16);
				
				/* Create the multicast multiplexer: */
				vruiMultiplexer=new Cluster::Multiplexer(vruiNumSlaves,0,master.c_str(),masterPort,multicastGroup.c_str(),multicastPort);
				vruiMultiplexer->setSendBufferSize(multicastSendBufferSize);
				
				/* Determine the fully-qualified name of this process's executable: */
				char exeName[PATH_MAX];
				#ifdef __linux__
				ssize_t exeNameLength=readlink("/proc/self/exe",exeName,PATH_MAX-1);
				if(exeNameLength>0)
					exeName[exeNameLength]='\0';
				else
					strcpy(exeName,argv[0]);
				#else
				strcpy(exeName,argv[0]);
				#endif
				
				/* Start the multipipe slaves on all slave nodes: */
				masterPort=vruiMultiplexer->getLocalPortNumber();
				std::string multipipeRemoteCommand=vruiConfigFile->retrieveString("./multipipeRemoteCommand","ssh");
				if(strcasecmp(multipipeRemoteCommand.c_str(),"Manual")!=0)
					{
					vruiSlavePids=new pid_t[vruiNumSlaves];
					std::string cwd=Misc::getCurrentDirectory();
					size_t rcLen=cwd.length()+strlen(exeName)+master.length()+multicastGroup.length()+512;
					char* rc=new char[rcLen];
					if(vruiVerbose)
						std::cout<<"Vrui: Spawning slave processes..."<<std::flush;
					for(int i=0;i<vruiNumSlaves;++i)
						{
						if(vruiVerbose)
							std::cout<<' '<<slaves[i]<<std::flush;
						pid_t childPid=fork();
						if(childPid==0)
							{
							/* Create a command line to run the program from the current working directory: */
							int ai=0;
							ai+=snprintf(rc+ai,rcLen-ai,"cd '%s' ;",cwd.c_str());
							ai+=snprintf(rc+ai,rcLen-ai," %s",exeName);
							ai+=snprintf(rc+ai,rcLen-ai," -vruiMultipipeSlave");
							ai+=snprintf(rc+ai,rcLen-ai," %d %d",vruiNumSlaves,i+1);
							ai+=snprintf(rc+ai,rcLen-ai," %s %d",master.c_str(),masterPort);
							ai+=snprintf(rc+ai,rcLen-ai," %s %d",multicastGroup.c_str(),multicastPort);
							
							/* Create command line for ssh (or other remote login) program: */
							char* sshArgv[20];
							int sshArgc=0;
							sshArgv[sshArgc++]=const_cast<char*>(multipipeRemoteCommand.c_str());
							sshArgv[sshArgc++]=const_cast<char*>(slaves[i].c_str());
							sshArgv[sshArgc++]=rc;
							sshArgv[sshArgc]=0;
							
							/* Run the remote login program: */
							execvp(sshArgv[0],sshArgv);
							}
						else
							{
							/* Store PID of ssh process for later: */
							vruiSlavePids[i]=childPid;
							}
						}
					if(vruiVerbose)
						std::cout<<" Ok"<<std::endl;
					
					/* Clean up: */
					delete[] rc;
					}
				else
					{
					/* Ask the user to start the slave processes manually: */
					std::cout<<"Vrui: Please start slave processes using command lines:"<<std::endl;
					for(int i=0;i<vruiNumSlaves;++i)
						std::cout<<exeName<<" -vruiMultipipeSlave "<<vruiNumSlaves<<" "<<i+1<<" "<<master<<" "<<masterPort<<" "<<multicastGroup<<" "<<multicastPort<<std::endl;
					}
				
				/* Wait until the entire cluster is connected: */
				if(vruiVerbose)
					std::cout<<"Vrui: Waiting for cluster to connect..."<<std::flush;
				vruiMultiplexer->waitForConnection();
				if(vruiVerbose)
					std::cout<<" Ok"<<std::endl;
				
				if(vruiVerbose)
					std::cout<<"Vrui: Distributing configuration and command line..."<<std::flush;
				
				/* Open a multicast pipe: */
				vruiPipe=new Cluster::MulticastPipe(vruiMultiplexer);
				
				/* Send the verbosity flag: */
				vruiPipe->write<char>(vruiVerbose?1:0);
				
				/* Send the entire Vrui configuration file and the root section name across the pipe: */
				vruiConfigFile->writeToPipe(*vruiPipe);
				Misc::writeCString(vruiConfigRootSectionName,*vruiPipe);
				
				/* Write the application's name and command line: */
				Misc::writeCString(vruiApplicationName,*vruiPipe);
				vruiPipe->write<int>(argc);
				for(int i=0;i<argc;++i)
					Misc::writeCString(argv[i],*vruiPipe);
				
				/* Flush the pipe: */
				vruiPipe->flush();
				
				if(vruiVerbose)
					std::cout<<" Ok"<<std::endl;
				
				/* Register Vrui's cluster multiplexer with the Opener object of the Cluster library: */
				Cluster::Opener::getOpener()->setMultiplexer(vruiMultiplexer);
				}
			catch(const std::runtime_error& err)
				{
				if(vruiVerbose)
					std::cout<<" error"<<std::endl;
				std::cerr<<"Master node: Caught exception "<<err.what()<<" while initializing cluster communication"<<std::endl;
				vruiErrorShutdown(true);
				}
			}
		}
	
	/* Synchronize threads between here and end of function body: */
	Cluster::ThreadSynchronizer threadSynchronizer(vruiPipe);
	
	/* Initialize Vrui state object: */
	try
		{
		if(vruiVerbose&&vruiMaster)
			std::cout<<"Vrui: Initializing Vrui environment..."<<std::flush;
		vruiState=new VruiState(vruiMultiplexer,vruiPipe);
		vruiState->initialize(vruiConfigFile->getCurrentSection());
		if(vruiVerbose&&vruiMaster)
			std::cout<<" Ok"<<std::endl;
		}
	catch(const std::runtime_error& err)
		{
		if(vruiVerbose&&vruiMaster)
			std::cout<<" error"<<std::endl;
		std::cerr<<vruiErrorHeader<<"Caught exception "<<err.what()<<" while initializing Vrui state object"<<std::endl;
		vruiErrorShutdown(true);
		}
	
	/* Create the total list of all windows on the cluster: */
	vruiTotalNumWindows=0;
	if(vruiMultiplexer!=0)
		{
		/* Count the number of windows on all cluster nodes: */
		for(unsigned int nodeIndex=0;nodeIndex<vruiMultiplexer->getNumNodes();++nodeIndex)
			{
			/* Remember the index of the first window on this node: */
			if(nodeIndex==vruiMultiplexer->getNodeIndex())
				vruiFirstLocalWindowIndex=vruiTotalNumWindows;
			
			/* Add the number of windows in the node's window list to the total: */
			vruiTotalNumWindows+=int(vruiConfigFile->retrieveValue<StringList>(Misc::stringPrintf("./node%uWindowNames",nodeIndex).c_str()).size());
			}
		}
	else
		{
		/* On a single-machine environment, total windows are local windows: */
		StringList windowNames=vruiConfigFile->retrieveValue<StringList>("./windowNames");
		vruiTotalNumWindows=int(windowNames.size());
		vruiFirstLocalWindowIndex=0;
		}
	vruiTotalWindows=new VRWindow*[vruiTotalNumWindows];
	for(int i=0;i<vruiTotalNumWindows;++i)
		vruiTotalWindows[i]=0;
	
	/* Process additional command line arguments: */
	for(int i=1;i<argc;++i)
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i]+1,"loadInputGraph")==0)
				{
				/* Next parameter is name of input graph file to load: */
				if(i+1<argc)
					{
					/* Save input graph file name: */
					vruiState->loadInputGraph=true;
					vruiState->inputGraphFileName=argv[i+1];
					
					/* Remove parameters from argument list: */
					argc-=2;
					for(int j=i;j<argc;++j)
						argv[j]=argv[j+2];
					--i;
					}
				else
					{
					/* Ignore the loadInputGraph parameter: */
					if(vruiMaster)
						std::cerr<<"Vrui::init: No input graph file name given after -loadInputGraph option"<<std::endl;
					--argc;
					}
				}
			else if(strcasecmp(argv[i]+1,"addToolClass")==0)
				{
				/* Next parameter is name of tool class to load: */
				if(i+1<argc)
					{
					try
						{
						/* Load the tool class: */
						if(vruiVerbose&&vruiMaster)
							std::cout<<"Vrui: Adding requested tool class "<<argv[i+1]<<"..."<<std::flush;
						threadSynchronizer.sync();
						vruiState->toolManager->addClass(argv[i+1]);
						if(vruiVerbose&&vruiMaster)
							std::cout<<" Ok"<<std::endl;
						}
					catch(const std::runtime_error& err)
						{
						/* Print a warning and carry on: */
						if(vruiVerbose&&vruiMaster)
							std::cout<<" error"<<std::endl;
						std::cerr<<vruiErrorHeader<<"Ignoring tool class "<<argv[i+1]<<" due to exception "<<err.what()<<std::endl;
						}
					
					/* Remove parameters from argument list: */
					argc-=2;
					for(int j=i;j<argc;++j)
						argv[j]=argv[j+2];
					--i;
					}
				else
					{
					/* Ignore the addToolClass parameter: */
					if(vruiMaster)
						std::cerr<<"Vrui::init: No tool class name given after -addToolClass option"<<std::endl;
					--argc;
					}
				}
			else if(strcasecmp(argv[i]+1,"addTool")==0)
				{
				/* Next parameter is name of tool binding configuration file section: */
				if(i+1<argc)
					{
					try
						{
						/* Load the tool: */
						if(vruiVerbose&&vruiMaster)
							std::cout<<"Vrui: Adding requested tool from configuration section "<<argv[i+1]<<"..."<<std::flush;
						threadSynchronizer.sync();
						vruiState->toolManager->loadToolBinding(argv[i+1]);
						if(vruiVerbose&&vruiMaster)
							std::cout<<" Ok"<<std::endl;
						}
					catch(const std::runtime_error& err)
						{
						/* Print a warning and carry on: */
						if(vruiVerbose&&vruiMaster)
							std::cout<<" error"<<std::endl;
						std::cerr<<vruiErrorHeader<<"Ignoring tool binding "<<argv[i+1]<<" due to exception "<<err.what()<<std::endl;
						}
					
					/* Remove parameters from argument list: */
					argc-=2;
					for(int j=i;j<argc;++j)
						argv[j]=argv[j+2];
					--i;
					}
				else
					{
					/* Ignore the addTool parameter: */
					if(vruiMaster)
						std::cerr<<"Vrui::init: No tool binding section name given after -addTool option"<<std::endl;
					--argc;
					}
				}
			else if(strcasecmp(argv[i]+1,"vislet")==0)
				{
				if(i+1<argc)
					{
					/* First parameter is name of vislet class: */
					const char* className=argv[i+1];
					
					/* Find semicolon terminating vislet parameter list: */
					int argEnd;
					for(argEnd=i+2;argEnd<argc&&(argv[argEnd][0]!=';'||argv[argEnd][1]!='\0');++argEnd)
						;
					
					if(vruiState->visletManager!=0)
						{
						try
							{
							/* Initialize the vislet: */
							if(vruiVerbose&&vruiMaster)
								std::cout<<"Vrui: Loading vislet of class "<<className<<"..."<<std::flush;
							threadSynchronizer.sync();
							VisletFactory* factory=vruiState->visletManager->loadClass(className);
							vruiState->visletManager->createVislet(factory,argEnd-(i+2),argv+(i+2));
							if(vruiVerbose&&vruiMaster)
								std::cout<<" Ok"<<std::endl;
							}
						catch(const std::runtime_error& err)
							{
							/* Print a warning and carry on: */
							if(vruiVerbose&&vruiMaster)
								std::cout<<" error"<<std::endl;
							std::cerr<<vruiErrorHeader<<"Ignoring vislet of type "<<className<<" due to exception "<<err.what()<<std::endl;
							}
						}
					
					/* Remove all vislet parameters from the command line: */
					if(argEnd<argc)
						++argEnd;
					int numArgs=argEnd-i;
					argc-=numArgs;
					for(int j=i;j<argc;++j)
						argv[j]=argv[j+numArgs];
					--i;
					}
				else
					{
					/* Ignore the vislet parameter: */
					if(vruiMaster)
						std::cerr<<"Vrui: No vislet class name given after -vislet option"<<std::endl;
					argc=i;
					}
				}
			else if(strcasecmp(argv[i]+1,"loadView")==0)
				{
				/* Next parameter is name of viewpoint file to load: */
				if(i+1<argc)
					{
					/* Save viewpoint file name: */
					vruiState->viewpointFileName=argv[i+1];
					
					/* Remove parameters from argument list: */
					argc-=2;
					for(int j=i;j<argc;++j)
						argv[j]=argv[j+2];
					--i;
					}
				else
					{
					/* Ignore the loadView parameter: */
					if(vruiMaster)
						std::cerr<<"Vrui: No viewpoint file name given after -loadView option"<<std::endl;
					--argc;
					}
				}
			else if(strcasecmp(argv[i]+1,"setLinearUnit")==0)
				{
				/* Next two parameters are unit name and scale factor: */
				if(i+2<argc)
					{
					/* Set the linear unit: */
					getCoordinateManager()->setUnit(Geometry::LinearUnit(argv[i+1],atof(argv[i+2])));
					
					/* Remove parameters from argument list: */
					argc-=3;
					for(int j=i;j<argc;++j)
						argv[j]=argv[j+3];
					--i;
					}
				else
					{
					/* Ignore the setLinearUnit parameter: */
					if(vruiMaster)
						std::cerr<<"Vrui: No unit name and scale factor given after -setLinearUnit option"<<std::endl;
					--argc;
					}
				}
			}
	
	if(vruiVerbose&&vruiMaster)
		{
		std::cout<<"Vrui: Command line passed to application:";
		for(int i=1;i<argc;++i)
			std::cout<<" \""<<argv[i]<<'"';
		std::cout<<std::endl;
		}
	}

void startDisplay(void)
	{
	/* Synchronize threads between here and end of function body: */
	Cluster::ThreadSynchronizer threadSynchronizer(vruiState->pipe);
	
	/* Wait for all nodes in the multicast group to reach this point: */
	if(vruiState->multiplexer!=0)
		{
		if(vruiVerbose&&vruiMaster)
			std::cout<<"Vrui: Waiting for cluster before graphics initialization..."<<std::flush;
		vruiState->pipe->barrier();
		if(vruiVerbose&&vruiMaster)
			std::cout<<" Ok"<<std::endl;
		}
	
	if(vruiVerbose&&vruiMaster)
		std::cout<<"Vrui: Starting graphics subsystem..."<<std::endl;
	
	/* Find the mouse adapter listed in the input device manager (if there is one): */
	InputDeviceAdapterMouse* mouseAdapter=0;
	for(int i=0;i<vruiState->inputDeviceManager->getNumInputDeviceAdapters()&&mouseAdapter==0;++i)
		mouseAdapter=dynamic_cast<InputDeviceAdapterMouse*>(vruiState->inputDeviceManager->getInputDeviceAdapter(i));
	
	/* Find the multitouch adapter listed in the input device manager (if there is one): */
	InputDeviceAdapterMultitouch* multitouchAdapter=0;
	for(int i=0;i<vruiState->inputDeviceManager->getNumInputDeviceAdapters()&&multitouchAdapter==0;++i)
		multitouchAdapter=dynamic_cast<InputDeviceAdapterMultitouch*>(vruiState->inputDeviceManager->getInputDeviceAdapter(i));
	
	try
		{
		/* Retrieve the list of VR windows: */
		typedef std::vector<std::string> StringList;
		StringList windowNames;
		if(vruiState->multiplexer!=0)
			windowNames=vruiConfigFile->retrieveValue<StringList>(Misc::stringPrintf("./node%dWindowNames",vruiState->multiplexer->getNodeIndex()).c_str());
		else
			windowNames=vruiConfigFile->retrieveValue<StringList>("./windowNames");
		
		/* Ready the GLObject manager to initialize its objects per-window: */
		GLContextData::resetThingManager();
		
		/* Initialize the window list: */
		vruiNumWindows=windowNames.size();
		vruiWindows=new VRWindow*[vruiNumWindows];
		for(int i=0;i<vruiNumWindows;++i)
			vruiWindows[i]=0;
		
		/* Initialize X11 if any windows need to be opened: */
		if(vruiNumWindows>0)
			{
			#if 0 //  Not necessary; Vrui never makes X calls to the same display concurrently from different threads
			/* Enable thread management in X11 library: */
			XInitThreads();
			#endif
			
			/* Set error handlers: */
			XSetErrorHandler(vruiXErrorHandler);
			XSetIOErrorHandler(vruiXIOErrorHandler);
			}
		
		/* Sort the windows into groups based on their group IDs and collect each group's OpenGL context properties: */
		WindowGroup::CreatorMap windowGroups=WindowGroup::collectWindowGroups(vruiApplicationName,windowNames,vruiWindows,*vruiConfigFile);
		
		/* Initialize the window groups array: */
		vruiNumWindowGroups=int(windowGroups.getNumEntries());
		vruiWindowGroups=new WindowGroup[vruiNumWindowGroups];
		
		/* Check if window groups should be rendered in parallel: */
		if(vruiNumWindowGroups>1)
			{
			vruiConfigFile->updateValue("./renderInParallel",vruiRenderInParallel);
			#if !GLSUPPORT_CONFIG_USE_TLS
			if(vruiVerbose&&vruiRenderInParallel)
				std::cout<<"Vrui: Parallel rendering not supported"<<std::endl;
			vruiRenderInParallel=false;
			#endif
			}
		
		if(vruiVerbose)
			{
			std::cout<<"Vrui: Opening "<<vruiNumWindows<<(vruiNumWindows!=1?" windows":" window");
			if(vruiNumWindowGroups>1)
				std::cout<<" in "<<vruiNumWindowGroups<<" window groups (rendering "<<(vruiRenderInParallel?"in parallel":"serially")<<")";
			else
				std::cout<<" in 1 window group";
			std::cout<<std::endl;
			}
		
		/* Retrieve the name of the optional synchronization window: */
		std::string syncWindowName;
		vruiConfigFile->updateString("./syncWindowName",syncWindowName);
		vruiState->synced=!syncWindowName.empty();
		
		/* Create all windows in all window groups: */
		bool allWindowsOk=true;
		int windowGroupIndex=0;
		for(WindowGroup::CreatorMap::Iterator wgIt=windowGroups.begin();allWindowsOk&&!wgIt.isFinished();++wgIt,++windowGroupIndex)
			allWindowsOk=vruiWindowGroups[windowGroupIndex].initialize(wgIt->getDest(),syncWindowName,mouseAdapter,multitouchAdapter);
		
		if(!allWindowsOk)
			{
			/* Delete the window groups array: */
			vruiNumWindowGroups=0;
			delete[] vruiWindowGroups;
			vruiWindowGroups=0;
			
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannnot create all rendering windows");
			}
		
		/* Populate the total list of all windows on the cluster: */
		for(int i=0;i<vruiNumWindows;++i)
			{
			/* Store the node-local window pointer in the cluster-wide list: */
			vruiTotalWindows[vruiFirstLocalWindowIndex+i]=vruiWindows[i];
			
			/* Tell the window its own index in the cluster-wide list: */
			vruiWindows[i]->setWindowIndex(vruiFirstLocalWindowIndex+i);
			}
		
		/* Check if there are multiple window groups, so multiple threads can be used: */
		if(vruiRenderInParallel&&vruiNumWindowGroups>1)
			{
			/* Release the OpenGL contexts of all window groups from this thread: */
			for(int windowGroupIndex=0;windowGroupIndex<vruiNumWindowGroups;++windowGroupIndex)
				vruiWindowGroups[windowGroupIndex].getContext().release();
			
			/* Initialize the rendering barrier: */
			vruiRenderingBarrier.setNumSynchronizingThreads(vruiNumWindowGroups+1);
			
			/* Create one rendering thread for each window group: */
			vruiRenderingThreads=new Threads::Thread[vruiNumWindowGroups];
			for(int windowGroupIndex=0;windowGroupIndex<vruiNumWindowGroups;++windowGroupIndex)
				vruiRenderingThreads[windowGroupIndex].start(vruiRenderingThreadFunction,windowGroupIndex);
			
			/* Wait until all rendering threads have initialized: */
			vruiRenderingBarrier.synchronize();
			}
		
		if(vruiVerbose&&vruiMaster)
			std::cout<<"Vrui: Graphics subsystem Ok"<<std::endl;
		}
	catch(const std::runtime_error& err)
		{
		std::cerr<<vruiErrorHeader<<"Caught exception "<<err.what()<<" while initializing rendering windows"<<std::endl;
		vruiErrorShutdown(true);
		}
	catch(...)
		{
		std::cerr<<vruiErrorHeader<<"Caught spurious exception while initializing rendering windows"<<std::endl;
		vruiErrorShutdown(true);
		}
	}

void startSound(void)
	{
	/* Synchronize threads between here and end of function body: */
	Cluster::ThreadSynchronizer threadSynchronizer(vruiState->pipe);
	
	/* Wait for all nodes in the multicast group to reach this point: */
	if(vruiState->multiplexer!=0)
		{
		if(vruiVerbose&&vruiMaster)
			std::cout<<"Vrui: Waiting for cluster before sound initialization..."<<std::flush;
		vruiState->pipe->barrier();
		if(vruiVerbose&&vruiMaster)
			std::cout<<" Ok"<<std::endl;
		}
	
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	
	if(vruiVerbose&&vruiMaster)
		std::cout<<"Vrui: Starting sound subsystem..."<<std::endl;
	
	/* Retrieve the name of the sound context: */
	std::string soundContextName;
	if(vruiState->multiplexer!=0)
		soundContextName=vruiConfigFile->retrieveValue<std::string>(Misc::stringPrintf("./node%dSoundContextName",vruiState->multiplexer->getNodeIndex()).c_str(),"");
	else
		soundContextName=vruiConfigFile->retrieveValue<std::string>("./soundContextName","");
	if(soundContextName.empty())
		return;
	
	/* Ready the ALObject manager to initialize its objects per-context: */
	ALContextData::resetThingManager();
	
	try
		{
		/* Open the sound context configuration file section: */
		Misc::ConfigurationFileSection cfs=vruiConfigFile->getSection(soundContextName.c_str());
		if(vruiVerbose)
			std::cout<<vruiErrorHeader<<"Opening sound context from configuration section "<<cfs.getName()<<':'<<std::endl;
		
		/* Create a new sound context: */
		SoundContext* sc=new SoundContext(cfs,vruiState);
		
		/* Install the Vrui sound context: */
		vruiNumSoundContexts=1;
		vruiSoundContexts=new SoundContext*[1];
		vruiSoundContexts[0]=sc;
		
		/* Set the recording sound context if the created sound context can record: */
		if(vruiSoundContexts[0]->canRecord())
			vruiRecordingSoundContext=vruiSoundContexts[0];
		
		/* Initialize all ALObjects for this sound context's context data: */
		vruiSoundContexts[0]->makeCurrent();
		vruiSoundContexts[0]->getContextData().updateThings();
		}
	catch(const std::runtime_error& err)
		{
		std::cerr<<vruiErrorHeader<<"Disabling OpenAL sound due to exception "<<err.what()<<std::endl;
		if(vruiSoundContexts!=0)
			{
			if(vruiSoundContexts[0]!=0)
				{
				delete vruiSoundContexts[0];
				vruiSoundContexts[0]=0;
				}
			delete[] vruiSoundContexts;
			vruiSoundContexts=0;
			vruiNumSoundContexts=0;
			}
		}
	catch(...)
		{
		std::cerr<<vruiErrorHeader<<"Disabling OpenAL sound due to spurious exception"<<std::endl;
		if(vruiSoundContexts!=0)
			{
			if(vruiSoundContexts[0]!=0)
				{
				delete vruiSoundContexts[0];
				vruiSoundContexts[0]=0;
				}
			delete[] vruiSoundContexts;
			vruiSoundContexts=0;
			vruiNumSoundContexts=0;
			}
		}
	
	#else
	
	if(vruiVerbose&&vruiMaster)
		std::cout<<"Vrui: Sound disabled due to missing OpenAL library"<<std::endl;
	
	#endif
	}

void vruiInnerLoopMultiWindow(void)
	{
	/* Print frame rates at regular intervals for window-less Vrui head nodes: */
	Threads::EventTime nextFrameRateDisplayTime;
	nextFrameRateDisplayTime+=Threads::EventInterval(1,0);
	unsigned long frameRateBaseIndex=vruiState->frameIndex;
	
	/* Run the main loop until shut down: */
	while(true)
		{
		/* Start a new Vrui frame and bail out if shutdown was requested: */
		if(!vruiState->startFrame())
			break;
		
		/* Main loop instrumentation: */
		vruiState->renderStart.set();
		
		/* Reset the AL thing manager: */
		ALContextData::resetThingManager();
		
		#if ALSUPPORT_CONFIG_HAVE_OPENAL
		/* Update all sound contexts: */
		for(int i=0;i<vruiNumSoundContexts;++i)
			vruiSoundContexts[i]->draw();
		#endif
		
		/* Reset the GL thing manager: */
		GLContextData::resetThingManager();
		
		if(vruiNumWindowGroups>1)
			{
			if(vruiRenderInParallel)
				{
				/* Start the rendering cycle by synchronizing with the render threads: */
				vruiRenderingBarrier.synchronize();
				
				/* Wait until all threads are done rendering: */
				vruiRenderingBarrier.synchronize();
				
				if(vruiState->multiplexer!=0)
					{
					/* Synchronize with other nodes: */
					vruiState->pipe->barrier();
					
					/* Main loop instrumentation: */
					vruiState->renderEnd.set();
					
					/* Notify the render threads to swap buffers: */
					vruiRenderingBarrier.synchronize();
					}
				
				/* Wait until all threads are done swapping buffers: */
				vruiRenderingBarrier.synchronize();
				
				/* Main loop instrumentation: */
				vruiState->present.set();
				}
			else
				{
				/* Draw all window groups sequentially: */
				for(int i=0;i<vruiNumWindowGroups;++i)
					vruiWindowGroups[i].draw();
				
				/* Wait for all windows in all window groups to finish rendering: */
				for(int i=0;i<vruiNumWindowGroups;++i)
					vruiWindowGroups[i].wait();
				
				/* Wait until all other nodes in a cluster are finished rendering: */
				if(vruiState->multiplexer!=0)
					vruiState->pipe->barrier();
				
				/* Main loop instrumentation: */
				vruiState->renderEnd.set();
				
				/* Present the rendering results of all windows in all window groups at once: */
				for(int i=0;i<vruiNumWindowGroups;++i)
					vruiWindowGroups[i].present();
				
				/* Main loop instrumentation: */
				vruiState->present.set();
				}
			}
		else if(vruiNumWindows>0)
			{
			/* Draw the only window group: */
			vruiWindowGroups[0].draw();
			
			/* Wait for all windows to finish rendering: */
			vruiWindowGroups[0].wait();
			
			/* Wait until all other nodes in a cluster are finished rendering: */
			if(vruiState->multiplexer!=0)
				vruiState->pipe->barrier();
			
			/* Main loop instrumentation: */
			vruiState->renderEnd.set();
			
			/* Present the rendering results of all windows at once: */
			vruiWindowGroups[0].present();
			
			/* Main loop instrumentation: */
			vruiState->present.set();
			}
		else
			{
			/* If in a cluster, synchronize with other nodes: */
			if(vruiState->multiplexer!=0)
				vruiState->pipe->barrier();
			
			/* Main loop instrumentation: */
			vruiState->renderEnd.set();
			vruiState->present=vruiState->renderEnd;
			
			/* Print current frame rate on head node's console for window-less Vrui processes: */
			if(vruiMaster)
				{
				Threads::EventTime now;
				if(now>=nextFrameRateDisplayTime)
					{
					unsigned int numFrames=(unsigned int)(vruiState->frameIndex-frameRateBaseIndex);
					printf("Current frame rate: %8u fps\r",numFrames);
					fflush(stdout);
					
					nextFrameRateDisplayTime+=Threads::EventInterval(1,0);
					frameRateBaseIndex=vruiState->frameIndex;
					}
				}
			}
		
		/* Call all post-rendering callbacks: */
		{
		Misc::CallbackData cbData;
		vruiState->postRenderingCallbacks.call(&cbData);
		}
		
		/* Main loop instrumentation: */
		vruiState->postRenderEnd.set();
		}
	
	/* If we were printing frame rates on a window-less head node, clean that up: */
	if(vruiNumWindows==0&&vruiMaster)
		{
		printf("\n");
		fflush(stdout);
		}
	}

void vruiInnerLoopSingleWindow(void)
	{
	while(true)
		{
		/* Start a new Vrui frame and bail out if shutdown was requested: */
		if(!vruiState->startFrame())
			break;
		
		/* Main loop instrumentation: */
		vruiState->renderStart.set();
		
		/* Reset the AL thing manager: */
		ALContextData::resetThingManager();
		
		#if ALSUPPORT_CONFIG_HAVE_OPENAL
		/* Update all sound contexts: */
		for(int i=0;i<vruiNumSoundContexts;++i)
			vruiSoundContexts[i]->draw();
		#endif
		
		/* Reset the GL thing manager: */
		GLContextData::resetThingManager();
		
		/* Draw the only window group: */
		vruiWindowGroups[0].draw();
		
		/* Wait for the only window to finish rendering: */
		vruiWindowGroups[0].wait();
		
		/* Wait until all other nodes in a cluster are finished rendering: */
		if(vruiState->multiplexer!=0)
			vruiState->pipe->barrier();
		
		/* Main loop instrumentation: */
		vruiState->renderEnd.set();
		
		/* Present the rendering results of the only window: */
		vruiWindowGroups[0].present();
		
		/* Main loop instrumentation: */
		vruiState->present.set();
		
		/* Call all post-rendering callbacks: */
		{
		Misc::CallbackData cbData;
		vruiState->postRenderingCallbacks.call(&cbData);
		}
		
		/* Main loop instrumentation: */
		vruiState->postRenderEnd.set();
		}
	}

void mainLoop(void)
	{
	/* Bail out if someone requested a shutdown during the initialization procedure: */
	if(vruiState->runLoop.wasStopped())
		{
		if(vruiVerbose&&vruiMaster)
			std::cout<<"Vrui: Shutting down due to shutdown request during initialization"<<std::endl;
		return;
		}
	
	/* Start the display subsystem: */
	startDisplay();
	
	if(vruiState->useSound)
		{
		/* Start the sound subsystem: */
		startSound();
		}
	
	/* Initialize the navigation transformation: */
	vruiState->application->resetNavigation();
	
	/* Wait for all nodes in the multicast group to reach this point: */
	if(vruiState->multiplexer!=0)
		{
		if(vruiVerbose&&vruiMaster)
			std::cout<<"Vrui: Waiting for cluster before preparing main loop..."<<std::flush;
		vruiState->pipe->barrier();
		if(vruiVerbose&&vruiMaster)
			std::cout<<" Ok"<<std::endl;
		}
	
	/* Prepare Vrui state for main loop: */
	if(vruiVerbose&&vruiMaster)
		std::cout<<"Vrui: Preparing main loop..."<<std::flush;
	vruiState->prepareMainLoop();
	if(vruiVerbose&&vruiMaster)
		std::cout<<" Ok"<<std::endl;
	
	/* Listen for commands on stdin: */
	Threads::IOWatcherEventHandler* commandDispatcherFunction=Threads::createFunctionCall(vruiState,&VruiState::dispatchCommandsCallback);
	vruiStdinWatcher=new Threads::IOWatcher(vruiState->runLoop,STDIN_FILENO,Threads::IOWatcher::Read,true,*commandDispatcherFunction);
	
	/* If there is a command pipe, listen for commands on that: */
	std::string commandPipeName=vruiConfigFile->retrieveString("./commandPipeName",std::string());
	if(!commandPipeName.empty())
		{
		/* Open the command pipe: */
		vruiCommandPipe=open(commandPipeName.c_str(),O_RDONLY|O_NONBLOCK);
		if(vruiCommandPipe>=0)
			{
			/* Open an extra (non-writing) writer to hold the command pipe open between external writers: */
			vruiCommandPipeHolder=open(commandPipeName.c_str(),O_WRONLY|O_NONBLOCK);
			}
		if(vruiCommandPipeHolder>=0)
			{
			if(vruiVerbose&&vruiMaster)
				std::cout<<"Vrui: Listening for commands on pipe "<<commandPipeName<<std::endl;
			
			/* Listen for commands on the pipe: */
			vruiCommandPipeWatcher=new Threads::IOWatcher(vruiState->runLoop,vruiCommandPipe,Threads::IOWatcher::Read,true,*commandDispatcherFunction);
			}
		else
			{
			int error=errno;
			if(vruiMaster)
				std::cout<<"Vrui: Unable to listen for commands from command pipe "<<commandPipeName<<" due to error "<<error<<" ("<<strerror(error)<<")"<<std::endl;
			if(vruiCommandPipe>=0)
				close(vruiCommandPipe);
			vruiCommandPipe=-1;
			}
		}
	commandDispatcherFunction=0;
	
	/* Perform the main loop until the quit command is entered: */
	if(vruiVerbose&&vruiMaster)
		std::cout<<"Vrui: Entering main loop"<<std::endl;
	if(vruiMaster&&vruiNumWindows==0)
		std::cout<<"Vrui: Enter \"quit\" to exit from main loop..."<<std::endl;
	if(vruiNumWindows!=1)
		vruiInnerLoopMultiWindow();
	else
		vruiInnerLoopSingleWindow();
	
	/* Perform first clean-up steps: */
	if(vruiVerbose&&vruiMaster)
		std::cout<<"Vrui: Exiting main loop..."<<std::flush;
	vruiStdinWatcher=0;
	vruiCommandPipeWatcher=0;
	vruiState->finishMainLoop();
	if(vruiVerbose&&vruiMaster)
		std::cout<<" Ok"<<std::endl;
	
	/* Shut down the rendering system: */
	if(vruiVerbose&&vruiMaster)
		std::cout<<"Vrui: Shutting down graphics subsystem..."<<std::flush;
	GLContextData::shutdownThingManager();
	if(vruiRenderingThreads!=0)
		{
		/* Shut down all rendering threads: */
		vruiStopRenderingThreads=true;
		vruiRenderingBarrier.synchronize();
		for(int i=0;i<vruiNumWindowGroups;++i)
			{
			// vruiRenderingThreads[i].cancel();
			vruiRenderingThreads[i].join();
			}
		delete[] vruiRenderingThreads;
		vruiRenderingThreads=0;
		}
	if(vruiWindows!=0)
		{
		/* Release all OpenGL state: */
		for(int i=0;i<vruiNumWindowGroups;++i)
			vruiWindowGroups[i].releaseGLState();
		
		/* Delete all windows: */
		for(int i=0;i<vruiNumWindows;++i)
			delete vruiWindows[i];
		delete[] vruiWindows;
		vruiWindows=0;
		delete[] vruiWindowGroups;
		vruiWindowGroups=0;
		delete[] vruiTotalWindows;
		vruiTotalWindows=0;
		vruiTotalNumWindows=0;
		}
	if(vruiVerbose&&vruiMaster)
		std::cout<<" Ok"<<std::endl;
	
	/* Shut down the sound system: */
	if(vruiVerbose&&vruiMaster&&vruiSoundContexts!=0)
		std::cout<<"Vrui: Shutting down sound subsystem..."<<std::flush;
	ALContextData::shutdownThingManager();
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	if(vruiSoundContexts!=0)
		{
		/* Destroy all sound contexts: */
		for(int i=0;i<vruiNumSoundContexts;++i)
			delete vruiSoundContexts[i];
		delete[] vruiSoundContexts;
		}
	#endif
	if(vruiVerbose&&vruiMaster&&vruiSoundContexts!=0)
		std::cout<<" Ok"<<std::endl;
	}

void deinit(void)
	{
	/* Clean up: */
	if(vruiVerbose&&vruiMaster)
		std::cout<<"Vrui: Shutting down Vrui environment"<<std::endl;
	delete[] vruiApplicationName;
	delete vruiState;
	
	if(vruiMultiplexer!=0)
		{
		if(vruiVerbose&&vruiMaster)
			std::cout<<"Vrui: Exiting cluster mode"<<std::endl;
		
		/* Unregister the multiplexer from the Cluster::Opener object: */
		Cluster::Opener::getOpener()->setMultiplexer(0);
		
		/* Destroy the multiplexer: */
		if(vruiVerbose&&vruiMaster)
			std::cout<<"Vrui: Shutting down intra-cluster communication..."<<std::flush;
		delete vruiPipe;
		delete vruiMultiplexer;
		if(vruiVerbose&&vruiMaster)
			std::cout<<" Ok"<<std::endl;
		
		if(vruiMaster&&vruiSlavePids!=0)
			{
			/* Wait for all slaves to terminate: */
			if(vruiVerbose)
				std::cout<<"Vrui: Waiting for slave processes to terminate..."<<std::flush;
			for(int i=0;i<vruiNumSlaves;++i)
				waitpid(vruiSlavePids[i],0,0);
			delete[] vruiSlavePids;
			vruiSlavePids=0;
			if(vruiVerbose)
				std::cout<<" Ok"<<std::endl;
			}
		if(!vruiMaster&&vruiSlaveArgv!=0)
			{
			/* Delete the slave command line: */
			for(int i=0;i<vruiSlaveArgc;++i)
				delete[] vruiSlaveArgv[i];
			delete[] vruiSlaveArgv;
			vruiSlaveArgv=0;
			delete[] vruiSlaveArgvShadow;
			vruiSlaveArgvShadow=0;
			}
		}
	
	/* Close the configuration file: */
	delete vruiConfigFile;
	delete[] vruiConfigRootSectionName;
	
	/* Close the command pipe: */
	if(vruiCommandPipe>=0)
		{
		close(vruiCommandPipeHolder);
		close(vruiCommandPipe);
		}
	}

void shutdown(void)
	{
	/* Signal asynchronous shutdown if this node is the master node: */
	if(vruiMaster)
		vruiState->runLoop.stop();
	}

const char* getApplicationName(void)
	{
	return vruiApplicationName;
	}

const char* getRootSectionName(void)
	{
	return vruiConfigRootSectionName;
	}

Misc::ConfigurationFileSection getAppConfigurationSection(void)
	{
	return vruiConfigFile->getSection(vruiApplicationName);
	}

Misc::ConfigurationFileSection getModuleConfigurationSection(const char* moduleName)
	{
	return vruiConfigFile->getSection(moduleName);
	}

int getNumWindows(void)
	{
	return vruiTotalNumWindows;
	}

VRWindow* getWindow(int index)
	{
	return vruiTotalWindows[index];
	}

int getNumSoundContexts(void)
	{
	return vruiNumSoundContexts;
	}

SoundContext* getSoundContext(int index)
	{
	return vruiSoundContexts[index];
	}

SoundContext* getRecordingSoundContext(void)
	{
	return vruiRecordingSoundContext;
	}

void requestUpdate(void)
	{
	/* Wake up the run loop, but only if this is the master node: */
	if(vruiMaster)
		vruiState->runLoop.wakeUp();
	}

}
