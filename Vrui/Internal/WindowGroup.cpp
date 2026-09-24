/***********************************************************************
WindowGroup - Class representing a group of windows sharing the same
OpenGL context and X11 display connection.
Copyright (c) 2000-2026 Oliver Kreylos

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

#include <Vrui/Internal/WindowGroup.h>

#include <stdexcept>
#include <iostream>
#include <Misc/StringPrintf.h>
#include <Misc/StandardHashFunction.h>
#include <Misc/StringHashFunctions.h>
#include <Misc/StandardValueCoders.h>
#include <Threads/FunctionCalls.h>
#include <Vrui/VRWindow.h>
#include <Vrui/Internal/Vrui.h>

/* External global variables: */
namespace Vrui {
extern bool vruiVerbose;
extern VruiErrorHeader vruiErrorHeader;
}

namespace Vrui {

/****************************
Methods of class WindowGroup:
****************************/

void WindowGroup::displayWatcherCallback(Threads::IOWatcherEvent& event)
	{
	/* Mark the window group's socket as ready: */
	socketReady=true;
	
	/* Enable the event dispatching process function: */
	eventDispatcher->enable();
	}

void WindowGroup::eventDispatcherCallback(Threads::ProcessFunction& processFunction)
	{
	/* We potentially handle events twice; first, those that are already in the display connection's event queue; second, those that are waiting to be read from the connection's socket: */
	while(numEventsInQueue>0||socketReady)
		{
		/* Handle all events in the event queue: */
		while(numEventsInQueue>0)
			{
			/* Grab the next event: */
			XEvent event;
			XNextEvent(display,&event); // This is guaranteed not to block...
			--numEventsInQueue;
			
			/* Check if this event is a repeated key press, signaled by a key release immediately followed by a key press for the same key with the same time stamp: */
			bool dispatchEvent=true;
			if(event.type==KeyRelease&&numEventsInQueue>0)
				{
				/* Peek at the next event: */
				XEvent nextEvent;
				XPeekEvent(display,&nextEvent);
				
				/* Don't dispatch this event if the next event matches it: */
				dispatchEvent=nextEvent.type!=KeyPress||nextEvent.xkey.keycode!=event.xkey.keycode||nextEvent.xkey.time!=event.xkey.time;
				}
			
			if(dispatchEvent)
				{
				/* Pass the event to all windows interested in it: */
				for(std::vector<Window>::iterator wIt=windows.begin();wIt!=windows.end();++wIt)
					if(wIt->window->isEventForWindow(event))
						wIt->window->processEvent(event);
				}
			}
		
		/* Read pending events from the display connection's socket into the event queue: */
		if(socketReady)
			numEventsInQueue=XEventsQueued(display,QueuedAfterReading);
		
		/* Don't read from the display connection's socket again: */
		socketReady=false;
		}
	
	/* All events have been handled; disable the process function until more arrive: */
	eventDispatcher->disable();
	}

WindowGroup::WindowGroup(void)
	:display(0),displayFd(-1),numEventsInQueue(0),socketReady(false),displayState(0)
	{
	}

WindowGroup::~WindowGroup(void)
	{
	}

WindowGroup::CreatorMap WindowGroup::collectWindowGroups(const char* applicationName,const std::vector<std::string>& windowNames,VRWindow** resultWindows,Misc::ConfigurationFile& configFile)
	{
	/* Create a map from display names to default group IDs: */
	typedef Misc::HashTable<std::string,unsigned int> DisplayGroupMap;
	DisplayGroupMap displayGroups(7);
	
	/* Sort the windows into groups based on their group IDs and collect each group's OpenGL context properties: */
	CreatorMap creatorMap(7);
	unsigned int nextGroupId=0;
	unsigned int numWindows=windowNames.size();
	for(unsigned int windowIndex=0;windowIndex<numWindows;++windowIndex)
		{
		/* Go to the window's configuration section: */
		Misc::ConfigurationFileSection windowSection=configFile.getSection(windowNames[windowIndex].c_str());
		
		/* Read the name of the window's X display: */
		std::pair<std::string,int> displayName=VRWindow::getDisplayName(windowSection);
		
		/* Create a default group ID for the window: */
		DisplayGroupMap::Iterator dgIt=displayGroups.findEntry(displayName.first);
		unsigned int groupId=dgIt.isFinished()?nextGroupId:dgIt->getDest();
		
		/* Overwrite the window's default group ID from its configuration section: */
		windowSection.updateValue("./groupId",groupId);
		
		/* Look for the group ID in the window groups hash table: */
		CreatorMap::Iterator wgIt=creatorMap.findEntry(groupId);
		if(wgIt.isFinished())
			{
			/* Start a new window group: */
			wgIt=creatorMap.setAndFindEntry(CreatorMap::Entry(groupId,Creator(applicationName,numWindows,resultWindows,groupId)));
			wgIt->getDest().displayName=displayName.first;
			wgIt->getDest().screen=displayName.second;
			vruiState->windowProperties.setContextProperties(wgIt->getDest().contextProperties);
			
			/* Associate the new window group with this window's display name: */
			displayGroups.setEntry(DisplayGroupMap::Entry(displayName.first,groupId));
			if(nextGroupId<=groupId)
				nextGroupId=groupId+1;
			}
		
		/* Add this window to the new or existing window group: */
		Creator::Window newWindow;
		newWindow.windowIndex=windowIndex;
		newWindow.windowConfigFileSection=windowSection;
		wgIt->getDest().windows.push_back(newWindow);
		VRWindow::updateContextProperties(wgIt->getDest().contextProperties,windowSection);
		}
	
	return creatorMap;
	}

bool WindowGroup::initialize(const WindowGroup::Creator& creator,const std::string& syncWindowName,InputDeviceAdapterMouse* mouseAdapter,InputDeviceAdapterMultitouch* multitouchAdapter)
	{
	if(vruiVerbose)
		{
		std::cout<<vruiErrorHeader<<"Creating window group "<<creator.groupId<<" containing "<<creator.windows.size()<<(creator.windows.size()!=1?" windows":" window")<<" with visual type";
		if(creator.contextProperties.direct)
			{
			std::cout<<" direct";
			if(creator.contextProperties.stereo)
				std::cout<<" stereo";
			if(creator.contextProperties.numSamples>1)
				std::cout<<" with "<<creator.contextProperties.numSamples<<" samples per pixel";
			}
		else
			{
			std::cout<<" indirect";
			if(creator.contextProperties.backbuffer)
				std::cout<<" double-buffered";
			}
		std::cout<<std::endl;
		}
	
	/* Create an OpenGL context for this window group: */
	context=new GLContext(creator.displayName.c_str());
	context->initialize(creator.screen,creator.contextProperties);
	display=context->getDisplay();
	displayFd=ConnectionNumber(display);
	
	// DEBUGGING
	// XSynchronize(display,true);
	
	/* Create all windows in this group: */
	maxViewportSize=ISize(0,0);
	maxFrameSize=ISize(0,0);
	bool firstWindow=true;
	bool allWindowsOk=true;
	for(std::vector<Creator::Window>::const_iterator wIt=creator.windows.begin();wIt!=creator.windows.end();++wIt)
		{
		try
			{
			/* Assign a unique name to the window: */
			std::string windowName=creator.applicationName;
			if(creator.numWindows>1)
				windowName+=Misc::stringPrintf(" - %d",wIt->windowIndex);
			if(vruiVerbose)
				std::cout<<vruiErrorHeader<<"Opening window "<<windowName<<" from configuration section "<<wIt->windowConfigFileSection.getName()<<':'<<std::endl;
			
			/* Create the new window and add it to this window group's list: */
			Window newWindow;
			newWindow.window=VRWindow::createWindow(*context,windowName.c_str(),wIt->windowConfigFileSection);
			newWindow.window->makeCurrent();
			newWindow.viewportSize=ISize(0,0);
			newWindow.frameSize=ISize(0,0);
			windows.push_back(newWindow);
			creator.resultWindows[wIt->windowIndex]=newWindow.window;
			
			/* Check if this was the first window in this group: */
			if(firstWindow)
				{
				/* Register this group's OpenGL context with the Vrui kernel: */
				displayState=vruiState->registerContext(*context);
			
				/* Initialize all GLObjects for the group's context data: */
				context->getContextData().updateThings();
				}
			firstWindow=false;
			
			/* Initialize the new window: */
			newWindow.window->setVruiState(vruiState,wIt->windowConfigFileSection.getName()==syncWindowName);
			newWindow.window->setWindowGroup(this);
			if(mouseAdapter!=0)
				newWindow.window->setMouseAdapter(mouseAdapter,wIt->windowConfigFileSection);
			if(multitouchAdapter!=0)
				newWindow.window->setMultitouchAdapter(multitouchAdapter,wIt->windowConfigFileSection);
			newWindow.window->setDisplayState(displayState,wIt->windowConfigFileSection);
			newWindow.window->init(wIt->windowConfigFileSection);
			
			/* Let Vrui quit when the window is closed: */
			newWindow.window->getCloseCallbacks().add(vruiState,&VruiState::quitCallback);
			}
		catch(const std::runtime_error& err)
			{
			std::cerr<<vruiErrorHeader<<"Caught exception "<<err.what()<<" while initializing rendering window "<<wIt->windowIndex<<std::endl;
			
			/* Bail out: */
			allWindowsOk=false;
			break;
			}
		}
	
	if(allWindowsOk)
		{
		/* Create an I/O watcher for the display connection's socket: */
		displayWatcher=new Threads::IOWatcher(vruiState->runLoop,displayFd,Threads::IOWatcher::Read,true,*Threads::createFunctionCall(this,&WindowGroup::displayWatcherCallback));
		
		/* Check if there are unhandled events in the display connection's event queue: */
		numEventsInQueue=XQLength(display);
		socketReady=false;
		
		/* Create a process function to dispatch X11 events to windows when there are events: */
		eventDispatcher=new Threads::ProcessFunction(vruiState->runLoop,true,numEventsInQueue>0,*Threads::createFunctionCall(this,&WindowGroup::eventDispatcherCallback));
		}
	
	return allWindowsOk;
	}

void WindowGroup::resizeWindow(const VRWindow* window,const ISize& newViewportSize,const ISize& newFrameSize)
	{
	/* Find the window in the window list: */
	for(std::vector<Window>::iterator wIt=windows.begin();wIt!=windows.end();++wIt)
		if(wIt->window==window)
			{
			/* Check if the window's viewport got bigger in both directions: */
			bool viewportBigger=wIt->viewportSize[0]<=newViewportSize[0]&&wIt->viewportSize[1]<=newViewportSize[1];
			
			/* Update the window's viewport size: */
			wIt->viewportSize=newViewportSize;
			
			if(viewportBigger)
				{
				/* Simply update this window group's maximum viewport size: */
				maxViewportSize.max(newViewportSize);
				}
			else
				{
				/* Recalculate this window group's maximum viewport size from scratch: */
				std::vector<Window>::iterator w2It=windows.begin();
				maxViewportSize=w2It->viewportSize;
				for(++w2It;w2It!=windows.end();++w2It)
					maxViewportSize.max(w2It->viewportSize);
				}
			
			/* Check if the window's frame buffer got bigger: */
			bool frameBigger=wIt->frameSize[0]<=newFrameSize[0]&&wIt->frameSize[1]<=newFrameSize[1];
			
			/* Update the window's frame buffer size: */
			wIt->frameSize=newFrameSize;
			
			if(frameBigger)
				{
				/* Simply update this window group's maximum frame buffer size: */
				maxFrameSize.max(newFrameSize);
				}
			else
				{
				/* Recalculate this window group's maximum frame buffer size from scratch: */
				std::vector<Window>::iterator w2It=windows.begin();
				maxFrameSize=w2It->frameSize;
				for(++w2It;w2It!=windows.end();++w2It)
					maxFrameSize.max(w2It->frameSize);
				}
			
			break;
			}
	}

bool WindowGroup::dispatchXEvents(void)
	{
	bool result=false;
	
	/* We potentially handle events twice; first, those that are already in the display connection's event queue; second, those that are waiting to be read from the connection's socket: */
	while(numEventsInQueue>0||socketReady)
		{
		/* Handle all events in the event queue: */
		while(numEventsInQueue>0)
			{
			/* Grab the next event: */
			XEvent event;
			XNextEvent(display,&event); // This is guaranteed not to block...
			--numEventsInQueue;
			
			/* Check if this event is a repeated key press, signaled by a key release immediately followed by a key press for the same key with the same time stamp: */
			bool dispatchEvent=true;
			if(event.type==KeyRelease&&numEventsInQueue>0)
				{
				/* Peek at the next event: */
				XEvent nextEvent;
				XPeekEvent(display,&nextEvent);
				
				/* Don't dispatch this event if the next event matches it: */
				dispatchEvent=nextEvent.type!=KeyPress||nextEvent.xkey.keycode!=event.xkey.keycode||nextEvent.xkey.time!=event.xkey.time;
				}
			
			if(dispatchEvent)
				{
				/* Pass the event to all windows interested in it: */
				for(std::vector<Window>::iterator wIt=windows.begin();wIt!=windows.end();++wIt)
					if(wIt->window->isEventForWindow(event))
						wIt->window->processEvent(event);
				
				/* Tell the caller that we handled at least one event: */
				result=true;
				}
			}
		
		/* Read pending events from the display connection's socket into the event queue: */
		if(socketReady)
			numEventsInQueue=XEventsQueued(display,QueuedAfterReading);
		
		/* Don't read from the display connection's socket again: */
		socketReady=false;
		}
	
	return result;
	}

void WindowGroup::draw(void)
	{
	/* Initialize the display state object: */
	displayState->maxViewportSize=maxViewportSize;
	displayState->maxFrameSize=maxFrameSize;
	
	/* Make the shared OpenGL context current with the first window: */
	std::vector<Window>::iterator wIt=windows.begin();
	wIt->window->makeCurrent();
	
	/* Update all GLObjects for the OpenGL context: */
	context->getContextData().updateThings();
	
	/* Call all pre-rendering callbacks: */
	{
	PreRenderingCallbackData cbData(context->getContextData());
	vruiState->preRenderingCallbacks.call(&cbData);
	}
	
	/* Draw the first window: */
	wIt->window->draw();
	
	/* Draw all remaining windows: */
	for(++wIt;wIt!=windows.end();++wIt)
		{
		wIt->window->makeCurrent();
		wIt->window->draw();
		}
	
	/* Flush the OpenGL context shared by all windows in this group to guarantee timely completion: */
	glFlush();
	}

void WindowGroup::wait(void)
	{
	/* Wait until all windows are done rendering: */
	for(std::vector<Window>::iterator wIt=windows.begin();wIt!=windows.end();++wIt)
		{
		wIt->window->makeCurrent();
		wIt->window->waitComplete();
		}
	}

void WindowGroup::present(void)
	{
	/* Present all windows: */
	for(std::vector<Window>::iterator wIt=windows.begin();wIt!=windows.end();++wIt)
		{
		wIt->window->makeCurrent();
		wIt->window->present();
		}
	
	/* Since any of the rendering calls may internally have read from the display connection's socket, it's now time to query the event queue's size: */
	numEventsInQueue=XQLength(display);
	
	/* Enable the event dispatching process function if there are events in the queue: */
	if(numEventsInQueue>0)
		eventDispatcher->enable();
	}

void WindowGroup::releaseGLState(void)
	{
	/* Release the OpenGL states of all windows in this group: */
	for(std::vector<Window>::iterator wIt=windows.begin();wIt!=windows.end();++wIt)
		{
		wIt->window->makeCurrent();
		wIt->window->releaseGLState();
		}
	
	/* De-initialize the shared OpenGL context; this is okay to call because the context will be current with the last window used above: */
	context->deinit();
	}

}
