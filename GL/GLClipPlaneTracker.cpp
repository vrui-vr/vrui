/***********************************************************************
GLClipPlaneTracker - Class to keep track of changes to OpenGL's clipping
plane state to support just-in-time compilation of GLSL shaders
depending on the OpenGL context's current clipping state.
Copyright (c) 2012-2022 Oliver Kreylos

This file is part of the OpenGL Support Library (GLSupport).

The OpenGL Support Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The OpenGL Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the OpenGL Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <GL/GLClipPlaneTracker.h>

#include <Misc/PrintInteger.h>
#include <GL/gl.h>
#include <GL/Extensions/GLARBShaderObjects.h>

/***********************************
Methods of class GLClipPlaneTracker:
***********************************/

GLClipPlaneTracker::GLClipPlaneTracker(void)
	:version(0),
	 maxNumClipPlanes(0),clipPlaneStates(0),clipPlaneEnableds(0)
	{
	/* Determine the maximum number of clipping planes supported by the local OpenGL: */
	glGetIntegerv(GL_MAX_CLIP_PLANES,&maxNumClipPlanes);
	
	/* Initialize the clipping plane state arrays: */
	clipPlaneStates=new ClipPlaneState[maxNumClipPlanes];
	clipPlaneEnableds=new GLint[maxNumClipPlanes];
	for(int i=0;i<maxNumClipPlanes;++i)
		{
		clipPlaneStates[i].enabled=false;
		clipPlaneStates[i].plane=Plane(0.0,0.0,0.0,0.0);
		clipPlaneEnableds[i]=GLint(0);
		}
	
	/* Query the current clipping plane state: */
	update();
	
	/* Reset the version number to one, even if there was no change: */
	version=1;
	}

GLClipPlaneTracker::~GLClipPlaneTracker(void)
	{
	/* Destroy the clipping plane state arrays: */
	delete[] clipPlaneStates;
	delete[] clipPlaneEnableds;
	}

bool GLClipPlaneTracker::update(void)
	{
	/* Let the tracked clipping plane state reflect the new setting and remember whether clipping plane state changed: */
	bool changed=false;
	
	/* Check all clipping plane's enabled states: */
	for(int clipPlaneIndex=0;clipPlaneIndex<maxNumClipPlanes;++clipPlaneIndex)
		{
		/* Get the clipping plane's enabled flag: */
		bool enabled=glIsEnabled(GL_CLIP_PLANE0+clipPlaneIndex);
		changed=changed||clipPlaneStates[clipPlaneIndex].enabled!=enabled;
		clipPlaneStates[clipPlaneIndex].enabled=enabled;
		
		if(clipPlaneStates[clipPlaneIndex].enabled)
			{
			/* Query the clipping plane's plane equation: */
			glGetClipPlane(GL_CLIP_PLANE0+clipPlaneIndex,clipPlaneStates[clipPlaneIndex].plane.getXyzw());
			}
		
		clipPlaneEnableds[clipPlaneIndex]=enabled?GLint(1):GLint(0);
		}
	
	/* Update the version number if there was a change: */
	if(changed)
		++version;
	
	return changed;
	}

int GLClipPlaneTracker::getNumEnabledClipPlanes(void) const
	{
	/* Count the number of enabled clipping planes: */
	int result=0;
	for(int i=0;i<maxNumClipPlanes;++i)
		if(clipPlaneStates[i].enabled)
			++result;
	
	return result;
	}

bool GLClipPlaneTracker::enableClipPlane(int clipPlaneIndex,const GLClipPlaneTracker::Plane& plane,bool enable)
	{
	/* Let the tracked clipping plane state reflect the new setting and remember whether clipping plane state changed: */
	bool changed=false;
	
	/* Check the clipping plane's enabled state: */
	changed=changed||!clipPlaneStates[clipPlaneIndex].enabled;
	clipPlaneStates[clipPlaneIndex].enabled=true;
	
	/* Update the clipping plane's plane equation: */
	clipPlaneStates[clipPlaneIndex].plane=plane;
	
	/* Update the clipping plane state version number if anything changed: */
	if(changed)
		++version;
	
	/* Pass the clipping plane change through to OpenGL: */
	if(enable)
		glEnable(GL_CLIP_PLANE0+clipPlaneIndex);
	glClipPlane(GL_CLIP_PLANE0+clipPlaneIndex,clipPlaneStates[clipPlaneIndex].plane.getXyzw());
	
	/* Read the clipping plane back immediately to store it in eye coordinates (sucky): */
	glGetClipPlane(GL_CLIP_PLANE0+clipPlaneIndex,clipPlaneStates[clipPlaneIndex].plane.getXyzw());
	
	/* Update the enabled flag array: */
	clipPlaneEnableds[clipPlaneIndex]=GLint(1);
	
	return changed;
	}

bool GLClipPlaneTracker::enableClipPlane(int clipPlaneIndex,const GLClipPlaneTracker::Vector& planeNormal,const GLClipPlaneTracker::Vector& planePoint,bool enable)
	{
	/* Let the tracked clipping plane state reflect the new setting and remember whether clipping plane state changed: */
	bool changed=false;
	
	/* Check the clipping plane's enabled state: */
	changed=changed||!clipPlaneStates[clipPlaneIndex].enabled;
	clipPlaneStates[clipPlaneIndex].enabled=true;
	
	/* Convert the affine plane equation to a homogeneous plane equation: */
	Plane plane;
	plane[3]=0.0;
	for(int i=0;i<3;++i)
		{
		plane[i]=planeNormal[i];
		plane[3]-=planeNormal[i]*planePoint[i];
		}
	
	/* Update the clipping plane's plane equation: */
	clipPlaneStates[clipPlaneIndex].plane=plane;
	
	/* Update the clipping plane state version number if anything changed: */
	if(changed)
		++version;
	
	/* Pass the clipping plane change through to OpenGL: */
	if(enable)
		glEnable(GL_CLIP_PLANE0+clipPlaneIndex);
	glClipPlane(GL_CLIP_PLANE0+clipPlaneIndex,plane.getXyzw());
	
	/* Read the clipping plane back immediately to store it in eye coordinates (sucky): */
	glGetClipPlane(GL_CLIP_PLANE0+clipPlaneIndex,clipPlaneStates[clipPlaneIndex].plane.getXyzw());
	
	/* Update the enabled flag array: */
	clipPlaneEnableds[clipPlaneIndex]=GLint(1);
	
	return changed;
	}

bool GLClipPlaneTracker::disableClipPlane(int clipPlaneIndex)
	{
	/* Let the tracked clipping plane state reflect the new setting and remember whether clipping plane state changed: */
	bool changed=false;
	
	/* Check the clipping plane's enabled state: */
	changed=changed||clipPlaneStates[clipPlaneIndex].enabled;
	clipPlaneStates[clipPlaneIndex].enabled=false;
	
	/* Update the clipping plane state version number if anything changed: */
	if(changed)
		++version;
	
	/* Pass the clipping plane change through to OpenGL: */
	glDisable(GL_CLIP_PLANE0+clipPlaneIndex);
	
	/* Update the enabled flag array: */
	clipPlaneEnableds[clipPlaneIndex]=GLint(0);
	
	return changed;
	}

int GLClipPlaneTracker::addClipPlane(const GLClipPlaneTracker::Plane& plane)
	{
	/* Find an unused clipping plane: */
	int clipPlaneIndex;
	for(clipPlaneIndex=0;clipPlaneIndex<maxNumClipPlanes&&clipPlaneStates[clipPlaneIndex].enabled;++clipPlaneIndex)
		;
	
	/* Bail out if all clipping planes are already used: */
	if(clipPlaneIndex==maxNumClipPlanes)
		return -1;
	
	/* Enable the found clipping plane: */
	clipPlaneStates[clipPlaneIndex].enabled=true;
	
	/* Update the found clipping plane's plane equation: */
	clipPlaneStates[clipPlaneIndex].plane=plane;
	
	/* Pass the clipping plane change through to OpenGL: */
	glEnable(GL_CLIP_PLANE0+clipPlaneIndex);
	glClipPlane(GL_CLIP_PLANE0+clipPlaneIndex,clipPlaneStates[clipPlaneIndex].plane.getXyzw());
	
	/* Read the clipping plane back immediately to store it in eye coordinates (sucky): */
	glGetClipPlane(GL_CLIP_PLANE0+clipPlaneIndex,clipPlaneStates[clipPlaneIndex].plane.getXyzw());
	
	/* Invalidate current clipping plane state: */
	++version;
	
	/* Update the enabled flag array: */
	clipPlaneEnableds[clipPlaneIndex]=GLint(1);
	
	return clipPlaneIndex;
	}

int GLClipPlaneTracker::addClipPlane(const GLClipPlaneTracker::Vector& planeNormal,const GLClipPlaneTracker::Vector& planePoint)
	{
	/* Find an unused clipping plane: */
	int clipPlaneIndex;
	for(clipPlaneIndex=0;clipPlaneIndex<maxNumClipPlanes&&clipPlaneStates[clipPlaneIndex].enabled;++clipPlaneIndex)
		;
	
	/* Bail out if all clipping planes are already used: */
	if(clipPlaneIndex==maxNumClipPlanes)
		return -1;
	
	/* Enable the found clipping plane: */
	clipPlaneStates[clipPlaneIndex].enabled=true;
	
	/* Convert the affine plane equation to a homogeneous plane equation: */
	Plane plane;
	plane[3]=0.0;
	for(int i=0;i<3;++i)
		{
		plane[i]=planeNormal[i];
		plane[3]-=planeNormal[i]*planePoint[i];
		}
	
	/* Update the found clipping plane's plane equation: */
	clipPlaneStates[clipPlaneIndex].plane=plane;
	
	/* Pass the clipping plane change through to OpenGL: */
	glEnable(GL_CLIP_PLANE0+clipPlaneIndex);
	glClipPlane(GL_CLIP_PLANE0+clipPlaneIndex,clipPlaneStates[clipPlaneIndex].plane.getXyzw());
	
	/* Read the clipping plane back immediately to store it in eye coordinates (sucky): */
	glGetClipPlane(GL_CLIP_PLANE0+clipPlaneIndex,clipPlaneStates[clipPlaneIndex].plane.getXyzw());
	
	/* Invalidate current clipping plane state: */
	++version;
	
	/* Update the enabled flag array: */
	clipPlaneEnableds[clipPlaneIndex]=GLint(1);
	
	return clipPlaneIndex;
	}

void GLClipPlaneTracker::uploadClipPlaneEnableds(GLint uniformLocation) const
	{
	/* Upload the enabled array: */
	glUniform1ivARB(uniformLocation,maxNumClipPlanes,clipPlaneEnableds);
	}

std::string GLClipPlaneTracker::createCalcClipDistances(const char* vertexEc) const
	{
	/* Create code to calculate the vertex' position relative to all user-specified clipping planes: */
	std::string result;
	for(int clipPlaneIndex=0;clipPlaneIndex<maxNumClipPlanes;++clipPlaneIndex)
		if(clipPlaneStates[clipPlaneIndex].enabled)
			{
			char cpiBuffer[12];
			const char* cpiString=Misc::print(clipPlaneIndex,cpiBuffer+11);
			result+="\tgl_ClipDistance[";
			result.append(cpiString);
			result+="]=dot(gl_ClipPlane[";
			result.append(cpiString);
			result+="],";
			result.append(vertexEc);
			result+=");\n";
			}
	
	return result;
	}

void GLClipPlaneTracker::pause(void) const
	{
	for(int i=0;i<maxNumClipPlanes;++i)
		if(clipPlaneStates[i].enabled)
			glDisable(GL_CLIP_PLANE0+i);
	}

void GLClipPlaneTracker::resume(void) const
	{
	for(int i=0;i<maxNumClipPlanes;++i)
		if(clipPlaneStates[i].enabled)
			glEnable(GL_CLIP_PLANE0+i);
	}
