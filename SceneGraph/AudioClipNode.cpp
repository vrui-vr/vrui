/***********************************************************************
AudioClipNode - Class for audio clips that can be played by Sound nodes.
Copyright (c) 2021-2024 Oliver Kreylos

This file is part of the Simple Scene Graph Renderer (SceneGraph).

The Simple Scene Graph Renderer is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Simple Scene Graph Renderer is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Simple Scene Graph Renderer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <SceneGraph/AudioClipNode.h>

#include <string.h>
#include <stdexcept>
#include <Misc/StdError.h>
#include <Misc/VarIntMarshaller.h>
#include <Misc/FileNameExtensions.h>
#include <IO/File.h>
#include <IO/SeekableFile.h>
#include <AL/ALContextData.h>
#include <Sound/SoundDataFormat.h>
#include <Sound/WAVFile.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SceneGraphReader.h>
#include <SceneGraph/SceneGraphWriter.h>
#include <SceneGraph/ALRenderState.h>

#if !ALSUPPORT_CONFIG_HAVE_OPENAL

/* Define some OpenAL constants and enumerants so that not everything has to be bracketed: */
#define AL_FORMAT_MONO8    0x1100
#define AL_FORMAT_MONO16   0x1101
#define AL_FORMAT_STEREO8  0x1102
#define AL_FORMAT_STEREO16 0x1103

#endif

namespace SceneGraph {

/****************************************
Methods of class AudioClipNode::DataItem:
****************************************/

AudioClipNode::DataItem::DataItem(void)
	:bufferId(0),version(0)
	{
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	
	/* Create an audio buffer: */
	alGenBuffers(1,&bufferId);
	
	#endif
	}

AudioClipNode::DataItem::~DataItem(void)
	{
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	
	/* Destroy the audio buffer: */
	alDeleteBuffers(1,&bufferId);
	
	#endif
	}

/**************************************
Static elements of class AudioClipNode:
**************************************/

const char* AudioClipNode::className="AudioClip";

/******************************
Methods of class AudioClipNode:
******************************/

void AudioClipNode::loadSoundFile(IO::Directory& baseDirectory)
	{
	/* Determine the sound file's format: */
	if(Misc::hasCaseExtension(url.getValue(0).c_str(),".wav"))
		soundFileFormat=0;
	else
		soundFileFormat=1;
	
	if(soundFileFormat<1)
		{
		/* Load the sound file into memory: */
		soundFile=new IO::VariableMemoryFile;
		IO::FilePtr sourceSoundFile=baseDirectory.openFile(url.getValue(0).c_str());
		
		/* Copy the source sound file's contents to the sound file: */
		while(true)
			{
			/* Read a chunk of the source sound file into its internal read buffer: */
			void* buffer;
			size_t bufferSize=sourceSoundFile->readInBuffer(buffer);
			
			/* Bail out if the file is all read: */
			if(bufferSize==0)
				break;
			
			/* Write the source sound file's buffer contents to the sound file: */
			soundFile->writeRaw(buffer,bufferSize);
			}
		soundFile->flush();
		}
	
	/* Invalidate the cached sound waveform: */
	++version;
	}

AudioClipNode::AudioClipNode(void)
	:loop(false),pitch(1),startTime(0),stopTime(0),
	 soundFileFormat(1),
	 version(0)
	{
	}

const char* AudioClipNode::getClassName(void) const
	{
	return className;
	}

void AudioClipNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"description")==0)
		vrmlFile.parseField(description);
	else if(strcmp(fieldName,"loop")==0)
		vrmlFile.parseField(loop);
	else if(strcmp(fieldName,"pitch")==0)
		vrmlFile.parseField(pitch);
	else if(strcmp(fieldName,"startTime")==0)
		vrmlFile.parseField(startTime);
	else if(strcmp(fieldName,"stopTime")==0)
		vrmlFile.parseField(stopTime);
	else if(strcmp(fieldName,"url")==0)
		{
		vrmlFile.parseField(url);
		
		if(!url.getValues().empty())
			{
			/* Immediately load the sound file referenced by the url field: */
			loadSoundFile(vrmlFile.getBaseDirectory());
			}
		else
			{
			/* Reset the sound file: */
			soundFileFormat=1;
			soundFile=0;
			++version;
			}
		}
	else
		Node::parseField(fieldName,vrmlFile);
	}

void AudioClipNode::update(void)
	{
	/* Clamp pitch field: */
	if(pitch.getValue()<Scalar(1)/Scalar(256))
		pitch.setValue(Scalar(1)/Scalar(256));
	}

void AudioClipNode::read(SceneGraphReader& reader)
	{
	/* Read all fields: */
	reader.readField(description);
	reader.readField(loop);
	reader.readField(pitch);
	reader.readField(startTime);
	reader.readField(stopTime);
	url.clearValues();
	
	/*********************************************************************
	Do not read the URL field, but read the contents of the referenced
	sound file instead.
	*********************************************************************/
	
	/* Read the sound file's format from the source file: */
	soundFileFormat=reader.getFile().read<Misc::UInt8>();
	
	/* Read the sound file's content if the format is valid: */
	if(soundFileFormat<1)
		{
		/* Read the size of the sound file from the source file: */
		size_t soundFileSize=Misc::readVarInt32(reader.getFile());
		
		/* Read the sound file: */
		soundFile=new IO::VariableMemoryFile;
		while(soundFileSize>0)
			{
			/* Read a chunk of the source file into its internal read buffer: */
			void* buffer;
			size_t bufferSize=reader.getFile().readInBuffer(buffer,soundFileSize);
			soundFileSize-=bufferSize;
			
			/* Bail out if the file is all read: */
			if(bufferSize==0)
				break;
			
			/* Write the source file's buffer contents to the sound file: */
			soundFile->writeRaw(buffer,bufferSize);
			}
		soundFile->flush();
		}
	else
		soundFile=0;
	
	/* Invalidate the cached sound waveform: */
	++version;
	}

void AudioClipNode::write(SceneGraphWriter& writer) const
	{
	/* Write all fields: */
	writer.writeField(description);
	writer.writeField(loop);
	writer.writeField(pitch);
	writer.writeField(startTime);
	writer.writeField(stopTime);
	
	/*********************************************************************
	Do not write the URL field, but write the contents of the referenced
	audio file instead.
	*********************************************************************/
	
	/* Write the sound file's format to the destination file: */
	writer.getFile().write(Misc::UInt8(soundFileFormat));
	
	/* Write the sound file's contents if the sound file format is valid: */
	if(soundFileFormat<1)
		{
		/* Write the size of the sound file to the destination file: */
		Misc::writeVarInt32(soundFile->getDataSize(),writer.getFile());
		
		/* Copy the sound file's contents to the destination file: */
		IO::FilePtr reader=soundFile->getReader();
		while(true)
			{
			/* Read a chunk of the sound file into its internal read buffer: */
			void* buffer;
			size_t bufferSize=reader->readInBuffer(buffer);
			
			/* Bail out if the file is all read: */
			if(bufferSize==0)
				break;
			
			/* Write the sound file's buffer contents to the destination file: */
			writer.getFile().writeRaw(buffer,bufferSize);
			}
		}
	}

void AudioClipNode::initContext(ALContextData& contextData) const
	{
	/* Create a data item and store it in the AL context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	}

void AudioClipNode::setUrl(const std::string& newUrl,IO::Directory& baseDirectory)
	{
	/* Store the URL and load the referenced sound file: */
	url.setValue(newUrl);
	loadSoundFile(baseDirectory);
	}

void AudioClipNode::setUrl(const std::string& newUrl)
	{
	/* Store the URL and load the referenced sound file: */
	url.setValue(newUrl);
	loadSoundFile(*IO::Directory::getCurrent());
	}

ALuint AudioClipNode::getBufferObject(ALRenderState& renderState) const
	{
	if(url.getNumValues()>0)
		{
		/* Acess the context data item: */
		DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
		
		/* Check if the buffer object needs to be updated: */
		if(dataItem->version!=version)
			{
			/* Read the sound file based on its format: */
			if(soundFileFormat==0)
				{
				/* Read the sound file as aWAV file: */
				Sound::WAVFile wav(soundFile->getReader());
				
				/* Check if the WAV file's sound format is OpenAL-compatible: */
				const Sound::SoundDataFormat& sdf=wav.getFormat();
				size_t frameSize=sdf.bytesPerSample*sdf.samplesPerFrame;
				ALenum bufferFormat=AL_NONE;
				if(sdf.bitsPerSample==8&&!sdf.signedSamples)
					{
					if(sdf.samplesPerFrame==1&&frameSize==1)
						bufferFormat=AL_FORMAT_MONO8;
					else if(sdf.samplesPerFrame==2&&frameSize==2)
						bufferFormat=AL_FORMAT_STEREO8;
					}
				else if(sdf.bitsPerSample==16&&sdf.signedSamples&&sdf.sampleEndianness==Sound::SoundDataFormat::LittleEndian)
					{
					if(sdf.samplesPerFrame==1&&frameSize==2)
						bufferFormat=AL_FORMAT_MONO16;
					else if(sdf.samplesPerFrame==2&&frameSize==4)
						bufferFormat=AL_FORMAT_STEREO16;
					}
				if(bufferFormat==AL_NONE)
					throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Sound file has unsupported sound data format");
				
				/* Read the WAV file's contents into an OpenAL buffer: */
				size_t numFrames=wav.getNumAudioFrames();
				
				void* buffer=malloc(numFrames*frameSize);
				wav.readAudioFrames(buffer,numFrames);
				#if ALSUPPORT_CONFIG_HAVE_OPENAL
				alBufferData(dataItem->bufferId,bufferFormat,buffer,numFrames*frameSize,sdf.framesPerSecond);
				#endif
				free(buffer);
				}
			else
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Sound file has unsupported file format");
			
			/* Mark the buffer object as up-to-date: */
			dataItem->version=version;
			}
		
		return dataItem->bufferId;
		}
	else
		return 0;
	}

}
