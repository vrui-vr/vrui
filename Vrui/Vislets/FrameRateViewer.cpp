/***********************************************************************
FrameRateViewer - Vislet class to view a live graph of Vrui frame times
for debugging and optimization purposes.
Copyright (c) 2015-2026 Oliver Kreylos

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

#include <Vrui/Vislets/FrameRateViewer.h>

#include <stdlib.h>
#include <string.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Misc/MessageLogger.h>
#include <Math/Math.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GL/GLColorTemplates.h>
#include <Vrui/Vrui.h>
#include <Vrui/VisletManager.h>
#include <Vrui/DisplayState.h>

#include <Vrui/Internal/Vrui.h>

namespace Vrui {

namespace Vislets {

/***************************************
Methods of class FrameRateViewerFactory:
***************************************/

FrameRateViewerFactory::FrameRateViewerFactory(VisletManager& visletManager)
	:VisletFactory("FrameRateViewer",visletManager)
	{
	#if 0
	/* Insert class into class hierarchy: */
	VisletFactory* visletFactory=visletManager.loadClass("Vislet");
	visletFactory->addChildClass(this);
	addParentClass(visletFactory);
	#endif
	
	/* Set vislet class' factory pointer: */
	FrameRateViewer::factory=this;
	}

FrameRateViewerFactory::~FrameRateViewerFactory(void)
	{
	/* Reset vislet class' factory pointer: */
	FrameRateViewer::factory=0;
	}

Vislet* FrameRateViewerFactory::createVislet(int numArguments,const char* const arguments[]) const
	{
	return new FrameRateViewer(numArguments,arguments);
	}

void FrameRateViewerFactory::destroyVislet(Vislet* vislet) const
	{
	delete vislet;
	}

extern "C" void resolveFrameRateViewerDependencies(Plugins::FactoryManager<VisletFactory>& manager)
	{
	#if 0
	/* Load base classes: */
	manager.loadClass("Vislet");
	#endif
	}

extern "C" VisletFactory* createFrameRateViewerFactory(Plugins::FactoryManager<VisletFactory>& manager)
	{
	/* Get pointer to vislet manager: */
	VisletManager* visletManager=static_cast<VisletManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	FrameRateViewerFactory* factory=new FrameRateViewerFactory(*visletManager);
	
	/* Return factory object: */
	return factory;
	}

extern "C" void destroyFrameRateViewerFactory(VisletFactory* factory)
	{
	delete factory;
	}

/****************************************
Static elements of class FrameRateViewer:
****************************************/

FrameRateViewerFactory* FrameRateViewer::factory=0;

/********************************
Methods of class FrameRateViewer:
********************************/

FrameRateViewer::FrameRateViewer(int numArguments,const char* const arguments[])
	:target(vruiState->animationFrameInterval*1.0e9),
	 min(0),max(0),oldestTotalDuration(0),
	 numberRenderer(12.0f,false)
	{
	/* Calculate the duration that should be at the top of the graph based on the target frame duration: */
	double step=Math::exp10(Math::floor(Math::log10(target)));
	top=Math::ceil(target/step)*step;
	
	/* Parse the command line: */
	for(int i=0;i<numArguments;++i)
		{
		if(arguments[i][0]=='-')
			{
			if(strcasecmp(arguments[i]+1,"t")==0||strcasecmp(arguments[i]+1,"top")==0)
				{
				++i;
				if(i<numArguments)
					top=atof(arguments[i]);
				else
					Misc::formattedConsoleError("FrameRateViewer: Ignoring dangling %s option",arguments[i-1]);
				}
			else
				Misc::formattedConsoleError("FrameRateViewer: Ignoring unknown %s option",arguments[i]);
			}
		else
			Misc::formattedConsoleError("FrameRateViewer: Ignoring unknown %s parameter",arguments[i]);
		}
	
	}

FrameRateViewer::~FrameRateViewer(void)
	{
	}

VisletFactory* FrameRateViewer::getFactory(void) const
	{
	return factory;
	}

void FrameRateViewer::frame(void)
	{
	/* Update the total frame duration history's range: */
	VruiState::FrameTiming& mostRecentFt=vruiState->frameTimings[(vruiState->nextFrameTimingsIndex+vruiState->numFrameTimings-1)%vruiState->numFrameTimings];
	if(min>mostRecentFt.totalDuration)
		min=mostRecentFt.totalDuration;
	if(max<mostRecentFt.totalDuration)
		max=mostRecentFt.totalDuration;
	
	/* Check if the just removed history entry was the min or max: */
	if(min==oldestTotalDuration||max==oldestTotalDuration)
		{
		/* Recalculate minimum and maximum: */
		VruiState::FrameTiming* ftPtr=vruiState->frameTimings;
		VruiState::FrameTiming* ftEnd=ftPtr+vruiState->numFrameTimings;
		min=max=ftPtr->totalDuration;
		for(++ftPtr;ftPtr!=ftEnd;++ftPtr)
			{
			if(min>ftPtr->totalDuration)
				min=ftPtr->totalDuration;
			else if(max<ftPtr->totalDuration)
				max=ftPtr->totalDuration;
			}
		}
	
	/* Remember the oldest total frame duration in the frame timings buffer: */
	oldestTotalDuration=vruiState->frameTimings[vruiState->nextFrameTimingsIndex].totalDuration;
	}

void FrameRateViewer::display(GLContextData& contextData) const
	{
	/* Get the viewport size of the current window: */
	const DisplayState& ds=getDisplayState(contextData);
	
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
	glDisable(GL_LIGHTING);
	glLineWidth(1.0f);
	
	/* Go to pixel coordinates: */
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0,ds.viewport.size[0],0.0,ds.viewport.size[1],0.0,1.0);
	
	/* Calculate graph offsets and scaling factors: */
	double xs=double(ds.viewport.size[0])*0.8/double(vruiState->numFrameTimings);
	double x0=double(ds.viewport.size[0])*0.15;
	double ys=double(ds.viewport.size[1])*0.2/top;
	double y0=double(ds.viewport.size[1])*0.05;
	
	/* Draw the frame rate graph: */
	glBegin(GL_LINES);
	double x=x0;
	const VruiState::FrameTiming* ftEnd=vruiState->frameTimings+vruiState->numFrameTimings;
	for(const VruiState::FrameTiming* ftPtr=vruiState->frameTimings+vruiState->nextFrameTimingsIndex;ftPtr!=ftEnd;++ftPtr,x+=xs)
		{
		glColor3f(1.0f,0.0f,0.0f);
		glVertex2d(x,y0);
		glVertex2d(x,y0+double(ftPtr->renderStart)*ys);
		glColor3f(1.0f,1.0f,0.0f);
		glVertex2d(x,y0+double(ftPtr->renderStart)*ys);
		glVertex2d(x,y0+double(ftPtr->renderEnd)*ys);
		glColor3f(0.0f,1.0f,0.0f);
		glVertex2d(x,y0+double(ftPtr->renderEnd)*ys);
		glVertex2d(x,y0+double(ftPtr->present)*ys);
		glColor3f(0.0f,1.0f,1.0f);
		glVertex2d(x,y0+double(ftPtr->present)*ys);
		glVertex2d(x,y0+double(ftPtr->postRenderEnd)*ys);
		glColor3f(0.0f,0.0f,1.0f);
		glVertex2d(x,y0+double(ftPtr->postRenderEnd)*ys);
		glVertex2d(x,y0+double(ftPtr->totalDuration)*ys);
		}
	ftEnd=vruiState->frameTimings+vruiState->nextFrameTimingsIndex;
	for(const VruiState::FrameTiming* ftPtr=vruiState->frameTimings;ftPtr!=ftEnd;++ftPtr,x+=xs)
		{
		glColor3f(1.0f,0.0f,0.0f);
		glVertex2d(x,y0);
		glVertex2d(x,y0+double(ftPtr->renderStart)*ys);
		glColor3f(1.0f,1.0f,0.0f);
		glVertex2d(x,y0+double(ftPtr->renderStart)*ys);
		glVertex2d(x,y0+double(ftPtr->renderEnd)*ys);
		glColor3f(0.0f,1.0f,0.0f);
		glVertex2d(x,y0+double(ftPtr->renderEnd)*ys);
		glVertex2d(x,y0+double(ftPtr->present)*ys);
		glColor3f(0.0f,1.0f,1.0f);
		glVertex2d(x,y0+double(ftPtr->present)*ys);
		glVertex2d(x,y0+double(ftPtr->postRenderEnd)*ys);
		glColor3f(0.0f,0.0f,1.0f);
		glVertex2d(x,y0+double(ftPtr->postRenderEnd)*ys);
		glVertex2d(x,y0+double(ftPtr->totalDuration)*ys);
		}
	
	/* Get graph colors: */
	Color bg=getBackgroundColor();
	Color fg=getForegroundColor();
	
	/* Draw a grid: */
	glColor3f(Math::mid(bg[0],fg[0]),Math::mid(bg[1],fg[1]),Math::mid(bg[2],fg[2]));
	
	/* Draw the bottom line: */
	glVertex2d(x0-5.0,y0);
	glVertex2d(x0+double(vruiState->numFrameTimings)*xs+5.0,y0);
	
	/* Draw intermediate and top lines: */
	for(int i=1;i<=10;++i)
		{
		double l=top*double(i)/10.0;
		glVertex2d(x0-5.0,y0+l*ys);
		glVertex2d(x0+double(vruiState->numFrameTimings)*xs+5.0,y0+l*ys);
		}
	
	/* Draw the target frame rate: */
	glColor3f(1.0f,0.0f,0.0f);
	glVertex2d(x0-5.0,y0+target*ys);
	glVertex2d(x0+double(vruiState->numFrameTimings)*xs+5.0,y0+target*ys);
	
	glEnd();
	
	/* Draw bottom and top labels: */
	glColor(fg);
	numberRenderer.drawNumber(GLNumberRenderer::Vector(x0-10.0,y0,0.0),0.0,2,contextData,1,0);
	numberRenderer.drawNumber(GLNumberRenderer::Vector(x0-10.0,y0+top*0.5*ys,0.0),top/2.0e6,2,contextData,1,0);
	numberRenderer.drawNumber(GLNumberRenderer::Vector(x0-10.0,y0+top*ys,0.0),top/1.0e6,2,contextData,1,0);
	
	/* Restore OpenGL state: */
	glPopAttrib();
	
	/* Return to physical coordinates: */
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	}

}

}
