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

#include <Vrui/Internal/HMDConfiguration.h>

namespace Vrui {

/*********************************
Methods of class HMDConfiguration:
*********************************/

HMDConfiguration::HMDConfiguration(void)
	:trackerIndex(0),faceDetectorButtonIndex(0),
	 displayLatency(0),
	 ipd(0),
	 eyePosVersion(0U),eyeRotVersion(0U),
	 renderTargetSize(0,0),distortionMeshSize(0,0),
	 eyeVersion(0U),distortionMeshVersion(0U)
	{
	/* Initialize eye position and rotation: */
	for(int eye=0;eye<2;++eye)
		{
		eyePos[eye]=Point::origin;
		eyeRot[eye]=Rotation::identity;
		}
	
	/* Initialize the eye configurations: */
	for(int eye=0;eye<2;++eye)
		{
		eyes[eye].viewport=IRect(ISize(0,0));
		for(int i=0;i<4;++i)
			eyes[eye].fov[i]=Scalar(0);
		eyes[eye].distortionMesh=0;
		}
	}

HMDConfiguration::~HMDConfiguration(void)
	{
	/* Delete distortion meshes: */
	for(int eye=0;eye<2;++eye)
		delete[] eyes[eye].distortionMesh;
	}

const HMDConfiguration::DistortionMeshVertex* HMDConfiguration::getDistortionMesh(int eye) const
	{
	/* Return the given eye's distortion mesh: */
	return eyes[eye].distortionMesh;
	}

void HMDConfiguration::setTrackerIndex(unsigned int newTrackerIndex)
	{
	trackerIndex=newTrackerIndex;
	}

void HMDConfiguration::setFaceDetectorButtonIndex(unsigned int newFaceDetectorButtonIndex)
	{
	faceDetectorButtonIndex=newFaceDetectorButtonIndex;
	}

void HMDConfiguration::setDisplayLatency(int newDisplayLatency)
	{
	displayLatency=newDisplayLatency;
	}

void HMDConfiguration::setEyePos(const Point& leftPos,const Point& rightPos)
	{
	/* Update both eye positions directly: */
	eyePos[0]=leftPos;
	eyePos[1]=rightPos;
	
	/* Calculate the inter-pupillary distance: */
	ipd=Scalar(Geometry::dist(eyePos[0],eyePos[1]));
	
	/* Invalidate the cached eye position: */
	if(++eyePosVersion==0U)
		++eyePosVersion;
	}

void HMDConfiguration::setIpd(Scalar newIpd)
	{
	/* Check if the new inter-pupillary distance is different from the current one: */
	if(ipd!=newIpd)
		{
		/* Update both eye positions symmetrically based on old distance vector and new distance: */
		Point monoPos=Geometry::mid(eyePos[0],eyePos[1]);
		Vector dist=eyePos[1]-eyePos[0];
		dist*=Math::div2(newIpd/ipd);
		eyePos[0]=monoPos-dist;
		eyePos[1]=monoPos+dist;
		
		ipd=newIpd;
		
		/* Invalidate the cached eye position: */
		if(++eyePosVersion==0U)
			++eyePosVersion;
		}
	}

void HMDConfiguration::setEyeRot(const Rotation& leftRot,const Rotation& rightRot)
	{
	/* Update both eye rotations directly: */
	eyeRot[0]=leftRot;
	eyeRot[1]=rightRot;
	
	/* Invalidate the cached eye rotation: */
	if(++eyeRotVersion==0U)
		++eyeRotVersion;
	}

void HMDConfiguration::setRenderTargetSize(const ISize& newRenderTargetSize)
	{
	/* Check if the render target size changed: */
	if(renderTargetSize!=newRenderTargetSize)
		{
		/* Update the render target size: */
		renderTargetSize=newRenderTargetSize;
		
		/* Invalidate the cached distortion mesh: */
		if(++distortionMeshVersion==0U)
			++distortionMeshVersion;
		}
	}

void HMDConfiguration::setDistortionMeshSize(const ISize& newDistortionMeshSize)
	{
	/* Check if the mesh size changed: */
	if(distortionMeshSize!=newDistortionMeshSize)
		{
		/* Re-allocate distortion meshes: */
		distortionMeshSize=newDistortionMeshSize;
		for(int eye=0;eye<2;++eye)
			{
			delete[] eyes[eye].distortionMesh;
			eyes[eye].distortionMesh=new DistortionMeshVertex[distortionMeshSize.volume()];
			
			/* Initialize the new distortion mesh: */
			DistortionMeshVertex* dmPtr=eyes[eye].distortionMesh;
			DistortionMeshVertex* dmEnd=dmPtr+distortionMeshSize.volume();
			for(;dmPtr!=dmEnd;++dmPtr)
				dmPtr->blue=dmPtr->green=dmPtr->red=Point2::origin;
			}
		
		/* Invalidate the cached distortion mesh: */
		if(++distortionMeshVersion==0U)
			++distortionMeshVersion;
		}
	}

void HMDConfiguration::setViewport(int eye,const IRect& newViewport)
	{
	/* Check if the viewport changed: */
	if(eyes[eye].viewport!=newViewport)
		{
		/* Update the given eye's viewport: */
		eyes[eye].viewport=newViewport;
		
		/* Invalidate the cached distortion mesh: */
		if(++distortionMeshVersion==0U)
			++distortionMeshVersion;
		}
	}

void HMDConfiguration::setFov(int eye,Scalar left,Scalar right,Scalar bottom,Scalar top)
	{
	/* Check if the FoV changed: */
	if(eyes[eye].fov[0]!=left||eyes[eye].fov[1]!=right||eyes[eye].fov[2]!=bottom||eyes[eye].fov[3]!=top)
		{
		/* Update the given eye's FoV boundaries: */
		eyes[eye].fov[0]=left;
		eyes[eye].fov[1]=right;
		eyes[eye].fov[2]=bottom;
		eyes[eye].fov[3]=top;
		
		/* Invalidate the cached per-eye state: */
		if(++eyeVersion==0U)
			++eyeVersion;
		}
	}

HMDConfiguration::DistortionMeshVertex* HMDConfiguration::getDistortionMesh(int eye)
	{
	/* Return the given eye's distortion mesh: */
	return eyes[eye].distortionMesh;
	}

void HMDConfiguration::updateDistortionMeshes(void)
	{
	if(++distortionMeshVersion==0U)
		++distortionMeshVersion;
	}

void HMDConfiguration::write(unsigned int sinkEyePosVersion,unsigned int sinkEyeVersion,unsigned int sinkDistortionMeshVersion,IO::File& sink) const
	{
	/* Write the appropriate update message ID: */
	VRDeviceProtocol::MessageIdType messageId=VRDeviceProtocol::HMDCONFIG_UPDATE;
	if(sinkEyePosVersion!=eyePosVersion)
		messageId=messageId|VRDeviceProtocol::MessageIdType(0x1U);
	if(sinkEyeVersion!=eyeVersion)
		messageId=messageId|VRDeviceProtocol::MessageIdType(0x2U);
	if(sinkDistortionMeshVersion!=distortionMeshVersion)
		messageId=messageId|VRDeviceProtocol::MessageIdType(0x4U);
	sink.write(messageId);
	
	/* Write the tracker index to identify this HMD: */
	sink.write(Misc::UInt16(trackerIndex));
	
	/* Write the face detector button index: */
	sink.write(Misc::UInt16(faceDetectorButtonIndex));
	
	/* Write the display latency: */
	sink.write(Misc::SInt32(displayLatency));
	
	/* Write out-of-date configuration components: */
	if(sinkEyePosVersion!=eyePosVersion)
		{
		/* Write both eye positions: */
		for(int eye=0;eye<2;++eye)
			sink.write<WScalar>(eyePos[eye].getComponents(),3);
		}
	if(sinkEyeVersion!=eyeVersion)
		{
		/* Write both eyes' FoV boundaries: */
		for(int eye=0;eye<2;++eye)
			sink.write<WScalar>(eyes[eye].fov,4);
		}
	if(sinkDistortionMeshVersion!=distortionMeshVersion)
		{
		/* Write recommended render target size: */
		sink.write<WUInt,unsigned int>(renderTargetSize.getComponents(),2);
		
		/* Write distortion mesh size: */
		sink.write<WUInt,unsigned int>(distortionMeshSize.getComponents(),2);
		
		/* Write per-eye state: */
		for(int eye=0;eye<2;++eye)
			{
			/* Write eye's viewport: */
			sink.write<WUInt,int>(eyes[eye].viewport.offset.getComponents(),2);
			sink.write<WUInt,unsigned int>(eyes[eye].viewport.size.getComponents(),2);
			
			/* Write eye's distortion mesh: */
			const DistortionMeshVertex* dmPtr=eyes[eye].distortionMesh;
			const DistortionMeshVertex* dmEnd=dmPtr+distortionMeshSize.volume();
			for(;dmPtr!=dmEnd;++dmPtr)
				{
				sink.write<WScalar>(dmPtr->red.getComponents(),2);
				sink.write<WScalar>(dmPtr->green.getComponents(),2);
				sink.write<WScalar>(dmPtr->blue.getComponents(),2);
				}
			}
		}
	}

void HMDConfiguration::writeEyeRotation(IO::File& sink) const
	{
	/* Write the update message ID: */
	sink.write(VRDeviceProtocol::MessageIdType(VRDeviceProtocol::HMDCONFIG_EYEROTATION_UPDATE));
	
	/* Write the tracker index to identify this HMD: */
	sink.write<Misc::UInt16>(trackerIndex);
	
	/* Write both eye rotations: */
	for(int eye=0;eye<2;++eye)
		sink.write<WScalar>(eyeRot[eye].getQuaternion(),4);
	}

void HMDConfiguration::read(VRDeviceProtocol::MessageIdType messageId,unsigned int newTrackerIndex,IO::File& source)
	{
	/* Update the tracker index: */
	trackerIndex=newTrackerIndex;
	
	/* Read the face detector button index: */
	faceDetectorButtonIndex=(unsigned int)(source.read<Misc::UInt16>());
	
	/* Read the display latency: */
	displayLatency=int(source.read<Misc::SInt32>());
	
	/* Check which configuration components are being sent: */
	if(messageId&VRDeviceProtocol::MessageIdType(0x1U))
		{
		/* Read both eye positions: */
		for(int eye=0;eye<2;++eye)
			source.read<WScalar>(eyePos[eye].getComponents(),3);
		
		/* Calculate the inter-pupillary distance: */
		ipd=Scalar(Geometry::dist(eyePos[0],eyePos[1]));
		
		/* Invalidate the cached eye position: */
		if(++eyePosVersion==0U)
			++eyePosVersion;
		}
	if(messageId&VRDeviceProtocol::MessageIdType(0x2U))
		{
		/* Read both eyes' FoV boundaries: */
		for(int eye=0;eye<2;++eye)
			source.read<WScalar>(eyes[eye].fov,4);
		
		/* Invalidate the cached per-eye state: */
		if(++eyeVersion==0U)
			++eyeVersion;
		}
	if(messageId&VRDeviceProtocol::MessageIdType(0x4U))
		{
		/* Read recommended render target size: */
		source.read<WUInt,unsigned int>(renderTargetSize.getComponents(),2);
		
		/* Read distortion mesh size: */
		ISize newDistortionMeshSize;
		source.read<WUInt,unsigned int>(newDistortionMeshSize.getComponents(),2);
		if(distortionMeshSize!=newDistortionMeshSize)
			{
			/* Re-allocate both eyes' distortion mesh arrays: */
			distortionMeshSize=newDistortionMeshSize;
			for(int eye=0;eye<2;++eye)
				{
				delete[] eyes[eye].distortionMesh;
				eyes[eye].distortionMesh=new DistortionMeshVertex[distortionMeshSize.volume()];
				}
			}
		
		/* Read per-eye state: */
		for(int eye=0;eye<2;++eye)
			{
			/* Read eye's viewport: */
			source.read<WUInt,int>(eyes[eye].viewport.offset.getComponents(),2);
			source.read<WUInt,unsigned int>(eyes[eye].viewport.size.getComponents(),2);
			
			/* Read eye's distortion mesh: */
			DistortionMeshVertex* dmPtr=eyes[eye].distortionMesh;
			DistortionMeshVertex* dmEnd=dmPtr+distortionMeshSize.volume();
			for(;dmPtr!=dmEnd;++dmPtr)
				{
				source.read<WScalar>(dmPtr->red.getComponents(),2);
				source.read<WScalar>(dmPtr->green.getComponents(),2);
				source.read<WScalar>(dmPtr->blue.getComponents(),2);
				}
			}
		
		/* Invalidate the cached distortion mesh: */
		if(++distortionMeshVersion==0U)
			++distortionMeshVersion;
		}
	}

void HMDConfiguration::readEyeRotation(IO::File& source)
	{
	/* Read both eye rotations: */
	for(int eye=0;eye<2;++eye)
		{
		Scalar quat[4];
		source.read<WScalar>(quat,4);
		eyeRot[eye]=Rotation(quat);
		}
	
	/* Invalidate the cached eye rotation: */
	if(++eyeRotVersion==0U)
		++eyeRotVersion;
	}

}
