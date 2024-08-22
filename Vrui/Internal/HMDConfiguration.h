/***********************************************************************
HMDConfiguration - Class to represent the internal configuration of a
head-mounted display.
Copyright (c) 2016-2023 Oliver Kreylos

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

#ifndef VRUI_INTERNAL_HMDCONFIGURATION_INCLUDED
#define VRUI_INTERNAL_HMDCONFIGURATION_INCLUDED

#include <Misc/SizedTypes.h>
#include <Misc/Size.h>
#include <Misc/Rect.h>
#include <IO/File.h>
#include <Geometry/Point.h>
#include <Geometry/Rotation.h>
#include <Vrui/Vrui.h>
#include <Vrui/Internal/VRDeviceProtocol.h>

namespace Vrui {

class HMDConfiguration
	{
	/* Embedded classes: */
	public:
	typedef Misc::UInt32 WUInt; // Wire type for unsigned integers
	typedef Misc::Float32 WScalar; // Wire scalar type for HMD device coordinates
	typedef Geometry::Point<Scalar,2> Point2; // Type for 2D points in HMD screen space
	
	struct DistortionMeshVertex // Structure for distortion mesh vertices
		{
		/* Elements: */
		public:
		Point2 red; // Distortion-corrected vertex position for red color component
		Point2 green; // Distortion-corrected vertex position for green color component
		Point2 blue; // Distortion-corrected vertex position for blue color component
		};
	
	struct EyeConfiguration // Structure defining the configuration of one eye
		{
		/* Elements: */
		public:
		IRect viewport; // Eye's viewport (x, y, width, height) in final display window
		Scalar fov[4]; // Left, right, bottom, and top field-of-view boundaries in tangent space
		DistortionMeshVertex* distortionMesh; // 2D array of distortion mesh vertices
		};
	
	/* Elements: */
	private:
	unsigned int trackerIndex; // Tracker index associated with the HMD
	unsigned int faceDetectorButtonIndex; // Button index associated with the HMD's face detection sensor
	int displayLatency; // Latency from vblank to the time a new image is actually displayed in nanoseconds
	Scalar ipd; // Current inter-pupillary distance in HMD device coordinate units
	Point eyePos[2]; // Positions of left and right eyes in HMD device coordinates
	unsigned int eyePosVersion; // Version number of the eye positions
	Rotation eyeRot[2]; // Rotations of left and right eyes relative to HMD coordinates
	unsigned int eyeRotVersion; // Version number of the eye rotations
	ISize renderTargetSize; // Recommended width and height of per-eye pre-distortion render target
	ISize distortionMeshSize; // Number of vertices in per-eye lens distortion correction meshes
	EyeConfiguration eyes[2]; // Configurations of the left and right eyes
	unsigned int eyeVersion; // Version number of the eye configurations
	unsigned int distortionMeshVersion; // Version number of the distortion meshes
	
	/* Constructors and destructors: */
	public:
	HMDConfiguration(void); // Creates uninitialized HMD configuration structure
	private:
	HMDConfiguration(const HMDConfiguration& source); // Prohibit copy constructor
	HMDConfiguration& operator=(const HMDConfiguration& source); // Prohibit copy constructor
	public:
	~HMDConfiguration(void);
	
	/* Methods: */
	unsigned int getTrackerIndex(void) const // Returns the index of the tracker tracking this HMD
		{
		return trackerIndex;
		}
	unsigned int getFaceDetectorButtonIndex(void) const // Returns the button index of this HMD's face detector
		{
		return faceDetectorButtonIndex;
		}
	int getDisplayLatency(void) const // Returns the latency from vblank to the time a new image is actually displayed in nanoseconds
		{
		return displayLatency;
		}
	const Point& getEyePosition(int eyeIndex) const // Returns the position of the left or right eye
		{
		return eyePos[eyeIndex];
		}
	Scalar getIpd(void) const // Returns the inter-pupillary distance
		{
		return Geometry::dist(eyePos[0],eyePos[1]);
		}
	const Rotation& getEyeRotation(int eyeIndex) const // Returns the rotation of the left or right eye
		{
		return eyeRot[eyeIndex];
		}
	const ISize& getRenderTargetSize(void) const // Returns the recommended per-eye render target size
		{
		return renderTargetSize;
		}
	const ISize& getDistortionMeshSize(void) const // Returns the per-eye distortion mesh size
		{
		return distortionMeshSize;
		}
	const IRect& getViewport(int eyeIndex) const // Returns the final display window viewport for the given eye
		{
		return eyes[eyeIndex].viewport;
		}
	const Scalar* getFov(int eyeIndex) const // Returns the tangent-space field-of-view boundaries for the given eye
		{
		return eyes[eyeIndex].fov;
		}
	const DistortionMeshVertex* getDistortionMesh(int eyeIndex) const; // Returns a pointer to the given eye's distortion mesh for reading
	void setTrackerIndex(unsigned int newTrackerIndex); // Sets the index of the tracker tracking this HMD
	void setFaceDetectorButtonIndex(unsigned int newFaceDetectorButtonIndex); // Sets the index of the button representing this HMD's face detector
	void setDisplayLatency(int newDisplayLatency); // Sets the latency from vblank to the time a new image is actually displayed in nanoseconds
	void setEyePos(const Point& leftPos,const Point& rightPos); // Sets left and right eye positions directly
	void setIpd(Scalar newIPD); // Sets left and right eye positions based on previous positions and new inter-pupillary distance
	void setEyeRot(const Rotation& leftRot,const Rotation& rightRot); // Sets left and right eye rotations
	void setRenderTargetSize(const ISize& newRenderTargetSize); // Sets a new recommended render target size
	void setDistortionMeshSize(const ISize& newDistortionMeshSize); // Sets a new distortion mesh size; resets mesh to undefined if size changed
	void setViewport(int eye,const IRect& newViewport); // Sets the given eye's final display window viewport
	void setFov(int eye,Scalar left,Scalar right,Scalar bottom,Scalar top); // Sets the given eye's tangent space field-of-view boundaries
	DistortionMeshVertex* getDistortionMesh(int eye); // Returns a pointer to the given eye's distortion mesh for updates
	void updateDistortionMeshes(void); // Marks the distortion mesh as updated after access is complete
	void write(unsigned int sinkEyePosVersion,unsigned int sinkEyeVersion,unsigned int sinkDistortionMeshVersion,IO::File& sink) const; // Writes outdated components of an HMD configuration to the given sink
	void writeEyeRotation(IO::File& sink) const; // Writes current eye rotations to the given sink
	void read(VRDeviceProtocol::MessageIdType messageId,unsigned int newTrackerIndex,IO::File& source); // Reads an HMD configuration from the given source after receiving the given update message ID
	void readEyeRotation(IO::File& source); // Reads eye rotations from the given source
	unsigned int getEyePosVersion(void) const
		{
		return eyePosVersion;
		}
	unsigned int getEyeRotVersion(void) const
		{
		return eyeRotVersion;
		}
	unsigned int getEyeVersion(void) const
		{
		return eyeVersion;
		}
	unsigned int getDistortionMeshVersion(void) const
		{
		return distortionMeshVersion;
		}
	};

}

#endif
