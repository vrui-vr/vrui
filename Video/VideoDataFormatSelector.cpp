/***********************************************************************
VideoDataFormatSelector - Helper class to set parts of a video data
format.
Copyright (c) 2025 Oliver Kreylos

This file is part of the Basic Video Library (Video).

The Basic Video Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The Basic Video Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Basic Video Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <Video/VideoDataFormatSelector.h>

#include <Misc/StdError.h>
#include <Misc/ValueCoder.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CommandLineParser.icpp>
#include <Math/MathValueCoders.h>

namespace Video {

namespace {

/******************************
Custom option handling classes:
******************************/

class PixelFormatOption:public Misc::CommandLineParser::Option
	{
	/* Elements: */
	private:
	VideoDataFormatSelector& vdfs; // Reference to the video data format selector
	
	/* Constructors and destructors: */
	public:
	PixelFormatOption(VideoDataFormatSelector& sVdfs)
		:Misc::CommandLineParser::Option("Selects a video pixel format as a FourCC code"),
		 vdfs(sVdfs)
		{
		}
	
	/* Methods from class Misc::CommandLineParser::Option: */
	virtual void printArguments(std::ostream& os) const
		{
		os<<" <FourCC code>";
		}
	virtual char** parse(char* arg,char** argPtr,char** argEnd)
		{
		/* Check if a FourCC code is present: */
		if(argPtr==argEnd)
			throw Misc::makeStdErr(0,"%s: Missing pixel format",arg);
		
		/* Check if the FourCC code is four characters long: */
		unsigned int length;
		for(length=0;(*argPtr)[length]!='\0';++length)
			;
		if(length!=4)
			throw Misc::makeStdErr(0,"%s: Pixel format %s does not have four characters",arg,*argPtr);
		
		/* Set the video format: */
		vdfs.setPixelFormat(*argPtr);
		
		return argPtr+1;
		}
	};

class FrameSizeOption:public Misc::CommandLineParser::Option
	{
	/* Elements: */
	private:
	VideoDataFormatSelector& vdfs; // Reference to the video data format selector
	
	/* Constructors and destructors: */
	public:
	FrameSizeOption(VideoDataFormatSelector& sVdfs)
		:Misc::CommandLineParser::Option("Selects a video frame size of <width>x<height> pixels."),
		 vdfs(sVdfs)
		{
		}
	
	/* Methods from class Misc::CommandLineParser::Option: */
	virtual void printArguments(std::ostream& os) const
		{
		os<<" <width> <height>";
		}
	virtual char** parse(char* arg,char** argPtr,char** argEnd)
		{
		/* Parse size components: */
		Size size;
		unsigned int* sPtr=size.getComponents();
		unsigned int* sEnd=sPtr+2;
		while(argPtr!=argEnd&&sPtr!=sEnd)
			{
			try
				{
				/* Convert the argument to unsigned int: */
				*sPtr=Misc::CommandLineParser::convertValue<unsigned int>(*argPtr);
				}
			catch(const std::runtime_error& err)
				{
				/* Wrap the error message: */
				throw Misc::makeStdErr(0,"%s: %s",arg,err.what());
				}
			
			++sPtr;
			++argPtr;
			}
		
		/* Check if all size components were read: */
		if(sPtr!=sEnd)
			throw Misc::makeStdErr(0,"%s: Missing frame size component(s)",arg);
		
		/* Set the video frame size: */
		vdfs.setSize(size);
		
		return argPtr;
		}
	};

class FrameRateOption:public Misc::CommandLineParser::Option
	{
	/* Elements: */
	private:
	VideoDataFormatSelector& vdfs; // Reference to the video data format selector
	
	/* Constructors and destructors: */
	public:
	FrameRateOption(VideoDataFormatSelector& sVdfs)
		:Misc::CommandLineParser::Option("Selects a video frame rate as a rational number"),
		 vdfs(sVdfs)
		{
		}
	
	/* Methods from class Misc::CommandLineParser::Option: */
	virtual void printArguments(std::ostream& os) const
		{
		os<<" <frame rate>";
		}
	virtual char** parse(char* arg,char** argPtr,char** argEnd)
		{
		/* Check if a frame rate is present: */
		if(argPtr==argEnd)
			throw Misc::makeStdErr(0,"%s: Missing frame rate",arg);
		
		try
			{
			/* Convert the argument to a rational: */
			vdfs.setFrameInterval(Misc::CommandLineParser::convertValue<Math::Rational>(*argPtr).inverse());
			}
		catch(const std::runtime_error& err)
			{
			/* Wrap the error message: */
			throw Misc::makeStdErr(0,"%s: %s",arg,err.what());
			}
		
		return argPtr+1;
		}
	};

class FrameIntervalOption:public Misc::CommandLineParser::Option
	{
	/* Elements: */
	private:
	VideoDataFormatSelector& vdfs; // Reference to the video data format selector
	
	/* Constructors and destructors: */
	public:
	FrameIntervalOption(VideoDataFormatSelector& sVdfs)
		:Misc::CommandLineParser::Option("Selects a video frame interval as a rational number"),
		 vdfs(sVdfs)
		{
		}
	
	/* Methods from class Misc::CommandLineParser::Option: */
	virtual void printArguments(std::ostream& os) const
		{
		os<<" <frame interval>";
		}
	virtual char** parse(char* arg,char** argPtr,char** argEnd)
		{
		/* Check if a frame interval is present: */
		if(argPtr==argEnd)
			throw Misc::makeStdErr(0,"%s: Missing frame interval",arg);
		
		try
			{
			/* Convert the argument to a rational: */
			vdfs.setFrameInterval(Misc::CommandLineParser::convertValue<Math::Rational>(*argPtr));
			}
		catch(const std::runtime_error& err)
			{
			/* Wrap the error message: */
			throw Misc::makeStdErr(0,"%s: %s",arg,err.what());
			}
		
		return argPtr+1;
		}
	};

}

/****************************************
Methods of class VideoDataFormatSelector:
****************************************/

void VideoDataFormatSelector::addToParser(Misc::CommandLineParser& commandLineParser)
	{
	commandLineParser.addOptionHandler("videoPixelFormat","vpf",new PixelFormatOption(*this));
	commandLineParser.addOptionHandler("videoFrameSize","vfs",new FrameSizeOption(*this));
	commandLineParser.addOptionHandler("videoFrameRate","vfr",new FrameRateOption(*this));
	commandLineParser.addOptionHandler("videoFrameInterval","vfi",new FrameIntervalOption(*this));
	}

}
