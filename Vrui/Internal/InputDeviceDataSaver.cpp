/***********************************************************************
InputDeviceDataSaver - Class to save input device data to a file for
later playback. The class is derived from Vrui::InputDeviceAdapter to
be able to hook it into the input device manager and simplify processing
in Vrui's main loop.
Copyright (c) 2004-2026 Oliver Kreylos

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

#include <Vrui/Internal/InputDeviceDataSaver.h>

#include <Misc/SizedTypes.h>
#include <Misc/StringMarshaller.h>
#include <Misc/MessageLogger.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <IO/File.h>
#include <IO/OpenFile.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/GeometryMarshallers.h>
#include <Sound/SoundDataFormat.h>
#include <Sound/SoundRecorder.h>
#include <Vrui/Types.h>
#include <Vrui/Vrui.h>
#include <Vrui/EnvironmentDefinition.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputDeviceFeature.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/TextEventDispatcher.h>

namespace Vrui {

/*************************************
Methods of class InputDeviceDataSaver:
*************************************/

void InputDeviceDataSaver::inputDeviceStateChangedCallback(InputGraphManager::InputDeviceStateChangeCallbackData* cbData)
	{
	/* Find the changed device in the list of saved devices: */
	for(int deviceIndex=0;deviceIndex<numSavedInputDevices;++deviceIndex)
		if(savedInputDevices[deviceIndex]==cbData->inputDevice)
			{
			/* Set the device's new enabled flag: */
			newInputDeviceEnableds[deviceIndex]=cbData->newEnabled;
			break;
			}
	}

InputDeviceDataSaver::InputDeviceDataSaver(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& configFileSection,unsigned int randomSeed)
	:InputDeviceAdapter(sInputDeviceManager),
	 inputGraphManager(inputDeviceManager->getInputGraphManager()),
	 numSavedInputDevices(inputDeviceManager->getNumInputDevices()),
	 savedInputDevices(new InputDevice*[numSavedInputDevices]),
	 inputDeviceEnableds(new bool[numSavedInputDevices]),
	 newInputDeviceEnableds(new bool[numSavedInputDevices])
	{
	/*********************************************************************
	Add this input device data saver as an input device adapter to the
	input device manager. It won't add any input devices and will appear
	transparent, but its updateInputDevices method will be called
	immediately after all other input device adapters have updated their
	devices, meaning at exactly the right time. Win-win!
	*********************************************************************/
	
	inputDeviceManager->addAdapter(this);
		
	/* Open the common base directory: */
	IO::DirectoryPtr baseDirectory=IO::openDirectory(configFileSection.retrieveString("./baseDirectory",".").c_str());
	
	/* Open the input device data file relative to the base directory: */
	inputDeviceDataFile=baseDirectory->openFile(baseDirectory->createNumberedFileName(configFileSection.retrieveString("./inputDeviceDataFileName").c_str(),4).c_str(),IO::File::WriteOnly);
	
	/* Write a file identification header: */
	inputDeviceDataFile->setEndianness(Misc::LittleEndian);
	static const char* fileHeader="Vrui Input Device Data File v8.0\n";
	inputDeviceDataFile->write(fileHeader,34);
	
	/* Save the random number seed: */
	inputDeviceDataFile->write<Misc::UInt32>(randomSeed);
	
	/* Save the environment definition: */
	getEnvironmentDefinition().write(*inputDeviceDataFile);
	
	/* Save the number of input devices: */
	inputDeviceDataFile->write(Misc::UInt8(numSavedInputDevices));
	
	/* Save layout and feature names of all input devices in the input device manager: */
	for(int deviceIndex=0;deviceIndex<numSavedInputDevices;++deviceIndex)
		{
		/* Get pointer to the input device: */
		InputDevice* device=savedInputDevices[deviceIndex]=inputDeviceManager->getInputDevice(deviceIndex);
		
		/* Save input device's name and layout: */
		Misc::writeCString(device->getDeviceName(),*inputDeviceDataFile);
		inputDeviceDataFile->write(Misc::UInt8(device->getTrackType()));
		inputDeviceDataFile->write(Misc::UInt16(device->getNumButtons()));
		inputDeviceDataFile->write(Misc::UInt16(device->getNumValuators()));
		
		/* Save input device's feature names: */
		for(int i=0;i<device->getNumFeatures();++i)
			{
			std::string featureName=inputDeviceManager->getFeatureName(InputDeviceFeature(device,i));
			Misc::writeCppString(featureName,*inputDeviceDataFile);
			}
		
		/* Save the input device's handle transformation: */
		const ONTransform& handleTransform=inputDeviceManager->getHandleTransform(device);
		if(handleTransform!=ONTransform::identity)
			{
			inputDeviceDataFile->write(Misc::UInt8(1));
			Misc::Marshaller<ONTransform>::write(handleTransform,*inputDeviceDataFile);
			}
		else
			inputDeviceDataFile->write(Misc::UInt8(0));
		
		/* Save and remember the device's enabled state: */
		newInputDeviceEnableds[deviceIndex]=inputDeviceEnableds[deviceIndex]=inputGraphManager->isEnabled(device);
		inputDeviceDataFile->write(Misc::UInt8(inputDeviceEnableds[deviceIndex]?1:0));
		}
	
	/* Register a callback with the input graph manager: */
	inputGraphManager->getInputDeviceStateChangeCallbacks().add(this,&InputDeviceDataSaver::inputDeviceStateChangedCallback);
	
	/* Check if the user wants to record a commentary track: */
	std::string soundFileName=configFileSection.retrieveString("./soundFileName","");
	if(!soundFileName.empty())
		{
		try
			{
			#if 0
			/* Create a sound data format for recording: */
			Sound::SoundDataFormat soundFormat;
			configFileSection.updateValue("./sampleResolution",soundFormat.bitsPerSample);
			configFileSection.updateValue("./numChannels",soundFormat.samplesPerFrame);
			configFileSection.updateValue("./sampleRate",soundFormat.framesPerSecond);
			
			/* Create a sound recorder for the given sound file name: */
			std::string soundDeviceName=configFileSection.retrieveValue<std::string>("./soundDeviceName","default");
			soundFileName=baseDirectory->getPath(baseDirectory->createNumberedFileName(soundFileName.c_str(),4).c_str());
			soundRecorder=new Sound::SoundRecorder(soundDeviceName.c_str(),soundFormat,soundFileName.c_str());
			#endif
			}
		catch(const std::runtime_error& err)
			{
			/* Print a message, but carry on: */
			Misc::sourcedConsoleWarning(__PRETTY_FUNCTION__,"Disabling sound recording due to exception %s",err.what());
			}
		}
	}

InputDeviceDataSaver::~InputDeviceDataSaver(void)
	{
	/* Log the total recording time as a convenience: */
	Misc::formattedLogNote("Vrui::InputDeviceDataSaver: Total recording time: %fs",getApplicationTime());
	
	/* Unregister the input device state change callback from the input graph manager: */
	inputGraphManager->getInputDeviceStateChangeCallbacks().remove(this,&InputDeviceDataSaver::inputDeviceStateChangedCallback);
	
	/* Delete the state tracking arrays: */
	delete[] inputDeviceEnableds;
	delete[] newInputDeviceEnableds;
	delete[] savedInputDevices;
	}

void InputDeviceDataSaver::prepareMainLoop(void)
	{
	#if 0
	try
		{
		/* Start recording sound now, if requested: */
		if(soundRecorder!=0)
			soundRecorder->start();
		}
	catch(const std::runtime_error& err)
		{
		Misc::sourcedConsoleWarning(__PRETTY_FUNCTION__,"Disabling sound recording due to exception %s",err.what());
		delete soundRecorder;
		soundRecorder=0;
		}
	#endif
	}

void InputDeviceDataSaver::updateInputDevices(void)
	{
	/* Write the current application time to the file: */
	inputDeviceDataFile->write(Misc::Float64(getApplicationTime()));
	
	/* Write the change lists of all saved input devices to the file: */
	for(int deviceIndex=0;deviceIndex<numSavedInputDevices;++deviceIndex)
		if(inputDeviceEnableds[deviceIndex]!=newInputDeviceEnableds[deviceIndex]||savedInputDevices[deviceIndex]->hasChanges())
			{
			/* Write the input device's index and enabled state: */
			inputDeviceDataFile->write(Misc::UInt8(deviceIndex));
			inputDeviceDataFile->write(Misc::UInt8(newInputDeviceEnableds[deviceIndex]));
			
			/* If the input device is enabled, write its change list: */
			if(newInputDeviceEnableds[deviceIndex])
				savedInputDevices[deviceIndex]->writeChanges(*inputDeviceDataFile);
			
			/* Mark the device's enabled state as up-to-date: */
			inputDeviceEnableds[deviceIndex]=newInputDeviceEnableds[deviceIndex];
			}
	
	/* Terminate the list of changes with an invalid input device index: */
	inputDeviceDataFile->write(Misc::UInt8(-1));
	
	/* Write the text event dispatcher's events to the pipe: */
	inputDeviceManager->getTextEventDispatcher()->writeEventQueues(*inputDeviceDataFile);
	}

}
