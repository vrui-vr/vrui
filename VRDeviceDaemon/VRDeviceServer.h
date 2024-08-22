/***********************************************************************
VRDeviceServer - Class encapsulating the VR device protocol's server
side.
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

#include <string>
#include <vector>
#include <Misc/SimpleObjectSet.h>
#include <Threads/EventDispatcher.h>
#include <Comm/ListeningSocket.h>
#include <Vrui/EnvironmentDefinition.h>
#include <Vrui/Internal/VRDeviceProtocol.h>

#include <VRDeviceDaemon/VRDeviceManager.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFile;
}
namespace Vrui {
class BatteryState;
class HMDConfiguration;
}

class VRDeviceServer:public VRDeviceManager::VRStreamer,public Vrui::VRDeviceProtocol
	{
	/* Embedded classes: */
	private:
	struct ClientState // Class containing state of connected client
		{
		/* Elements: */
		public:
		VRDeviceServer* server; // Pointer to server object handling this client, to simplify event handling
		Comm::PipePtr pipe; // Pipe connected to the client
		#ifdef VERBOSE
		std::string clientName; // Name of the client, to keep track of connections in verbose mode
		#endif
		Threads::EventDispatcher::ListenerKey listenerKey; // Key with which this client is listening for I/O events
		int state; // Client's current position in the VRDeviceServer protocol state machine
		unsigned int protocolVersion; // Version of the VR device daemon protocol to use with this client
		bool clientExpectsTimeStamps; // Flag whether the connected client expects to receive time stamp data
		bool clientExpectsValidFlags; // Flag whether the connected client expects to receive tracker valid flags
		bool active; // Flag whether the client is currently active
		bool streaming; // Flag whether client is currently in streaming mode
		
		/* Constructors and destructors: */
		ClientState(VRDeviceServer* sServer,Comm::PipePtr sPipe); // Connects to a VR client over the given pipe
		};
	
	typedef Misc::SimpleObjectSet<ClientState> ClientStateList; // Data type for sets of states of connected clients
	
	struct BatteryStateVersions // Structure to hold version numbers for a device battery state
		{
		/* Elements: */
		public:
		unsigned int managerVersion; // Battery state's version in device manager
		unsigned int streamingVersion; // Battery state version most recently sent to streaming clients
		
		/* Constructors and destructors: */
		BatteryStateVersions(void)
			:managerVersion(0U),streamingVersion(0U)
			{
			}
		};
	
	struct HMDConfigurationVersions // Structure to hold version numbers for an HMD configuration
		{
		/* Elements: */
		public:
		Vrui::HMDConfiguration* hmdConfiguration; // Pointer to the HMD configuration
		int displayLatency; // HMD display latency most recently sent to streaming clients
		unsigned int eyePosVersion,eyeRotVersion,eyeVersion,distortionMeshVersion; // Version numbers for the four HMD configuration components most recently sent to streaming clients
		
		/* Constructors and destructors: */
		HMDConfigurationVersions(void)
			:hmdConfiguration(0),
			 displayLatency(0),
			 eyePosVersion(0U),eyeRotVersion(0U),eyeVersion(0U),distortionMeshVersion(0U)
			{
			}
		};
	
	/* Elements: */
	private:
	Threads::EventDispatcher& dispatcher; // Reference to shared event dispatcher to handle communication with multiple clients in parallel
	Vrui::EnvironmentDefinition environmentDefinition; // Definition of physical environment that can be queried by clients
	Threads::EventDispatcher::ListenerKey	environmentDefinitionUpdatedSignalKey; // Key for signal when a client updates the environment definition
	
	Comm::ListeningSocketPtr tcpListeningSocket; // Optional TCP socket on which the server accepts incoming client connections
	Threads::EventDispatcher::ListenerKey tcpListeningSocketKey; // Key for IO events on the listening TCP socket
	Comm::ListeningSocketPtr unixListeningSocket; // Optional UNIX domain socket on which the server accepts incoming client connections
	Threads::EventDispatcher::ListenerKey unixListeningSocketKey; // Key for IO events on the listening UNIX domain socket
	int deviceStateMemoryFd; // File descriptor to access the device manager's shared-memory device state
	
	ClientStateList clientStates; // List of currently connected clients
	unsigned int numActiveClients; // Number of clients that are currently active
	unsigned int numStreamingClients; // Number of clients that are currently streaming
	Threads::EventDispatcher::Time suspendTime; // Inactivity interval after which VR devices will be suspended
	Threads::EventDispatcher::ListenerKey suspendTimerKey; // Key for timer to suspend VR devices a certain time after the last client deactivated
	
	bool haveUpdates; // Flag if any device state components have been updated since last status update was sent
	bool* trackerUpdateFlags; // Array of flags indicating which trackers have been updated since the last state update was sent
	bool* buttonUpdateFlags; // Array of flags indicating which trackers have been updated since the last state update was sent
	bool* valuatorUpdateFlags; // Array of flags indicating which trackers have been updated since the last state update was sent
	std::vector<int> updatedTrackers; // List of trackers that have been updated since last status update was sent
	std::vector<int> updatedButtons; // List of buttons that have been updated since last status update was sent
	std::vector<int> updatedValuators; // List of valuators that have been updated since last status update was sent
	unsigned int managerTrackerStateVersion; // Version number of tracker states in device manager
	unsigned int streamingTrackerStateVersion; // Version number of tracker states most recently sent to streaming clients
	unsigned int managerBatteryStateVersion; // Version number of device battery states in device manager
	unsigned int streamingBatteryStateVersion; // Version number of device battery states most recently sent to streaming clients
	BatteryStateVersions* batteryStateVersions; // Array of battery state version numbers
	unsigned int managerHmdConfigurationVersion; // Version number of HMD configurations in device manager
	unsigned int streamingHmdConfigurationVersion; // Version number of HMD configurations most recently sent to streaming clients
	unsigned int numHmdConfigurations; // Number of HMD configurations in the device manager
	HMDConfigurationVersions* hmdConfigurationVersions; // Array of HMD configuration version numbers
	
	/* Private methods: */
	void connectNewClient(Comm::ListeningSocket& listeningSocket); // Connects a new client over the given listening socket
	static void newTcpConnectionCallback(Threads::EventDispatcher::IOEvent& event); // Callback called when an incoming connection is waiting at the TCP listening socket
	static void newUnixConnectionCallback(Threads::EventDispatcher::IOEvent& event); // Callback called when an incoming connection is waiting at the UNIX domain listening socket
	static void suspendTimerCallback(Threads::EventDispatcher::TimerEvent& event); // Callback called after a period of inactivity
	static void environmentDefinitionUpdatedCallback(Threads::EventDispatcher::SignalEvent& event); // Callback called when a client updates the environment definition
	void goInactive(void); // Sets the server to inactive mode when the last client leaves active state
	void goActive(void); // Sets the server to active mode when the first client enters active state
	void disconnectClient(ClientState* client,bool removeListener,bool removeFromList); // Disconnects the given client due to a communication error; removes listener and/or dead client from list if respective flags are true
	static void clientMessageCallback(Threads::EventDispatcher::IOEvent& event); // Callback called when a message from a client arrives
	void disconnectClientOnError(ClientStateList::iterator csIt,const std::runtime_error& err); // Forcefully disconnects a client after a communication error
	bool writeStateUpdates(ClientStateList::iterator csIt); // Writes changes in the device manager's device state to the given client; returns false on error
	bool writeServerState(ClientStateList::iterator csIt); // Writes the device manager's current (locked) state to the given client; returns false on error
	bool writeBatteryState(ClientStateList::iterator csIt,unsigned int deviceIndex); // Writes the device manager's given battery state to the given client; returns false on error
	bool writeHmdConfiguration(ClientStateList::iterator csIt,HMDConfigurationVersions& hmdConfigurationVersions); // Writes the given HMD configuration to the given client; returns false on error
	
	/* Constructors and destructors: */
	public:
	VRDeviceServer(Threads::EventDispatcher& sDispatcher,VRDeviceManager* sDeviceManager,const Misc::ConfigurationFile& configFile); // Creates server associated with device manager
	virtual ~VRDeviceServer(void);
	
	/* Methods from VRDeviceManager::VRStreamer: */
	virtual void trackerUpdated(int trackerIndex);
	virtual void buttonUpdated(int buttonIndex);
	virtual void valuatorUpdated(int valuatorIndex);
	virtual void updateCompleted(void);
	virtual void batteryStateUpdated(unsigned int deviceIndex);
	virtual void hmdConfigurationUpdated(const Vrui::HMDConfiguration* hmdConfiguration);
	
	/* New methods: */
	void run(void); // Runs the server state machine
	void stop(void) // Stops the server state machine; can be called asynchronously
		{
		/* Stop the dispatcher's event handling: */
		dispatcher.stop();
		}
	};
