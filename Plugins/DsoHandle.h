/***********************************************************************
DsoHandle - Class to manage a DSO handle received from dlopen using
RAII.
Copyright (c) 2026 Oliver Kreylos

This file is part of the Plugin Handling Library (Plugins).

The Plugin Handling Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Plugin Handling Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Plugin Handling Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef PLUGINS_DSOHANDLE_INCLUDED
#define PLUGINS_DSOHANDLE_INCLUDED

namespace Plugins {

class DsoHandle
	{
	/* Elements: */
	private:
	void* handle; // The low-level DSO handle
	
	/* Private methods: */
	void releaseDso(void); // Releases a currently open DSO
	
	/* Constructors and destructors: */
	public:
	DsoHandle(void) // Creates an invalid DSO handle
		:handle(0)
		{
		}
	private:
	DsoHandle(const DsoHandle& source); // Prohibit copy constructor
	public:
	DsoHandle(DsoHandle&& source) // Move constructor
		:handle(source.handle)
		{
		/* Invalidate the source: */
		source.handle=0;
		}
	~DsoHandle(void) // Closes an opened DSO
		{
		releaseDso();
		}
	
	/* Methods: */
	private:
	DsoHandle& operator=(const DsoHandle& source); // Prohibit copy assignment operator
	public:
	DsoHandle& operator=(DsoHandle&& source) // Move assignment operator
		{
		if(this!=&source)
			{
			/* Close a currently open DSO and take over the source's DSO, then invalidate the source: */
			releaseDso();
			handle=source.handle;
			source.handle=0;
			}
		return *this;
		}
	bool valid(void) const // Returns true if the DSO handle refers to an opened DSO
		{
		return handle!=0;
		}
	bool open(const char* fileName,int flags); // Opens a DSO from the given file name and applies the given flags; returns true if the DSO was successfully opened
	void* resolve(const char* symbol); // Resolves the symbol of the given name from the DSO
	};

}

#endif
