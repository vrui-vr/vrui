/***********************************************************************
VRCompositorProtocol - Classes to exchange data between a VR client
application and a VR compositor server through shared memory.
Copyright (c) 2022-2023 Oliver Kreylos

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

#ifndef VRUI_INTERNAL_VRCOMPOSITORPROTOCOL_INCLUDED
#define VRUI_INTERNAL_VRCOMPOSITORPROTOCOL_INCLUDED

#include <Misc/Size.h>
#include <Misc/Rect.h>
#include <Realtime/Time.h>
#include <Threads/DoubleBuffer.h>
#include <Threads/TripleBuffer.h>
#include <Geometry/Point.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Vrui/Types.h>

namespace Vrui {

class VRCompositorProtocol
	{
	/* Embedded classes: */
	public:
	struct HMDConfiguration // Structure holding the application rendering configuration of the HMD controlled by the VR compositor
		{
		/* Elements: */
		public:
		ISize frameSize; // Width and height of pre-distortion frames
		IRect eyeRects[2]; // Left and right eyes' viewports inside a pre-distortion frame
		Point eyePositions[2]; // Positions of left and right eyes in HMD device coordinates
		Rotation eyeRotations[2]; // Rotations of left and right eyes relative to HMD coordinates
		Scalar eyeFovs[2][4]; // Field-of-view boundaries of left and right eyes in tangent space
		TimeVector exposeOffset; // Time interval from vblank event to when the submitted image is actually shown to the user
		};
	
	struct VblankTimer // Structure representing the VR compositor's current estimate of the HMD's Vblank period and next Vblank event
		{
		/* Elements: */
		public:
		unsigned long frameIndex; // Index of the current frame
		TimePoint nextVblankTime; // Time at which the next vblank event is predicted to happen
		TimeVector vblankPeriod; // HMD's vblank period
		};
	
	struct RenderResult // Structure representing the result of rendering a frame in a VR client application
		{
		/* Elements: */
		public:
		unsigned int imageIndex; // Index of the input image containing the rendering
		TimePoint renderTime; // Time at which rendering was started
		TrackerState headDeviceTransform; // Head device transformation for which the image was rendered, for reprojection
		};
	
	struct SharedMemorySegment // Layout of the shared memory segment used for client/server communication
		{
		/* Elements: */
		public:
		unsigned int protocolVersion; // The version number of the VR compositor protocol used in this shared memory segment
		Threads::DoubleBuffer<HMDConfiguration> hmdConfiguration; // The current configuration of the HMD
		Threads::DoubleBuffer<VblankTimer> vblankTimer; // The current vblank timing state
		Threads::TripleBuffer<RenderResult> renderResults; // Triple buffer of rendering results
		};
	
	/* Elements: */
	static const unsigned int protocolVersion; // Current version of the VR compositor protocol
	};

}

#endif
