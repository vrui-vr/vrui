/***********************************************************************
XlibDisplay - Class to represent an Xlib display connection to an X
server.
Copyright (c) 2022-2023 Oliver Kreylos

This file is part of the Vulkan Xlib Bindings Library (VulkanXlib).

The Vulkan Xlib Bindings Library is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Vulkan Xlib Bindings Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vulkan Xlib Bindings Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
MA 02111-1307 USA
***********************************************************************/

#ifndef VULKANXLIB_XLIBDISPLAY_INCLUDED
#define VULKANXLIB_XLIBDISPLAY_INCLUDED

#include <X11/Xlib.h>

namespace VulkanXlib {

class XlibDisplay
	{
	/* Elements: */
	private:
	Display* display; // Xlib display connection handle
	
	/* Constructors and destructors: */
	public:
	XlibDisplay(const char* displayName); // Creates an Xlib connection to the X server of the given name
	~XlibDisplay(void); // Closes the Xlib connection to the X server
	
	/* Methods: */
	Display* getDisplay(void) const // Returns the Xlib display connection handle
		{
		return display;
		}
	};

}

#endif
