/***********************************************************************
OutputOperators - Basic iostream output operators for Misc objects.
Copyright (c) 2022 Oliver Kreylos

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

#ifndef MISC_OUTPUTOPERATORS_INCLUDED
#define MISC_OUTPUTOPERATORS_INCLUDED

#include <iostream>
#include <iomanip>
#include <Misc/IntVector.h>
#include <Misc/Rect.h>

namespace std{

template <class ComponentParam,unsigned int numComponentsParam>
inline
ostream&
operator<<(
	ostream& os,
	const Misc::IntVector<ComponentParam,numComponentsParam>& value)
	{
	streamsize width=os.width();
	os<<setw(1)<<'('<<setw(width)<<value[0];
	for(unsigned int i=1;i<numComponentsParam;++i)
		os<<", "<<setw(width)<<value[i];
	os<<')';
	
	return os;
	}

template <unsigned int numComponentsParam>
inline
ostream&
operator<<(
	ostream& os,
	const Misc::Rect<numComponentsParam>& value)
	{
	streamsize width=os.width();
	os<<setw(width)<<value.offset<<", "<<setw(width)<<value.size;
	
	return os;
	}

}

#endif
