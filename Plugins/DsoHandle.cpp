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

#include <Plugins/DsoHandle.h>

#include <dlfcn.h>

namespace Plugins {

/**************************
Methods of class DsoHandle:
**************************/

void DsoHandle::releaseDso(void)
	{
	/* Close a currently open DSO: */
	if(handle!=0)
		dlclose(handle);
	}

bool DsoHandle::open(const char* fileName,int flags)
	{
	/* Close a currently open DSO: */
	if(handle!=0)
		dlclose(handle);
	
	/* Load a new DSO: */
	handle=dlopen(fileName,flags);
	
	return handle!=0;
	}

void* DsoHandle::resolve(const char* symbol)
	{
	/* Resolve the given symbol: */
	return dlsym(handle,symbol);
	}

}
