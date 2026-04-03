/***********************************************************************
VRDeviceServer - Class encapsulating the VR device protocol's server
side.
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

#include <VRDeviceDaemon/VRDeviceServer.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdexcept>
#include <Misc/SizedTypes.h>
#include <Misc/PrintInteger.h>
#include <Misc/FileNameExtensions.h>
#include <Misc/FileTests.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Threads/FunctionCalls.h>
#include <IO/ValueSource.h>
#include <IO/JsonEntityTypes.h>
#include <Comm/TCPPipe.h>
#include <Comm/UNIXPipe.h>
#include <Comm/ListeningTCPSocket.h>
#include <Comm/ListeningUNIXSocket.h>
#include <Comm/HttpPostRequest.h>
#include <Math/Math.h>
#include <Vrui/Internal/Config.h>
#include <Vrui/Internal/VRDeviceDescriptor.h>
#include <Vrui/Internal/HMDConfiguration.h>
#include <Vrui/Internal/VRBaseStation.h>

#include <VRDeviceDaemon/Config.h>

#define VRDEVICEDAEMON_DEBUG_PROTOCOL 0

namespace {

/**************
Helper classes:
**************/

enum State
	{
	START=0,CONNECTED,ACTIVE,STREAMING
	};

}

/********************************************
Methods of class VRDeviceServer::ClientState:
********************************************/

VRDeviceServer::ClientState::ClientState(Comm::PipePtr sPipe)
	:pipe(sPipe),
	 protocolVersion(Vrui::VRDeviceProtocol::protocolVersionNumber),clientExpectsTimeStamps(true),
	 state(START)
	{
	#ifdef VERBOSE
	Comm::TCPPipe* tcpPipe=dynamic_cast<Comm::TCPPipe*>(pipe.getPointer());
	if(tcpPipe!=0)
		{
		/* Assemble the client name: */
		clientName=tcpPipe->getPeerHostName();
		clientName.push_back(':');
		char portId[10];
		clientName.append(Misc::print(tcpPipe->getPeerPortId(),portId+sizeof(portId)-1));
		}
	else
		{
		/* UNIX domain clients are anonymous: */
		clientName="UNIX domain client";
		}
	#endif
	}

/***************************************
Static elements of class VRDeviceServer:
***************************************/

const unsigned int VRDeviceServer::httpProtocolVersion=1U;

/*******************************
Methods of class VRDeviceServer:
*******************************/

void VRDeviceServer::goInactive(void)
	{
	#ifdef VERBOSE
	printf("VRDeviceServer: Entering inactive state\n");
	fflush(stdout);
	#endif
	
	/* Stop processing events in the device manager: */
	deviceManager->stop();
	
	/* Set and re-enable the suspend timer if there is one: */
	if(suspendTimer!=0)
		suspendTimer->setTimeout(Threads::RunLoop::Time()+suspendInterval,true);
	}

void VRDeviceServer::goActive(void)
	{
	#ifdef VERBOSE
	printf("VRDeviceServer: Entering active state\n");
	fflush(stdout);
	#endif
	
	/* Disable the suspend timer if there is one: */
	if(suspendTimer!=0)
		suspendTimer->disable();
	
	/* Start processing events in the device manager: */
	deviceManager->start();
	}

void VRDeviceServer::disconnectClient(VRDeviceServer::ClientState* client,bool removeFromList)
	{
	/* Check if the client is still streaming or active: */
	if(client->state>=STREAMING)
		--numStreamingClients;
	if(client->state>=ACTIVE)
		{
		/* Decrease the number of active clients and go to inactive state if the number reaches zero: */
		if(--numActiveClients==0)
			goInactive();
		}
	
	if(removeFromList)
		{
		/* Remove the client from the client list and delete it: */
		delete clientStates.remove(client);
		}
	}

void VRDeviceServer::disconnectClientOnError(VRDeviceServer::ClientStateList::iterator csIt,const std::runtime_error& err)
	{
	/* Print error message to stderr: */
	#ifdef VERBOSE
	fprintf(stderr,"VRDeviceServer: Disconnecting client %s due to exception %s\n",csIt->clientName.c_str(),err.what());
	#else
	fprintf(stderr,"VRDeviceServer: Disconnecting client due to exception %s\n",err.what());
	#endif
	fflush(stderr);
	
	/* Disconnect the client, but don't remove it from the list (faster to do it through the iterator we have): */
	disconnectClient(&*csIt,false);
	
	/* Remove the disconnected client from the list and delete it: */
	delete clientStates.remove(csIt);
	}

void VRDeviceServer::environmentDefinitionUpdated(VRDeviceServer::ClientState* updatingClient)
	{
	/* Send the new environment definition to all clients except the one who updated it: */
	for(ClientStateList::iterator csIt=clientStates.begin();csIt!=clientStates.end();++csIt)
		{
		ClientState& client=*csIt;
		if(&client!=updatingClient&&client.protocolVersion>=13U)
			{
			try
				{
				/* Send environment definition update notification message: */
				client.pipe->write(MessageIdType(ENVIRONMENTDEFINITION_UPDATE_NOTIFICATION));
				environmentDefinition.write(*client.pipe);
				client.pipe->flush();
				}
			catch(const std::runtime_error& err)
				{
				/* Disconnect the client and signal an error: */
				disconnectClientOnError(csIt,err);
				--csIt;
				}
			}
		}
	}

void VRDeviceServer::clientMessage(Threads::RunLoop::IOWatcher::Event& event,VRDeviceServer::ClientState* client)
	{
	try
		{
		/* Read some data from the socket into the socket's read buffer and check if the client hung up: */
		if(client->pipe->readSomeData()==0)
			throw std::runtime_error("Client terminated connection");
		
		/* Process messages as long as there is data in the read buffer: */
		while(client->pipe->canReadImmediately())
			{
			#if VRDEVICEDAEMON_DEBUG_PROTOCOL
			printf("Reading message..."); fflush(stdout);
			#endif
			
			/* Read the next message from the client: */
			MessageIdType message=client->pipe->read<MessageIdType>();
			
			#if VRDEVICEDAEMON_DEBUG_PROTOCOL
			printf(" done, %u\n",(unsigned int)message);
			#endif
			
			/* Run the client state machine: */
			if(message==CONNECT_REQUEST)
				{
				if(client->state!=START)
					throw std::runtime_error("CONNECT_REQUEST outside START state");
				
				#if VRDEVICEDAEMON_DEBUG_PROTOCOL
				printf("Reading protocol version..."); fflush(stdout);
				#endif
				
				/* Read client's protocol version number: */
				client->protocolVersion=client->pipe->read<Misc::UInt32>();
				
				#if VRDEVICEDAEMON_DEBUG_PROTOCOL
				printf(" done, %u\n",client->protocolVersion);
				#endif
				
				#if VRDEVICEDAEMON_DEBUG_PROTOCOL
				printf("Sending connect reply..."); fflush(stdout);
				#endif
				
				/* Send connect reply message: */
				client->pipe->write(MessageIdType(CONNECT_REPLY));
				if(client->protocolVersion>protocolVersionNumber)
					client->protocolVersion=protocolVersionNumber;
				client->pipe->write(Misc::UInt32(client->protocolVersion));
				
				/* Send server layout: */
				state.writeLayout(*client->pipe);
				
				/* Check if the client expects virtual device descriptors: */
				if(client->protocolVersion>=2U)
					{
					/* Send the layout of all virtual devices: */
					client->pipe->write(Misc::SInt32(deviceManager->getNumVirtualDevices()));
					for(int deviceIndex=0;deviceIndex<deviceManager->getNumVirtualDevices();++deviceIndex)
						deviceManager->getVirtualDevice(deviceIndex).write(*client->pipe,client->protocolVersion);
					}
				
				/* Check if the client expects tracker state time stamps: */
				client->clientExpectsTimeStamps=client->protocolVersion>=3U;
				
				/* Check if the client expects device battery states: */
				if(client->protocolVersion>=5U)
					{
					/* Send all current device battery states: */
					Threads::Mutex::Lock batteryStateLock(batteryStateMutex);
					for(std::vector<Vrui::BatteryState>::const_iterator bsIt=batteryStates.begin();bsIt!=batteryStates.end();++bsIt)
						bsIt->write(*client->pipe);
					}
				
				/* Check if the client expects HMD configurations: */
				if(client->protocolVersion>=4U)
					{
					/* Send all current HMD configurations to the new client: */
					Threads::Mutex::Lock hmdConfigurationLock(hmdConfigurationMutex);
					client->pipe->write(Misc::UInt32(hmdConfigurations.size()));
					for(std::vector<Vrui::HMDConfiguration*>::const_iterator hcIt=hmdConfigurations.begin();hcIt!=hmdConfigurations.end();++hcIt)
						{
						/* Send the full configuration to the client: */
						(*hcIt)->write(0U,0U,0U,*client->pipe);
						}
					
					/* Check if the client expects eye rotations: */
					if(client->protocolVersion>=10U)
						{
						for(std::vector<Vrui::HMDConfiguration*>::const_iterator hcIt=hmdConfigurations.begin();hcIt!=hmdConfigurations.end();++hcIt)
							{
							/* Send the eye rotation to the client: */
							(*hcIt)->writeEyeRotation(*client->pipe);
							}
						}
					}
				
				/* Check if the client expects tracker valid flags: */
				client->clientExpectsValidFlags=client->protocolVersion>=5;
				
				/* Check if the client knows about power and haptic features: */
				if(client->protocolVersion>=6U)
					{
					/* Send the number of power and haptic features: */
					client->pipe->write(Misc::UInt32(deviceManager->getNumPowerFeatures()));
					client->pipe->write(Misc::UInt32(deviceManager->getNumHapticFeatures()));
					}
				
				/* Check if the client is connected via a UNIX socket and can use shared memory: */
				Comm::UNIXPipe* unixPipePtr=dynamic_cast<Comm::UNIXPipe*>(client->pipe.getPointer());
				if(unixPipePtr!=0&&client->protocolVersion>=12)
					{
					/* Send the file descriptor to access device state memory to the client: */
					unixPipePtr->writeFd(deviceStateMemoryFd);
					}
				
				/* Finish the reply message: */
				client->pipe->flush();
				
				#if VRDEVICEDAEMON_DEBUG_PROTOCOL
				printf(" done\n");
				#endif
				
				/* Go to connected state: */
				client->state=CONNECTED;
				}
			else if(message==DISCONNECT_REQUEST)
				{
				if(client->state!=CONNECTED)
					throw std::runtime_error("DISCONNECT_REQUEST outside CONNECTED state");
				
				/* Cleanly disconnect this client: */
				#ifdef VERBOSE
				printf("VRDeviceServer: Disconnecting client %s\n",client->clientName.c_str());
				fflush(stdout);
				#endif
				disconnectClient(client,true);
				
				goto doneWithMessages;
				}
			else if(message==ACTIVATE_REQUEST)
				{
				if(client->state!=CONNECTED)
					throw std::runtime_error("ACTIVATE_REQUEST outside CONNECTED state");
				
				/* Increase the number of active clients and go to active state if the number leaves zero: */
				if(numActiveClients++==0)
					goActive();
				
				/* Go to active state: */
				client->state=ACTIVE;
				}
			else if(message==DEACTIVATE_REQUEST)
				{
				if(client->state!=ACTIVE)
					throw std::runtime_error("DEACTIVATE_REQUEST outside ACTIVE state");
				
				/* Decrease the number of active clients and go to inactive state if the number reaches zero: */
				if(--numActiveClients==0)
					goInactive();
				
				/* Go to connected state: */
				client->state=CONNECTED;
				}
			else if(message==PACKET_REQUEST)
				{
				if(client->state!=ACTIVE)
					throw std::runtime_error("PACKET_REQUEST outside ACTIVE state");
				
				#if VRDEVICEDAEMON_DEBUG_PROTOCOL
				printf("Sending packet reply..."); fflush(stdout);
				#endif
				
				/* Send a packet reply message: */
				client->pipe->write(MessageIdType(PACKET_REPLY));
				
				/* Send the current server state to the client: */
				{
				Threads::Mutex::Lock stateLock(stateMutex);
				state.write(*client->pipe,client->clientExpectsTimeStamps,client->clientExpectsValidFlags);
				}
				
				/* Finish the reply message: */
				client->pipe->flush();
				
				#if VRDEVICEDAEMON_DEBUG_PROTOCOL
				printf(" done\n");
				#endif
				}
			else if(message==STARTSTREAM_REQUEST)
				{
				if(client->state!=ACTIVE)
					throw std::runtime_error("STARTSTREAM_REQUEST outside ACTIVE state");
				
				#if VRDEVICEDAEMON_DEBUG_PROTOCOL
				printf("Sending packet reply..."); fflush(stdout);
				#endif
				
				/* Send a packet reply message: */
				client->pipe->write(MessageIdType(PACKET_REPLY));
				
				/* Send the current server state to the client: */
				{
				Threads::Mutex::Lock stateLock(stateMutex);
				state.write(*client->pipe,client->clientExpectsTimeStamps,client->clientExpectsValidFlags);
				}
				
				/* Finish the reply message: */
				client->pipe->flush();
				
				#if VRDEVICEDAEMON_DEBUG_PROTOCOL
				printf(" done\n");
				#endif
				
				/* Increase the number of streaming clients: */
				++numStreamingClients;
				
				/* Go to streaming state: */
				client->state=STREAMING;
				}
			else if(message==STOPSTREAM_REQUEST)
				{
				if(client->state!=STREAMING)
					throw std::runtime_error("STOPSTREAM_REQUEST outside STREAMING state");
				
				/* Send stopstream reply message: */
				client->pipe->write(MessageIdType(STOPSTREAM_REPLY));
				client->pipe->flush();
				
				/* Decrease the number of streaming clients: */
				--numStreamingClients;
				
				/* Go to active state: */
				client->state=ACTIVE;
				}
			else if(message==POWEROFF_REQUEST)
				{
				if(client->state<ACTIVE)
					throw std::runtime_error("POWEROFF_REQUEST outside ACTIVE state");
				
				/* Read the index of the power feature to power off: */
				unsigned int powerFeatureIndex=client->pipe->read<Misc::UInt16>();
				
				/* Power off the requested feature: */
				deviceManager->powerOff(powerFeatureIndex);
				}
			else if(message==HAPTICTICK_REQUEST)
				{
				if(client->state<ACTIVE)
					throw std::runtime_error("HAPTICTICK_REQUEST outside ACTIVE state");
				
				/* Read the index of the haptic feature and the duration of the haptic tick: */
				unsigned int hapticFeatureIndex=client->pipe->read<Misc::UInt16>();
				unsigned int duration=client->pipe->read<Misc::UInt16>();
				unsigned int frequency=1U;
				unsigned int amplitude=255U;
				if(client->protocolVersion>=8U)
					{
					/* Read the haptic tick's frequency and amplitude: */
					frequency=client->pipe->read<Misc::UInt16>();
					amplitude=client->pipe->read<Misc::UInt8>();
					}
				
				/* Request a haptic tick on the requested feature: */
				deviceManager->hapticTick(hapticFeatureIndex,duration,frequency,amplitude);
				}
			else if(message==BASESTATIONS_REQUEST)
				{
				if(client->state<CONNECTED)
					throw std::runtime_error("BASESTATIONS_REQUEST outside CONNECTED state");
					
				/* Send the states of tracking base stations currently registered with the server: */
				client->pipe->write(MessageIdType(BASESTATIONS_REPLY));
				Threads::Mutex::Lock baseStationLock(baseStationMutex);
				
				client->pipe->write(Misc::UInt8(baseStations.size()));
				for(std::vector<Vrui::VRBaseStation>::const_iterator bsIt=baseStations.begin();bsIt!=baseStations.end();++bsIt)
					bsIt->write(*client->pipe);
				
				/* Finish the reply message: */
				client->pipe->flush();
				}
			else if(message==ENVIRONMENTDEFINITION_REQUEST)
				{
				if(client->state<CONNECTED)
					throw std::runtime_error("ENVIRONMENTDEFINITION_REQUEST outside CONNECTED state");
				
				/* Send the current environment definition: */
				client->pipe->write(MessageIdType(ENVIRONMENTDEFINITION_REPLY));
				environmentDefinition.write(*client->pipe);
				
				/* Finish the reply message: */
				client->pipe->flush();
				}
			else if(message==ENVIRONMENTDEFINITION_UPDATE_REQUEST)
				{
				if(client->state<CONNECTED)
					throw std::runtime_error("ENVIRONMENTDEFINITION_UPDATE_REQUEST outside CONNECTED state");
				
				/* Read the new environment definition: */
				environmentDefinition.read(*client->pipe);
				
				/* Notify all other clients that the environment definition has been updated: */
				environmentDefinitionUpdated(client);
				}
			else
				throw std::runtime_error("Invalid message");
			}
		
		doneWithMessages:
		;
		}
	catch(const std::runtime_error& err)
		{
		#ifdef VERBOSE
		printf("VRDeviceServer: Disconnecting client %s due to exception \"%s\"\n",client->clientName.c_str(),err.what());
		fflush(stdout);
		#endif
		disconnectClient(client,true);
		}
	}

void VRDeviceServer::newClientConnection(Threads::RunLoop::IOWatcher::Event& event,Comm::ListeningSocket& listeningSocket)
	{
	#if VRDEVICEDAEMON_DEBUG_PROTOCOL
	printf("Creating new client state..."); fflush(stdout);
	#endif
	
	/* Create a new client state object and add it to the list: */
	ClientState* newClient=new ClientState(listeningSocket.accept());
	
	#if VRDEVICEDAEMON_DEBUG_PROTOCOL
	printf(" done\n");
	#endif
	
	#ifdef VERBOSE
	printf("VRDeviceServer: Connecting new client %s\n",newClient->clientName.c_str());
	fflush(stdout);
	#endif
	
	#if VRDEVICEDAEMON_DEBUG_PROTOCOL
	printf("Adding new client state to list\n");
	#endif
	
	clientStates.add(newClient);
	
	#if VRDEVICEDAEMON_DEBUG_PROTOCOL
	printf("Adding I/O watcher for client's socket\n");
	#endif
	
	/* Create an I/O watcher for the client's communication pipe: */
	newClient->pipeWatcher=runLoop.createIOWatcher(newClient->pipe->getFd(),Threads::RunLoop::IOWatcher::Read,true,*Threads::createFunctionCall(this,&VRDeviceServer::clientMessage,newClient));
	
	#if VRDEVICEDAEMON_DEBUG_PROTOCOL
	printf("Client connected\n");
	#endif
	}

void VRDeviceServer::getServerStatus(IO::JsonObject& replyRoot)
	{
	/* Write versioning information: */
	replyRoot.setProperty("protocolVersion",httpProtocolVersion);
	
	/* Lock affected server state: */
	Threads::Mutex::Lock stateLock(stateMutex);
	Threads::Mutex::Lock batteryStateLock(batteryStateMutex);
	Threads::Mutex::Lock baseStationLock(baseStationMutex);
	
	/* Compose the list of devices: */
	IO::JsonArrayPointer devices=new IO::JsonArray;
	replyRoot.setProperty("devices",*devices);
	
	int powerFeatureIndex=0;
	int numVirtualDevices=deviceManager->getNumVirtualDevices();
	for(int deviceIndex=0;deviceIndex<numVirtualDevices;++deviceIndex)
		{
		const Vrui::VRDeviceDescriptor& vrd=deviceManager->getVirtualDevice(deviceIndex);
		IO::JsonObjectPointer device=new IO::JsonObject;
		devices->addItem(*device);
		
		device->setProperty("name",vrd.name);
		device->setProperty("hasTracker",vrd.trackType!=Vrui::VRDeviceDescriptor::TRACK_NONE);
		if(vrd.trackType!=Vrui::VRDeviceDescriptor::TRACK_NONE)
			{
			device->setProperty("trackerIndex",vrd.trackerIndex);
			device->setProperty("isConnected",deviceManager->isVirtualDeviceConnected(deviceIndex));
			device->setProperty("isTracked",state.getTrackerValid(vrd.trackerIndex));
			if(state.getTrackerValid(vrd.trackerIndex))
				{
				IO::JsonObjectPointer trackerState=new IO::JsonObject;
				device->setProperty("trackerState",*trackerState);
				
				const Vrui::VRDeviceState::TrackerState& ts=state.getTrackerState(vrd.trackerIndex);
				const Vrui::VRDeviceState::TrackerState::PositionOrientation&po=ts.positionOrientation;
				
				IO::JsonArrayPointer translation=new IO::JsonArray;
				trackerState->setProperty("translation",*translation);
				for(int i=0;i<3;++i)
					translation->addItem(po.getTranslation()[i]);
				
				IO::JsonArrayPointer rotation=new IO::JsonArray;
				trackerState->setProperty("rotation",*rotation);
				for(int i=0;i<4;++i)
					rotation->addItem(po.getRotation().getQuaternion()[i]);
				
				IO::JsonArrayPointer linearVelocity=new IO::JsonArray;
				trackerState->setProperty("linearVelocity",*linearVelocity);
				for(int i=0;i<3;++i)
					linearVelocity->addItem(ts.linearVelocity[i]);
				
				IO::JsonArrayPointer angularVelocity=new IO::JsonArray;
				trackerState->setProperty("angularVelocity",*angularVelocity);
				for(int i=0;i<3;++i)
					angularVelocity->addItem(ts.angularVelocity[i]);
				}
			}
		device->setProperty("hasBattery",vrd.hasBattery);
		if(vrd.hasBattery)
			{
			device->setProperty("batteryLevel",batteryStates[deviceIndex].batteryLevel);
			device->setProperty("isCharging",batteryStates[deviceIndex].charging);
			}
		device->setProperty("canPowerOff",vrd.canPowerOff);
		if(vrd.canPowerOff)
			{
			device->setProperty("powerFeatureIndex",powerFeatureIndex);
			++powerFeatureIndex;
			}
		
		if(vrd.numButtons>0)
			{
			IO::JsonArrayPointer buttons=new IO::JsonArray;
			device->setProperty("buttons",*buttons);
			
			for(int buttonIndex=0;buttonIndex<vrd.numButtons;++buttonIndex)
				{
				IO::JsonObjectPointer button=new IO::JsonObject;
				buttons->addItem(*button);
				
				button->setProperty("name",vrd.buttonNames[buttonIndex]);
				button->setProperty("index",vrd.buttonIndices[buttonIndex]);
				button->setProperty("value",state.getButtonState(vrd.buttonIndices[buttonIndex]));
				}
			}
		
		if(vrd.numValuators>0)
			{
			IO::JsonArrayPointer valuators=new IO::JsonArray;
			device->setProperty("valuators",*valuators);
			
			for(int valuatorIndex=0;valuatorIndex<vrd.numValuators;++valuatorIndex)
				{
				IO::JsonObjectPointer valuator=new IO::JsonObject;
				valuators->addItem(*valuator);
				
				valuator->setProperty("name",vrd.valuatorNames[valuatorIndex]);
				valuator->setProperty("index",vrd.valuatorIndices[valuatorIndex]);
				valuator->setProperty("value",state.getValuatorState(vrd.valuatorIndices[valuatorIndex]));
				}
			}
		
		if(vrd.numHapticFeatures>0)
			{
			IO::JsonArrayPointer hapticFeatures=new IO::JsonArray;
			device->setProperty("hapticFeatures",*hapticFeatures);
			
			for(int hapticFeatureIndex=0;hapticFeatureIndex<vrd.numHapticFeatures;++hapticFeatureIndex)
				{
				IO::JsonObjectPointer hapticFeature=new IO::JsonObject;
				hapticFeatures->addItem(*hapticFeature);
				
				hapticFeature->setProperty("name",vrd.hapticFeatureNames[hapticFeatureIndex]);
				hapticFeature->setProperty("index",vrd.hapticFeatureIndices[hapticFeatureIndex]);
				}
			}
		}
	
	/* Compose the list of base stations: */
	if(!baseStations.empty())
		{
		IO::JsonArrayPointer stations=new IO::JsonArray;
		replyRoot.setProperty("baseStations",*stations);
		
		for(std::vector<Vrui::VRBaseStation>::const_iterator bsIt=baseStations.begin();bsIt!=baseStations.end();++bsIt)
			{
			IO::JsonObjectPointer baseStation=new IO::JsonObject;
			stations->addItem(*baseStation);
			
			baseStation->setProperty("serialNumber",bsIt->getSerialNumber());
			
			IO::JsonArrayPointer fov=new IO::JsonArray;
			baseStation->setProperty("fov",*fov);
			for(int i=0;i<4;++i)
				fov->addItem(bsIt->getFov()[i]);
			
			IO::JsonArrayPointer range=new IO::JsonArray;
			baseStation->setProperty("range",*range);
			for(int i=0;i<2;++i)
				range->addItem(bsIt->getRange()[i]);
			
			baseStation->setProperty("isTracking",bsIt->getTracking());
			
			if(bsIt->getTracking())
				{
				IO::JsonObjectPointer positionOrientation=new IO::JsonObject;
				baseStation->setProperty("positionOrientation",*positionOrientation);
				
				const Vrui::VRBaseStation::PositionOrientation& po=bsIt->getPositionOrientation();
				
				IO::JsonArrayPointer translation=new IO::JsonArray;
				positionOrientation->setProperty("translation",*translation);
				for(int i=0;i<3;++i)
					translation->addItem(po.getTranslation()[i]);
				
				IO::JsonArrayPointer rotation=new IO::JsonArray;
				positionOrientation->setProperty("rotation",*rotation);
				for(int i=0;i<4;++i)
					rotation->addItem(po.getRotation().getQuaternion()[i]);
				}
			}
		}
	}

void VRDeviceServer::handlePostRequest(Comm::HttpServer::PostRequest& postRequest)
	{
	/* Check that the POST request is for the correct URL and has exactly one command in the body: */
	const Comm::HttpServer::RequestParameterList& ps=postRequest.parameters;
	if(postRequest.requestUri=="/VRDeviceServer.cgi"&&ps.size()>=1&&ps.front().name=="command")
		{
		const std::string& command=ps.front().value;
		
		/* Compose the server's reply as a JSON-encoded object: */
		IO::JsonObjectPointer replyRoot=new IO::JsonObject;
		replyRoot->setProperty("command",command);
		
		/* Process the command: */
		if(command=="getServerStatus")
			{
			/* Compose the JSON object representing the current server state: */
			getServerStatus(*replyRoot);
			replyRoot->setProperty("status","Success");
			}
		else if(command=="getDeviceStates")
			{
			}
		else if(command=="hapticTick"&&ps.size()>1&&ps[1].name=="hapticFeatureIndex")
			{
			/* Extract the haptic feature index: */
			unsigned int hapticFeatureIndex(strtoul(ps[1].value.c_str(),0,10));
			
			/* Extract optional haptic tick duration, frequency, and amplitude: */
			unsigned int duration=100;
			unsigned int frequency=100;
			unsigned int amplitude=255;
			for(unsigned int i=2;i<ps.size();++i)
				{
				if(ps[i].name=="duration")
					duration=(unsigned int)(strtoul(ps[i].value.c_str(),0,10));
				else if(ps[i].name=="frequency")
					frequency=(unsigned int)(strtoul(ps[i].value.c_str(),0,10));
				else if(ps[i].name=="amplitude")
					amplitude=Math::clamp((unsigned int)(strtoul(ps[i].value.c_str(),0,10)),0U,255U);
				}
			
			/* Request a haptic tick: */
			if(hapticFeatureIndex<deviceManager->getNumHapticFeatures())
				{
				deviceManager->hapticTick(hapticFeatureIndex,duration,frequency,amplitude);
				replyRoot->setProperty("status","Success");
				}
			else
				replyRoot->setProperty("status","Invalid hapticFeatureIndex");
			}
		else if(command=="powerOff"&&ps.size()>1&&ps[1].name=="powerFeatureIndex")
			{
			/* Extract the power feature index: */
			unsigned int powerFeatureIndex(strtoul(ps[1].value.c_str(),0,10));
			
			/* Power off the device: */
			if(powerFeatureIndex<deviceManager->getNumPowerFeatures())
				{
				deviceManager->powerOff(powerFeatureIndex);
				replyRoot->setProperty("status","Success");
				}
			else
				replyRoot->setProperty("status","Invalid powerFeatureIndex");
			}
		else if(command=="uploadEnvironment"&&ps.size()>1&&ps[1].name=="environmentFilePath")
			{
			try
				{
				/* Open the environment definition configuration file: */
				Misc::ConfigurationFile environmentFile(ps[1].value.c_str());
				
				/* Read an environment definition from the file's root section: */
				Vrui::EnvironmentDefinition newEnvironmentDefinition;
				newEnvironmentDefinition.configure(environmentFile.getCurrentSection());
				
				/* Replace the previous environment definition and notify all clients that the environment definition has been updated: */
				environmentDefinition=newEnvironmentDefinition;
				environmentDefinitionUpdated(0);
				
				replyRoot->setProperty("status","Success");
				}
			catch(const std::runtime_error& err)
				{
				replyRoot->setProperty("status","Invalid environmentFilePath");
				}
			}
		else
			replyRoot->setProperty("status","Invalid command");
		
		/* Pass the reply to the POST request: */
		postRequest.setJsonResult(*replyRoot);
		}
	}

void VRDeviceServer::suspendTimeout(Threads::RunLoop::Timer::Event& event)
	{
	#ifdef VERBOSE
	printf("VRDeviceServer: Suspending devices due to inactivity\n");
	fflush(stdout);
	#endif
	
	/* Suspend the devices: */
	// Not actually doing that until we find out how it works...
	}

bool VRDeviceServer::writeStateUpdates(VRDeviceServer::ClientStateList::iterator csIt)
	{
	/* Bail out if the client is not streaming or does not understand incremental state updates: */
	ClientState& client=*csIt;
	if(client.state<STREAMING||client.protocolVersion<7U)
		return true;
	
	/* Send state updates to client: */
	try
		{
		/* Send tracker state updates: */
		for(std::vector<int>::iterator utIt=updatedTrackers.begin();utIt!=updatedTrackers.end();++utIt)
			{
			/* Send tracker update message: */
			client.pipe->write(MessageIdType(TRACKER_UPDATE));
			client.pipe->write(Misc::UInt16(*utIt));
			Misc::Marshaller<Vrui::VRDeviceState::TrackerState>::write(state.getTrackerState(*utIt),*client.pipe);
			client.pipe->write(state.getTrackerTimeStamp(*utIt));
			client.pipe->write(Misc::UInt8(state.getTrackerValid(*utIt)?1U:0U));
			}
		
		/* Send button updates: */
		for(std::vector<int>::iterator ubIt=updatedButtons.begin();ubIt!=updatedButtons.end();++ubIt)
			{
			/* Send button update message: */
			client.pipe->write(MessageIdType(BUTTON_UPDATE));
			client.pipe->write(Misc::UInt16(*ubIt));
			client.pipe->write(Misc::UInt8(state.getButtonState(*ubIt)?1U:0U));
			}
		
		/* Send valuator updates: */
		for(std::vector<int>::iterator uvIt=updatedValuators.begin();uvIt!=updatedValuators.end();++uvIt)
			{
			/* Send valuator update message: */
			client.pipe->write(MessageIdType(VALUATOR_UPDATE));
			client.pipe->write(Misc::UInt16(*uvIt));
			client.pipe->write(state.getValuatorState(*uvIt));
			}
		
		/* Finish the message set: */
		client.pipe->flush();
		}
	catch(const std::runtime_error& err)
		{
		/* Disconnect the client and signal an error: */
		disconnectClientOnError(csIt,err);
		return false;
		}
	
	return true;
	}

bool VRDeviceServer::writeServerState(VRDeviceServer::ClientStateList::iterator csIt)
	{
	/* Bail out if the client is not streaming or understands incremental state updates: */
	ClientState& client=*csIt;
	if(client.state<STREAMING||client.protocolVersion>=7U)
		return true;
	
	/* Send state to client: */
	try
		{
		/* Send packet reply message: */
		client.pipe->write(MessageIdType(PACKET_REPLY));
		
		/* Send server state: */
		state.write(*client.pipe,client.clientExpectsTimeStamps,client.clientExpectsValidFlags);
		client.pipe->flush();
		}
	catch(const std::runtime_error& err)
		{
		/* Disconnect the client and signal an error: */
		disconnectClientOnError(csIt,err);
		return false;
		}
	
	return true;
	}

bool VRDeviceServer::writeBatteryState(VRDeviceServer::ClientStateList::iterator csIt,unsigned int deviceIndex)
	{
	/* Bail out if the client is not active or can't handle battery states: */
	ClientState& client=*csIt;
	if(client.state<ACTIVE||client.protocolVersion<5U)
		return true;
	
	/* Send battery state to client: */
	try
		{
		/* Send battery state update message: */
		client.pipe->write(MessageIdType(BATTERYSTATE_UPDATE));
		
		/* Send virtual device index: */
		client.pipe->write(Misc::UInt16(deviceIndex));
		
		/* Send battery state: */
		batteryStates[deviceIndex].write(*client.pipe);
		client.pipe->flush();
		}
	catch(const std::runtime_error& err)
		{
		/* Disconnect the client and signal an error: */
		disconnectClientOnError(csIt,err);
		return false;
		}
	
	return true;
	}

bool VRDeviceServer::writeHmdConfiguration(VRDeviceServer::ClientStateList::iterator csIt,VRDeviceServer::HMDConfigurationVersions& hmdConfigurationVersions)
	{
	/* Bail out if the client is not active or can't handle HMD configurations: */
	ClientState& client=*csIt;
	if(client.state<ACTIVE||client.protocolVersion<4U)
		return true;
	
	try
		{
		/* Send HMD configuration to client: */
		Vrui::HMDConfiguration& hc=*hmdConfigurationVersions.hmdConfiguration;
		hc.write(hmdConfigurationVersions.eyePosVersion,hmdConfigurationVersions.eyeVersion,hmdConfigurationVersions.distortionMeshVersion,*client.pipe);
		if(client.protocolVersion>=10U&&hmdConfigurationVersions.eyeRotVersion!=hc.getEyeRotVersion())
			hc.writeEyeRotation(*client.pipe);
		client.pipe->flush();
		}
	catch(const std::runtime_error& err)
		{
		/* Disconnect the client and signal an error: */
		disconnectClientOnError(csIt,err);
		return false;
		}
	
	return true;
	}

VRDeviceServer::VRDeviceServer(Threads::RunLoop& sRunLoop,VRDeviceManager& sDeviceManager,const Misc::ConfigurationFile& configFile)
	:VRDeviceManager::VRStreamer(&sDeviceManager),
	 runLoop(sRunLoop),
	 deviceStateMemoryFd(-1),
	 httpServer(0),
	 numActiveClients(0),numStreamingClients(0),
	 suspendInterval(0,0),
	 haveUpdates(false),trackerUpdateFlags(0),buttonUpdateFlags(0),valuatorUpdateFlags(0),
	 managerTrackerStateVersion(0U),streamingTrackerStateVersion(0U),
	 haveVirtualDeviceUpdates(false),virtualDeviceUpdateds(0),
	 managerBatteryStateVersion(0U),streamingBatteryStateVersion(0U),batteryStateVersions(0),
	 managerHmdConfigurationVersion(0U),streamingHmdConfigurationVersion(0U),
	 numHmdConfigurations(hmdConfigurations.size()),hmdConfigurationVersions(0)
	{
	/* Read the definition of the physical environment from a separate configuration file: */
	std::string environmentDefinitionFileName=configFile.retrieveString("environmentDefinition",VRDEVICEDAEMON_CONFIG_ENVIRONMENTFILENAME);
	if(!Misc::hasExtension(environmentDefinitionFileName.c_str(),VRUI_INTERNAL_CONFIG_CONFIGFILESUFFIX))
		environmentDefinitionFileName.append(VRUI_INTERNAL_CONFIG_CONFIGFILESUFFIX);
	std::string fullEnvironmentDefinitionFileName;
	if(environmentDefinitionFileName[0]!='/')
		{
		/* Prefix the environment definition file name with the global configuration directory: */
		fullEnvironmentDefinitionFileName=VRUI_INTERNAL_CONFIG_SYSCONFIGDIR;
		fullEnvironmentDefinitionFileName.push_back('/');
		fullEnvironmentDefinitionFileName.append(environmentDefinitionFileName);
		}
	else
		fullEnvironmentDefinitionFileName=environmentDefinitionFileName;
	Misc::ConfigurationFile environmentDefinitionFile(fullEnvironmentDefinitionFileName.c_str());
	
	/* Merge the per-user environment configuration file if it exists: */
	#if VRUI_INTERNAL_CONFIG_HAVE_USERCONFIGFILE
	const char* home=getenv("HOME");
	if(home!=0&&home[0]!='\0')
		{
		/* Construct the name of the per-user environment configuration file: */
		std::string userEnvironmentDefinitionFileName=home;
		userEnvironmentDefinitionFileName.push_back('/');
		userEnvironmentDefinitionFileName.append(VRUI_INTERNAL_CONFIG_USERCONFIGDIR);
		userEnvironmentDefinitionFileName.push_back('/');
		userEnvironmentDefinitionFileName.append(Misc::getFileName(environmentDefinitionFileName.c_str()));
		
		/* Merge the per-user environment configuration file if it exists: */
		if(Misc::isFileReadable(userEnvironmentDefinitionFileName.c_str()))
			environmentDefinitionFile.merge(userEnvironmentDefinitionFileName.c_str());
		}
	#endif
	
	/* Configure the environment: */
	environmentDefinition.configure(environmentDefinitionFile.getCurrentSection());
	
	/* Check if the server should listen for client connection on a TCP socket: */
	if(configFile.hasTag("serverPort"))
		{
		/* Create a listening TCP socket and an I/O watcher for it: */
		tcpListeningSocket=new Comm::ListeningTCPSocket(configFile.retrieveValue<int>("serverPort"),5);
		tcpListeningSocketWatcher=runLoop.createIOWatcher(tcpListeningSocket->getFd(),Threads::RunLoop::IOWatcher::Read,true,*Threads::createFunctionCall(this,&VRDeviceServer::newClientConnection,*tcpListeningSocket));
		}
	
	/* Check if the server should listen for client connection on a UNIX domain socket: */
	if(configFile.hasTag("serverSocketName"))
		{
		/* Create a listening UNIX socket and an I/O watcher for it: */
		unixListeningSocket=new Comm::ListeningUNIXSocket(configFile.retrieveString("serverSocketName").c_str(),5,configFile.retrieveValue<bool>("serverSocketAbstract",true));
		unixListeningSocketWatcher=runLoop.createIOWatcher(unixListeningSocket->getFd(),Threads::RunLoop::IOWatcher::Read,true,*Threads::createFunctionCall(this,&VRDeviceServer::newClientConnection,*unixListeningSocket));
		
		/* Tell the device manager to use a shared memory block for device states: */
		deviceStateMemoryFd=deviceManager->useSharedMemory(configFile.retrieveString("deviceStateMemoryName","/VRDeviceManagerDeviceState.shmem").c_str());
		}
	
	/* Check if the server should listen for HTTP POST requests on a TCP socket: */
	if(configFile.hasTag("./httpPort"))
		{
		/* Create an HTTP server and register an HTTP POST request handler: */
		httpServer=new Comm::HttpServer(runLoop,configFile.retrieveValue<int>("httpPort"));
		httpServer->setPostRequestHandler(*Threads::createFunctionCall(this,&VRDeviceServer::handlePostRequest));
		}
	
	/* Create an inactive timer event listener to suspend VR devices after a certain period of inactivity: */
	suspendInterval=Threads::RunLoop::Interval(configFile.retrieveValue<int>("suspendTimeout",0),0);
	if(suspendInterval.tv_sec!=0)
		{
		suspendTimer=runLoop.createTimer(Threads::RunLoop::Time()+suspendInterval,*Threads::createFunctionCall(this,&VRDeviceServer::suspendTimeout));
		suspendTimer->disable();
		}
	
	/* Initialize the update tracking arrays: */
	{
	Threads::Mutex::Lock stateLock(stateMutex);
	
	trackerUpdateFlags=new bool[state.getNumTrackers()];
	for(int i=0;i<state.getNumTrackers();++i)
		trackerUpdateFlags[i]=false;
	
	buttonUpdateFlags=new bool[state.getNumButtons()];
	for(int i=0;i<state.getNumButtons();++i)
		buttonUpdateFlags[i]=false;
	
	valuatorUpdateFlags=new bool[state.getNumValuators()];
	for(int i=0;i<state.getNumValuators();++i)
		valuatorUpdateFlags[i]=false;
	
	{
	Threads::Mutex::Lock batteryStateLock(batteryStateMutex);
	
	/* Initialize the array of virtual device update trackers: */
	virtualDeviceUpdateds=new VirtualDeviceUpdated[deviceManager->getNumVirtualDevices()];
	for(int i=0;i<deviceManager->getNumVirtualDevices();++i)
		{
		VirtualDeviceUpdated& vdu=virtualDeviceUpdateds[i];
		const Vrui::VRDeviceDescriptor& vrd=deviceManager->getVirtualDevice(i);
		vdu.updated=false;
		vdu.name=&vrd.name;
		vdu.connected=deviceManager->isVirtualDeviceConnected(i);
		vdu.tracked=vrd.trackerIndex>=0&&state.getTrackerValid(vrd.trackerIndex);
		vdu.hasBattery=vrd.hasBattery;
		if(vdu.hasBattery)
			{
			vdu.batteryLevel=batteryStates[i].batteryLevel;
			vdu.charging=batteryStates[i].charging;
			}
		else
			{
			vdu.batteryLevel=0U;
			vdu.charging=false;
			}
		}
	}
	}
	
	/* Initialize the array of battery state version numbers: */
	batteryStateVersions=new BatteryStateVersions[deviceManager->getNumVirtualDevices()];
	
	/* Initialize the array of HMD configuration version numbers: */
	hmdConfigurationVersions=new HMDConfigurationVersions[numHmdConfigurations];
	for(unsigned int i=0;i<hmdConfigurations.size();++i)
		hmdConfigurationVersions[i].hmdConfiguration=hmdConfigurations[i];
	}

VRDeviceServer::~VRDeviceServer(void)
	{
	/* Stop VR devices if there are still active clients: */
	if(numActiveClients>0)
		deviceManager->stop();
	
	/* Forcefully disconnect all clients: */
	clientStates.clear();
	
	/* Delete an HTTP server: */
	delete httpServer;
	
	/* Clean up: */
	delete[] trackerUpdateFlags;
	delete[] buttonUpdateFlags;
	delete[] valuatorUpdateFlags;
	delete[] virtualDeviceUpdateds;
	delete[] batteryStateVersions;
	delete[] hmdConfigurationVersions;
	}

void VRDeviceServer::deviceConnectedUpdated(int deviceIndex,bool newConnected)
	{
	/* Remember the changed device's index and state and wake up the run loop: */
	haveVirtualDeviceUpdates=true;
	virtualDeviceUpdateds[deviceIndex].updated=true;
	virtualDeviceUpdateds[deviceIndex].connected=newConnected;
	runLoop.wakeUp();
	}

void VRDeviceServer::deviceTrackedUpdated(int deviceIndex,bool newTracked)
	{
	/* Remember the changed device's index and state and wake up the run loop: */
	haveVirtualDeviceUpdates=true;
	virtualDeviceUpdateds[deviceIndex].updated=true;
	virtualDeviceUpdateds[deviceIndex].tracked=newTracked;
	runLoop.wakeUp();
	}

void VRDeviceServer::trackerUpdated(int trackerIndex)
	{
	/* Remember the updated tracker's index and wake up the run loop: */
	haveUpdates=true;
	if(!trackerUpdateFlags[trackerIndex])
		{
		trackerUpdateFlags[trackerIndex]=true;
		updatedTrackers.push_back(trackerIndex);
		}
	runLoop.wakeUp();
	}

void VRDeviceServer::buttonUpdated(int buttonIndex)
	{
	/* Remember the updated button's index and wake up the run loop: */
	haveUpdates=true;
	if(!buttonUpdateFlags[buttonIndex])
		{
		buttonUpdateFlags[buttonIndex]=true;
		updatedButtons.push_back(buttonIndex);
		}
	runLoop.wakeUp();
	}

void VRDeviceServer::valuatorUpdated(int valuatorIndex)
	{
	/* Remember the updated valuator's index and wake up the run loop: */
	haveUpdates=true;
	if(!valuatorUpdateFlags[valuatorIndex])
		{
		valuatorUpdateFlags[valuatorIndex]=true;
		updatedValuators.push_back(valuatorIndex);
		}
	runLoop.wakeUp();
	}

void VRDeviceServer::updateCompleted(void)
	{
	/* Update the version number of the device manager's tracking state and wake up the run loop: */
	++managerTrackerStateVersion;
	runLoop.wakeUp();
	}

void VRDeviceServer::batteryStateUpdated(unsigned int deviceIndex)
	{
	/* Remember the changed device's index and new state, update the version number of the device manager's device battery state, and wake up the run loop: */
	haveVirtualDeviceUpdates=true;
	virtualDeviceUpdateds[deviceIndex].updated=true;
	virtualDeviceUpdateds[deviceIndex].batteryLevel=batteryStates[deviceIndex].batteryLevel;
	virtualDeviceUpdateds[deviceIndex].charging=batteryStates[deviceIndex].charging;
	++batteryStateVersions[deviceIndex].managerVersion;
	++managerBatteryStateVersion;
	runLoop.wakeUp();
	}

void VRDeviceServer::hmdConfigurationUpdated(const Vrui::HMDConfiguration* hmdConfiguration)
	{
	/* Update the version number of the device manager's HMD configuration state and wake up the run loop: */
	++managerHmdConfigurationVersion;
	runLoop.wakeUp();
	}

void VRDeviceServer::run(void)
	{
	#ifdef VERBOSE
	if(tcpListeningSocket!=0)
		printf("VRDeviceServer: Listening for incoming connections on TCP port %d\n",static_cast<Comm::ListeningTCPSocket*>(tcpListeningSocket.getPointer())->getPortId());
	if(unixListeningSocket!=0)
		printf("VRDeviceServer: Listening for incoming connections on UNIX domain socket %s\n",static_cast<Comm::ListeningUNIXSocket*>(unixListeningSocket.getPointer())->getAddress().c_str());
	if(httpServer!=0)
		printf("VRDeviceServer: Listening for HTTP POST requests on TCP port %d\n",httpServer->getPort());
	fflush(stdout);
	#endif
	
	/* Enable update notifications: */
	deviceManager->setStreamer(this);
	
	/* Run the main loop and dispatch events until stopped: */
	while(runLoop.dispatchNextEvents())
		{
		/* Check whether there are incremental or complete device state updates: */
		if(haveUpdates||streamingTrackerStateVersion!=managerTrackerStateVersion)
			{
			Threads::Mutex::Lock stateLock(stateMutex);
			
			/* Send incremental updates to all clients in streaming mode: */
			if(haveUpdates)
				{
				for(ClientStateList::iterator csIt=clientStates.begin();csIt!=clientStates.end();++csIt)
					if(!writeStateUpdates(csIt))
						--csIt;
				
				/* Reset the incremental update arrays: */
				haveUpdates=false;
				
				for(std::vector<int>::iterator utIt=updatedTrackers.begin();utIt!=updatedTrackers.end();++utIt)
					trackerUpdateFlags[*utIt]=false;
				updatedTrackers.clear();
				
				for(std::vector<int>::iterator ubIt=updatedButtons.begin();ubIt!=updatedButtons.end();++ubIt)
					buttonUpdateFlags[*ubIt]=false;
				updatedButtons.clear();
				
				for(std::vector<int>::iterator uvIt=updatedValuators.begin();uvIt!=updatedValuators.end();++uvIt)
					valuatorUpdateFlags[*uvIt]=false;
				updatedValuators.clear();
				}
			
			/* Send a full state update to all clients in streaming mode: */
			if(streamingTrackerStateVersion!=managerTrackerStateVersion)
				{
				for(ClientStateList::iterator csIt=clientStates.begin();csIt!=clientStates.end();++csIt)
					if(!writeServerState(csIt))
						--csIt;
				
				/* Mark streaming state as up-to-date: */
				streamingTrackerStateVersion=managerTrackerStateVersion;
				}
			}
		
		/* Check whether there is an HTTP server and any virtual devices changed their states: */
		if(httpServer!=0&&haveVirtualDeviceUpdates)
			{
			/* Send update events for all changed devices: */
			for(int i=0;i<deviceManager->getNumVirtualDevices();++i)
				{
				VirtualDeviceUpdated& vdu=virtualDeviceUpdateds[i];
				if(vdu.updated)
					{
					/* Create a JSON event data structure: */
					IO::JsonObjectPointer device=new IO::JsonObject;
					device->setProperty("name",*vdu.name);
					device->setProperty("isConnected",vdu.connected);
					device->setProperty("hasTracker",vdu.hasTracker);
					if(vdu.hasTracker)
						device->setProperty("isTracked",vdu.tracked);
					device->setProperty("hasBattery",vdu.hasBattery);
					if(vdu.hasBattery)
						{
						device->setProperty("batteryLevel",vdu.batteryLevel);
						device->setProperty("isCharging",vdu.charging);
						}
					
					/* Send the event data structure to clients: */
					httpServer->sendEvent("deviceStateChanged",*device);
					
					/* Mark the virtual device as up-to-date: */
					vdu.updated=false;
					}
				}
			
			/* Mark virtual device state as up-to-date: */
			haveVirtualDeviceUpdates=false;
			}
		
		/* Check if any device battery states need to be sent: */
		if(streamingBatteryStateVersion!=managerBatteryStateVersion)
			{
			Threads::Mutex::Lock batteryStateLock(batteryStateMutex);
			
			for(int i=0;i<deviceManager->getNumVirtualDevices();++i)
				{
				if(batteryStateVersions[i].streamingVersion!=batteryStateVersions[i].managerVersion)
					{
					#ifdef VERBOSE
					printf("VRDeviceServer: Sending updated battery state %d to clients\n",i);
					fflush(stdout);
					#endif
					
					/* Send the new battery state to all clients that are active and can handle it: */
					for(ClientStateList::iterator csIt=clientStates.begin();csIt!=clientStates.end();++csIt)
						if(!writeBatteryState(csIt,i))
							--csIt;
					
					/* Mark the battery state as up-to-date: */
					batteryStateVersions[i].streamingVersion=batteryStateVersions[i].managerVersion;
					}
				}
			
			/* Mark device battery state as up-to-date: */
			streamingBatteryStateVersion=managerBatteryStateVersion;
			}
		
		/* Check if any HMD configuration updates need to be sent: */
		if(streamingHmdConfigurationVersion!=managerHmdConfigurationVersion)
			{
			Threads::Mutex::Lock hmdConfigurationLock(hmdConfigurationMutex);
			
			for(unsigned int i=0;i<numHmdConfigurations;++i)
				{
				/* Check if this HMD configuration has been updated: */
				Vrui::HMDConfiguration& hc=*hmdConfigurationVersions[i].hmdConfiguration;
				if(hmdConfigurationVersions[i].displayLatency!=hc.getDisplayLatency()||hmdConfigurationVersions[i].eyePosVersion!=hc.getEyePosVersion()||hmdConfigurationVersions[i].eyeRotVersion!=hc.getEyeRotVersion()||hmdConfigurationVersions[i].eyeVersion!=hc.getEyeVersion()||hmdConfigurationVersions[i].distortionMeshVersion!=hc.getDistortionMeshVersion())
					{
					#ifdef VERBOSE
					printf("VRDeviceServer: Sending updated HMD configuration %u to clients\n",i);
					fflush(stdout);
					#endif
					
					/* Send the new HMD configuration to all clients that are active and can handle it: */
					for(ClientStateList::iterator csIt=clientStates.begin();csIt!=clientStates.end();++csIt)
						if(!writeHmdConfiguration(csIt,hmdConfigurationVersions[i]))
							--csIt;
					
					/* Mark the HMD configuration as up-to-date: */
					hmdConfigurationVersions[i].displayLatency=hc.getDisplayLatency();
					hmdConfigurationVersions[i].eyePosVersion=hc.getEyePosVersion();
					hmdConfigurationVersions[i].eyeRotVersion=hc.getEyeRotVersion();
					hmdConfigurationVersions[i].eyeVersion=hc.getEyeVersion();
					hmdConfigurationVersions[i].distortionMeshVersion=hc.getDistortionMeshVersion();
					}
				}
			
			/* Mark HMD configuration state as up-to-date: */
			streamingHmdConfigurationVersion=managerHmdConfigurationVersion;
			}
		}
	
	/* Disable update notifications: */
	deviceManager->setStreamer(0);
	}
