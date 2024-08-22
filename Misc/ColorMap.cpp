/***********************************************************************
ColorMap - Templatized class to map scalar values within a defined range
to colors.
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

#include <Misc/ColorMap.h>
#include <Misc/ColorMap.icpp>

#include <Misc/SizedTypes.h>
#include <Misc/RGB.h>
#include <Misc/RGBA.h>

namespace Misc {

/****************************************************
Force instantiation of all standard ColorMap classes:
****************************************************/

template class ColorMap<RGB<SInt8> >;
template class ColorMap<RGB<UInt8> >;
template class ColorMap<RGB<SInt16> >;
template class ColorMap<RGB<UInt16> >;
template class ColorMap<RGB<SInt32> >;
template class ColorMap<RGB<UInt32> >;
template class ColorMap<RGB<Float32> >;
template class ColorMap<RGB<Float64> >;

template class ColorMap<RGBA<SInt8> >;
template class ColorMap<RGBA<UInt8> >;
template class ColorMap<RGBA<SInt16> >;
template class ColorMap<RGBA<UInt16> >;
template class ColorMap<RGBA<SInt32> >;
template class ColorMap<RGBA<UInt32> >;
template class ColorMap<RGBA<Float32> >;
template class ColorMap<RGBA<Float64> >;

}
