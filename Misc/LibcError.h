/***********************************************************************
LibcError - Child class of Misc::RuntimeError for errors reported by the
C library.
Copyright (c) 2023 Oliver Kreylos

This file is part of the Miscellaneous Support Library (Misc).

The Miscellaneous Support Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Miscellaneous Support Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Miscellaneous Support Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef MISC_LIBCERROR_INCLUDED
#define MISC_LIBCERROR_INCLUDED

#include <Misc/RuntimeError.h>

namespace Misc {

class LibcError:public RuntimeError
	{
	/* Elements: */
	private:
	int error; // The C library error code (errno)
	
	/* Constructors and destructors: */
	public:
	LibcError(const char* what,size_t sLocationLength,int sError)
		:RuntimeError(what,sLocationLength),
		 error(sError)
		{
		}
	
	/* Methods: */
	int getError(void) const // Returns the C library error
		{
		return error;
		}
	};

/* Namespace-global functions: */

LibcError libcError(const char* prettyFunction,int error,const char* whatFormat,...); // Returns a LibcError object that can be thrown; first parameter is expected to be __PRETTY_FUNCTION__, second parameter is expected to be errno, third parameter is printf-style format string, rest are printf parameters

}

#endif
