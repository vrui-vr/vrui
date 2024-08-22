/***********************************************************************
VRDeviceProtocol - Class defining the client-server protocol between a
Vrui application and a VR device server.
Copyright (c) 2002-2023 Oliver Kreylos

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

#ifndef VRUI_INTERNAL_VRDEVICEPROTOCOL_INCLUDED
#define VRUI_INTERNAL_VRDEVICEPROTOCOL_INCLUDED

#include <Misc/SizedTypes.h>

namespace Vrui {

class VRDeviceProtocol
	{
	/* Embedded classes: */
	public:
	typedef Misc::UInt16 MessageIdType; // Network type for protocol messages
	
	enum MessageId // Enumerated type for protocol messages
		{
		CONNECT_REQUEST, // Request to connect to server
		CONNECT_REPLY, // Positive connect reply with server layout
		DISCONNECT_REQUEST, // Polite request to disconnect from server
		ACTIVATE_REQUEST, // Request to activate server (prepare for sending packets)
		DEACTIVATE_REQUEST, // Request to deactivate server (no more packet requests)
		PACKET_REQUEST, // Requests a single packet with current device state
		PACKET_REPLY, // Sends a device state packet
		STARTSTREAM_REQUEST, // Requests entering stream mode (server sends packets automatically)
		STOPSTREAM_REQUEST, // Requests leaving stream mode
		STOPSTREAM_REPLY, // Server's reply after last stream packet has been sent
		BATTERYSTATE_UPDATE, // Battery status of a virtual input device has changed
		HMDCONFIG_UPDATE=16, // Server has an updated HMD configuration; lowest three bits of message ID define which components are updated
		POWEROFF_REQUEST=24, // Requests to power off a virtual input device
		HAPTICTICK_REQUEST, // Requests a haptic tick on a virtual input device
		TRACKER_UPDATE, // Sends new state for a single tracker
		BUTTON_UPDATE, // Sends new state for a single button
		VALUATOR_UPDATE, // Sends new state for a single valuator
		HMDCONFIG_EYEROTATION_UPDATE, // Sends new rotations for the left and right eyes
		BASESTATIONS_REQUEST, // Requests the list of states of base stations registered with the server
		BASESTATIONS_REPLY, // List of base station states registered with the server
		ENVIRONMENTDEFINITION_REQUEST, // Requests the definition of the environment in physical space
		ENVIRONMENTDEFINITION_REPLY, // Sends the definition of the environment in physical space
		ENVIRONMENTDEFINITION_UPDATE_REQUEST, // Requests that the server updates its physical environment definition, sent by RoomSetup utility
		ENVIRONMENTDEFINITION_UPDATE_NOTIFICATION // Notifies connected clients that the server's environment definition was changed
		};
	
	/* Elements: */
	protected:
	static const Misc::UInt32 protocolVersionNumber; // Version number of client/server protocol
	};

}

#endif
