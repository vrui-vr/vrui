/***********************************************************************
FindHMD - Utility to find a connected HMD based on its preferred video
mode, using the X11 Xrandr extension.
Copyright (c) 2018-2022 Oliver Kreylos

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
#include <string>
#include <iostream>
#include <iomanip>
#include <USB/VendorProductId.h>
#include <USB/DeviceList.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#define VERBOSE 0

/****************************************************************
Function to identify a head-mounted display based on its USB IDs:
****************************************************************/

enum HMDType // Enumerated type for supported HMDs
	{
	HTCVive,HTCVivePro,ValveIndex,
	NoHMD
	};

static const USB::VendorProductId hmdIds[NoHMD]= // USB IDs for supported HMDs
	{
	USB::VendorProductId(0x0bb4U,0x2c87U), // HTC Vive
	USB::VendorProductId(0x0bb4U,0x0309U), // HTC Vive Pro
	USB::VendorProductId(0x28deU,0x2613U) // Valve Index
	};

static const char* hmdTypes[NoHMD]= // Names of supported HMDs
	{
	"HTC Vive",
	"HTC Vive Pro",
	"Valve Index"
	};

HMDType findHMD(void)
	{
	/* Enumerate all USB devices: */
	USB::DeviceList devices;
	
	/* Find the first supported HMD: */
	for(size_t i=0;i<devices.getNumDevices();++i)
		{
		/* Check the device's ID against the supported list: */
		USB::VendorProductId deviceId=devices.getVendorProductId(i);
		for(int j=HTCVive;j<NoHMD;++j)
			if(deviceId==hmdIds[j])
				{
				/* Found it! */
				return HMDType(j);
				}
		}

	/* Didn't find anything! */
	return NoHMD;
	}

/**********************************************************
Display sizes and default refresh rates for supported HMDs:
**********************************************************/

static const unsigned int hmdScreenSizes[NoHMD][2]=
	{
	{2160U,1200U}, // HTC Vive
	{2880U,1600U}, // HTC Vive Pro
	{2880U,1600U} // Valve Index
	};

static const double hmdRefreshRates[NoHMD]=
	{
	89.53, // HTC Vive
	90.0409, // HTC Vive Pro
	144.0 // Valve Index
	};

int xrandrErrorBase;
bool hadError=false;

int errorHandler(Display* display,XErrorEvent* err)
	{
	if(err->error_code==BadValue)
		std::cout<<"X error: bad value"<<std::endl;
	else
		std::cout<<"X error: unknown error"<<std::endl;
	
	hadError=true;
	return 0;
	}

class ModeFunctor // Functor class receiving modes
	{
	/* Constructors and destructors: */
	public:
	virtual ~ModeFunctor(void)
		{
		};
	
	/* Methods: */
	virtual int testMode(XRROutputInfo* outputInfo,XRRModeInfo* modeInfo,XRRCrtcInfo* crtcInfo,bool primary) =0;
	virtual int finalize(int result)
		{
		if(result<0)
			{
			std::cerr<<"FindHMD: No matching mode found"<<std::endl;
			return 1;
			}
		else
			return result;
		}
	};

class MatchHMDFunctor:public ModeFunctor
	{
	/* Elements: */
	private:
	unsigned int size[2];
	double rateMin,rateMax;
	
	/* Constructors and destructors: */
	public:
	MatchHMDFunctor(const unsigned int sSize[2],double sRate,double sRateFuzz)
		:rateMin(sRate/(sRateFuzz+1.0)),rateMax(sRate*(sRateFuzz+1.0))
		{
		for(int i=0;i<2;++i)
			size[i]=sSize[i];
		}
	
	/* Methods from class ModeFunctor: */
	virtual int testMode(XRROutputInfo* outputInfo,XRRModeInfo* modeInfo,XRRCrtcInfo* crtcInfo,bool primary)
		{
		/* Check if the mode matches the HMD's specs: */
		double modeRate=double(modeInfo->dotClock)/(double(modeInfo->vTotal)*double(modeInfo->hTotal));
		return matchMode(modeInfo->width==size[0]&&modeInfo->height==size[1]&&modeRate>=rateMin&&modeRate<=rateMax,outputInfo,modeInfo,crtcInfo,primary);
		}
	
	/* New methods: */
	virtual int matchMode(bool matched,XRROutputInfo* outputInfo,XRRModeInfo* modeInfo,XRRCrtcInfo* crtcInfo,bool primary) =0;
	};

class FindHMDFunctor:public MatchHMDFunctor
	{
	/* Elements: */
	private:
	bool printGeometry;
	
	/* Constructors and destructors: */
	public:
	FindHMDFunctor(const unsigned int sSize[2],double sRate,double sRateFuzz,bool sPrintGeometry)
		:MatchHMDFunctor(sSize,sRate,sRateFuzz),
		 printGeometry(sPrintGeometry)
		{
		}
	
	/* Methods from class MatchHMDFunctor: */
	virtual int matchMode(bool matched,XRROutputInfo* outputInfo,XRRModeInfo* modeInfo,XRRCrtcInfo* crtcInfo,bool primary)
		{
		if(matched)
			{
			/* Print the output's name: */
			std::string outputName(outputInfo->name,outputInfo->name+outputInfo->nameLen);
			if(printGeometry)
				std::cout<<crtcInfo->width<<'x'<<crtcInfo->height<<'+'<<crtcInfo->x<<'+'<<crtcInfo->y<<std::endl;
			else
				std::cout<<outputName<<std::endl;
			if(crtcInfo==0)
				{
				/* Print and signal an error: */
				std::cerr<<"FindHMD: HMD found on video output port "<<outputName<<", but is not enabled"<<std::endl;
				return 2;
				}
			else
				return 0;
			}
		else
			return -1;
		}
	};

class XrandrCommandFunctor:public MatchHMDFunctor
	{
	/* Elements: */
	private:
	bool enable;
	std::string lastOutputName;
	std::string firstOutputName;
	bool hadActive;
	bool hadPrimary;
	int nonHmdBox[4];
	std::string hmdOutputName;
	RRMode hmdMode;
	
	/* Constructors and destructors: */
	public:
	XrandrCommandFunctor(const unsigned int sSize[2],double sRate,double sRateFuzz,bool sEnable)
		:MatchHMDFunctor(sSize,sRate,sRateFuzz),
		 enable(sEnable),hadActive(false),hadPrimary(false)
		{
		nonHmdBox[1]=nonHmdBox[0]=32768;
		nonHmdBox[3]=nonHmdBox[2]=-32768;
		}
	
	/* Methods from class MatchHMDFunctor: */
	virtual int matchMode(bool matched,XRROutputInfo* outputInfo,XRRModeInfo* modeInfo,XRRCrtcInfo* crtcInfo,bool primary)
		{
		/* Get the output's name: */
		std::string outputName(outputInfo->name,outputInfo->name+outputInfo->nameLen);
		
		/* Check if this is a new output: */
		if(lastOutputName!=outputName)
			{
			/* Remember the first output: */
			if(firstOutputName.empty())
				firstOutputName=outputName;
			
			/* Disable the last output if it did not have any active modes: */
			if(!lastOutputName.empty()&&!hadActive&&lastOutputName!=hmdOutputName)
				std::cout<<" --output "<<lastOutputName<<" --off";
			
			lastOutputName=outputName;
			hadActive=false;
			}
		
		/* Check if the mode matches the HMD: */
		if(matched)
			{
			/* Remember the HMD port name and mode: */
			hmdOutputName=outputName;
			hmdMode=modeInfo->id;
			
			/* If this was the first output, forget it again: */
			if(firstOutputName==outputName)
				firstOutputName.clear();
			}
		else
			{
			/* Check if the connected display is enabled: */
			if(crtcInfo!=0)
				{
				/* Set the connected display to its current mode: */
				std::cout<<" --output "<<outputName;
				std::cout<<" --mode 0x"<<std::hex<<modeInfo->id<<std::dec;
				std::cout<<" --pos "<<crtcInfo->x<<'x'<<crtcInfo->y;
				
				/* Add the display to the non-HMD bounding box: */
				if(nonHmdBox[0]>crtcInfo->x)
					nonHmdBox[0]=crtcInfo->x;
				if(nonHmdBox[1]>crtcInfo->y)
					nonHmdBox[1]=crtcInfo->y;
				if(nonHmdBox[2]<crtcInfo->x+int(crtcInfo->width))
					nonHmdBox[2]=crtcInfo->x+int(crtcInfo->width);
				if(nonHmdBox[3]<crtcInfo->y+int(crtcInfo->height))
					nonHmdBox[3]=crtcInfo->y+int(crtcInfo->height);
				
				/* Check if this output should be the primary: */
				if(primary)
					{
					std::cout<<" --primary";
					hadPrimary=true;
					}
				
				hadActive=true;
				}
			}
		
		return -1;
		}
	virtual int finalize(int result)
		{
		/* Disable the last output if it did not have any active modes: */
		if(!lastOutputName.empty()&&!hadActive&&lastOutputName!=hmdOutputName)
			std::cout<<" --output "<<lastOutputName<<" --off";
		
		/* Check if no primary outputs were found (which usually means the HMD is the primary, oops): */
		if(!hadPrimary)
			{
			/* Make the first non-HMD output the primary: */
			std::cout<<" --output "<<firstOutputName<<" --primary";
			}
		
		if(!hmdOutputName.empty())
			{
			std::cout<<" --output "<<hmdOutputName;
			if(enable)
				{
				std::cout<<" --mode 0x"<<std::hex<<hmdMode<<std::dec;
				std::cout<<" --pos "<<nonHmdBox[2]<<'x'<<nonHmdBox[1];
				}
			else
				std::cout<<" --off";
			std::cout<<std::endl;
			
			return 0;
			}
		else
			{
			std::cout<<std::endl;
			std::cerr<<"FindHMD: No matching mode found"<<std::endl;
			return 1;
			}
		}
	};

XRRModeInfo* findMode(XRRScreenResources* screenResources,RRMode modeId)
	{
	/* Find the mode ID in the screen resource's modes: */
	for(int i=0;i<screenResources->nmode;++i)
		if(screenResources->modes[i].id==modeId)
			return &screenResources->modes[i];
	
	return 0;
	}

int enumerateModes(Display* display,bool activeOnly,ModeFunctor& modeFunctor)
	{
	int result=-1;
	
	/* Iterate through all of the display's screens: */
	for(int screen=0;screen<XScreenCount(display)&&result<0;++screen)
		{
		/* Get the screen's resources: */
		XRRScreenResources* screenResources=XRRGetScreenResources(display,RootWindow(display,screen));
		
		/* Find the screen's primary monitor: */
		int numMonitors=0;
		XRRMonitorInfo* monitors=XRRGetMonitors(display,RootWindow(display,screen),True,&numMonitors);
		XRRMonitorInfo* primaryMonitor=0;
		for(int monitor=0;monitor<numMonitors;++monitor)
			if(monitors[monitor].primary)
				primaryMonitor=&monitors[monitor];
		
		/* Iterate through all of the screen's outputs: */
		for(int output=0;output<screenResources->noutput&&result<0;++output)
			{
			/* Get the output descriptor and check if there is a display connected: */
			XRROutputInfo* outputInfo=XRRGetOutputInfo(display,screenResources,screenResources->outputs[output]);
			if(outputInfo->nmode>0)
				{
				/* Get a CRTC descriptor for the output's active CRTC: */
				XRRCrtcInfo* crtcInfo=outputInfo->crtc!=None?XRRGetCrtcInfo(display,screenResources,outputInfo->crtc):0;
				
				/* Check if this output is the primary output: */
				bool primary=false;
				if(primaryMonitor!=0)
					{
					for(int moutput=0;moutput<primaryMonitor->noutput&&!primary;++moutput)
						primary=primaryMonitor->outputs[moutput]==screenResources->outputs[output];
					}
				
				if(activeOnly)
					{
					if(crtcInfo!=0)
						{
						/* Get a mode descriptor for the active CRTC's mode: */
						XRRModeInfo* modeInfo=findMode(screenResources,crtcInfo->mode);
						
						/* Call the testing functor: */
						result=modeFunctor.testMode(outputInfo,modeInfo,crtcInfo,primary);
						}
					}
				else
					{
					/* Iterate through all of the output's modes: */
					for(int mode=0;mode<outputInfo->nmode&&result<0;++mode)
						{
						/* Get the mode descriptor: */
						XRRModeInfo* modeInfo=findMode(screenResources,outputInfo->modes[mode]);
						
						/* Check if the output's CRTC is associated with this mode: */
						XRRCrtcInfo* modeCrtcInfo=crtcInfo!=0&&crtcInfo->mode==outputInfo->modes[mode]?crtcInfo:0;
						
						/* Call the testing functor: */
						result=modeFunctor.testMode(outputInfo,modeInfo,modeCrtcInfo,primary);
						}
					}
				
				/* Clean up: */
				if(crtcInfo!=0)
					XRRFreeCrtcInfo(crtcInfo);
				}
			
			/* Clean up: */
			XRRFreeOutputInfo(outputInfo);
			}
		
		/* Clean up: */
		XRRFreeScreenResources(screenResources);
		}
	
	return modeFunctor.finalize(result);
	}

int main(int argc,char* argv[])
	{
	/* Find the type of the first connected supported HMD: */
	HMDType hmdType=findHMD();
	if(hmdType==NoHMD)
		{
		std::cerr<<"No supported HMD found"<<std::endl;
		return 1;
		}
	
	enum Mode
		{
		GetType,PrintPort,EnableCmd,DisableCmd,PrintGeometry
		};
	
	/* Parse the command line: */
	const char* displayName=getenv("DISPLAY");
	double rateFuzz=0.01;
	Mode mode=GetType;
	double enableRate=hmdRefreshRates[hmdType];
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i]+1,"h")==0)
				{
				std::cout<<"Usage: "<<argv[0]<<" [-display <display name>] [-size <width> <height>] [-rate <rate>] [-rateFuzz <rate fuzz>] [-enableCmd] [-disableCmd] [-printGeometry]"<<std::endl;
				std::cout<<"\t-display <display name> : Connect to the X display of the given name; defaults to standard display"<<std::endl;
				std::cout<<"\t-rateFuzz <rate fuzz>   : Fuzz factor for refresh rate comparisons; defaults to 0.01"<<std::endl;
				std::cout<<"\t-port                   : Print the name of the video port to which the HMD is connected"<<std::endl;
				std::cout<<"\t-enableCmd              : Print an xrandr option list to enable the desired HMD"<<std::endl;
				std::cout<<"\t-rate <refresh rate>    : Refresh rate to use for HMDs that support multiple rates"<<std::endl;
				std::cout<<"\t-disableCmd             : Print an xrandr option list to disable the desired HMD"<<std::endl;
				std::cout<<"\t-printGeometry          : Print the position and size of the HMD's screen in virtual screen coordinates"<<std::endl;
				
				return 0;
				}
			else if(strcasecmp(argv[i]+1,"display")==0)
				{
				++i;
				displayName=argv[i];
				}
			else if(strcasecmp(argv[i]+1,"rateFuzz")==0)
				{
				++i;
				rateFuzz=atof(argv[i]);
				}
			else if(strcasecmp(argv[i]+1,"rate")==0)
				{
				++i;
				enableRate=atof(argv[i]);
				}
			else if(strcasecmp(argv[i]+1,"port")==0)
				mode=PrintPort;
			else if(strcasecmp(argv[i]+1,"enableCmd")==0)
				mode=EnableCmd;
			else if(strcasecmp(argv[i]+1,"disableCmd")==0)
				mode=DisableCmd;
			else if(strcasecmp(argv[i]+1,"printGeometry")==0)
				mode=PrintGeometry;
			else
				std::cerr<<"Ignoring unrecognized option "<<argv[i]<<std::endl;
			}
		}
	
	if(displayName==0)
		{
		std::cerr<<"FindHMD: No display name provided"<<std::endl;
		return 1;
		}
	
	/* Open a connection to the X display: */
	Display* display=XOpenDisplay(displayName);
	if(display==0)
		{
		std::cerr<<"FindHMD: Unable to connect to display "<<displayName<<std::endl;
		return 1;
		}
	
	/* Set the error handler: */
	XSetErrorHandler(errorHandler);
	
	/* Query the Xrandr extension: */
	int xrandrEventBase;
	int xrandrMajor,xrandrMinor;
	if(!XRRQueryExtension(display,&xrandrEventBase,&xrandrErrorBase)||XRRQueryVersion(display,&xrandrMajor,&xrandrMinor)==0)
		{
		std::cerr<<"FindHMD: Display "<<displayName<<" does not support RANDR extension"<<std::endl;
		XCloseDisplay(display);
		return 1;
		}
	
	#if VERBOSE
	std::cout<<"FindHMD: Found RANDR extension version "<<xrandrMajor<<'.'<<xrandrMinor<<std::endl;
	#endif
	
	/* Do the thing: */
	int result=0;
	switch(mode)
		{
		case GetType:
			std::cout<<hmdTypes[hmdType]<<std::endl;
			break;
		
		case PrintPort:
		case PrintGeometry:
			{
			FindHMDFunctor f(hmdScreenSizes[hmdType],enableRate,rateFuzz,mode==PrintGeometry);
			enumerateModes(display,mode==PrintGeometry,f);
			break;
			}
		
		case EnableCmd:
		case DisableCmd:
			{
			XrandrCommandFunctor f(hmdScreenSizes[hmdType],enableRate,rateFuzz,mode==EnableCmd);
			enumerateModes(display,false,f);
			break;
			}
		}
	
	/* Clean up and return: */
	XCloseDisplay(display);
	return result;
	}
