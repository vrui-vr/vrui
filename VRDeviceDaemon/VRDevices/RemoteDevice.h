/***********************************************************************
RemoteDevice - Class to daisy-chain device servers on remote machines.
Copyright (c) 2002-2023 Oliver Kreylos

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

#ifndef REMOTEDEVICE_INCLUDED
#define REMOTEDEVICE_INCLUDED

#include <Comm/Pipe.h>
#include <Vrui/Internal/VRDeviceState.h>
#include <Vrui/Internal/VRDeviceProtocol.h>

#include <VRDeviceDaemon/VRDevice.h>

class RemoteDevice:public VRDevice,public Vrui::VRDeviceProtocol
	{
	/* Elements: */
	private:
	Comm::PipePtr pipe; // Pipe connected to remote device server
	unsigned int serverProtocolVersionNumber; // Version number of server protocol
	bool serverHasTimeStamps; // Flag whether the connected device server sends tracker state time stamps
	bool serverHasValidFlags; // Flag whether the connected device server sends tracker valid flags
	Vrui::VRDeviceState state; // Shadow of connected server's current state
	
	/* Protected methods: */
	virtual void deviceThreadMethod(void);
	
	/* Constructors and destructors: */
	public:
	RemoteDevice(VRDevice::Factory* sFactory,VRDeviceManager* sDeviceManager,Misc::ConfigurationFile& configFile);
	~RemoteDevice(void);
	
	/* Methods: */
	virtual void start(void);
	virtual void stop(void);
	};

#endif
