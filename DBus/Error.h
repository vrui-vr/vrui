/***********************************************************************
Error - Class wrapping an error notification object.
Copyright (c) 2026 Oliver Kreylos

This file is part of the DBus C++ Wrapper Library (DBus).

The DBus C++ Wrapper Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The DBus C++ Wrapper Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the DBus C++ Wrapper Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef DBUS_ERROR_INCLUDED
#define DBUS_ERROR_INCLUDED

#include <dbus/dbus.h>

namespace DBus {

class Error:public DBusError
	{
	/* Constructors and destructors: */
	public:
	Error(void) // Initializes the error
		{
		dbus_error_init(this);
		}
	~Error(void) // Frees the error in case it has been set
		{
		dbus_error_free(this);
		}
	
	/* Methods: */
	bool isSet(void) const // Returns true if the error is set, i.e., an error has occurred
		{
		return dbus_error_is_set(this);
		}
	bool hasName(const char* name) const // Returns true if the error is set and has the given name
		{
		return dbus_error_has_name(this,name);
		}
	};

}

#endif
