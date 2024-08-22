/***********************************************************************
RemoteDevice - Class to daisy-chain device servers on remote machines.
Copyright (c) 2002-2024 Oliver Kreylos

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

#include <VRDeviceDaemon/VRDevices/RemoteDevice.h>

#include <Misc/SizedTypes.h>
#include <Misc/StdError.h>
#include <Misc/Time.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Comm/TCPPipe.h>

#include <VRDeviceDaemon/VRCalibrator.h>
#include <VRDeviceDaemon/VRDeviceManager.h>

/*****************************
Methods of class RemoteDevice:
*****************************/

void RemoteDevice::deviceThreadMethod(void)
	{
	while(true)
		{
		/* Wait for next message: */
		if(pipe->read<MessageIdType>()==PACKET_REPLY) // Just ignore any other messages
			{
			/* Read current server state: */
			state.read(*pipe,false,false);
			
			/* Copy new state into device manager: */
			for(int i=0;i<state.getNumValuators();++i)
				setValuatorState(i,state.getValuatorState(i));
			for(int i=0;i<state.getNumButtons();++i)
				setButtonState(i,state.getButtonState(i));
			for(int i=0;i<state.getNumTrackers();++i)
				setTrackerState(i,state.getTrackerState(i));
			}
		}
	}

RemoteDevice::RemoteDevice(VRDevice::Factory* sFactory,VRDeviceManager* sDeviceManager,Misc::ConfigurationFile& configFile)
	:VRDevice(sFactory,sDeviceManager,configFile),
	 pipe(new Comm::TCPPipe(configFile.retrieveString("./serverName").c_str(),configFile.retrieveValue<int>("./serverPort")))
	{
	/* Initiate connection: */
	#ifdef VERBOSE
	printf("RemoteDevice: Connecting to remote device server\n");
	fflush(stdout);
	#endif
	pipe->write(MessageIdType(CONNECT_REQUEST));
	pipe->write(Misc::UInt32(protocolVersionNumber));
	pipe->flush();
	
	/* Wait for server's reply: */
	if(!pipe->waitForData(Misc::Time(10,0))) // Throw exception if reply does not arrive in time
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Timeout while waiting for CONNECT_REPLY");
	if(pipe->read<MessageIdType>()!=CONNECT_REPLY)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Mismatching message while waiting for CONNECT_REPLY");
	serverProtocolVersionNumber=pipe->read<Misc::UInt32>();
	
	/* Check server version number for compatibility: */
	if(serverProtocolVersionNumber<1U||serverProtocolVersionNumber>protocolVersionNumber)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unsupported server protocol version");
	
	/* Read server's layout and initialize current state: */
	state.readLayout(*pipe);
	#ifdef VERBOSE
	printf("RemoteDevice: Serving %d trackers, %d buttons, %d valuators\n",state.getNumTrackers(),state.getNumButtons(),state.getNumValuators());
	fflush(stdout);
	#endif
	setNumTrackers(state.getNumTrackers(),configFile);
	setNumButtons(state.getNumButtons(),configFile);
	setNumValuators(state.getNumValuators(),configFile);
	}

RemoteDevice::~RemoteDevice(void)
	{
	/* Disconnect from device server: */
	pipe->write(MessageIdType(DISCONNECT_REQUEST));
	}

void RemoteDevice::start(void)
	{
	/* Start device communication thread: */
	startDeviceThread();
	
	/* Activate device server: */
	pipe->write(MessageIdType(ACTIVATE_REQUEST));
	pipe->write(MessageIdType(STARTSTREAM_REQUEST));
	pipe->flush();
	}

void RemoteDevice::stop(void)
	{
	/* Deactivate device server: */
	pipe->write(MessageIdType(STOPSTREAM_REQUEST));
	pipe->write(MessageIdType(DEACTIVATE_REQUEST));
	pipe->flush();
	
	/* Stop device communication thread: */
	stopDeviceThread();
	}

/*************************************
Object creation/destruction functions:
*************************************/

extern "C" VRDevice* createObjectRemoteDevice(VRFactory<VRDevice>* factory,VRFactoryManager<VRDevice>* factoryManager,Misc::ConfigurationFile& configFile)
	{
	VRDeviceManager* deviceManager=static_cast<VRDeviceManager::DeviceFactoryManager*>(factoryManager)->getDeviceManager();
	return new RemoteDevice(factory,deviceManager,configFile);
	}

extern "C" void destroyObjectRemoteDevice(VRDevice* device,VRFactory<VRDevice>* factory,VRFactoryManager<VRDevice>* factoryManager)
	{
	delete device;
	}
