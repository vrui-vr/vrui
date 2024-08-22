/***********************************************************************
BlendColorComponent - Namespace-global template function to blend two
color components of the same scalar type.
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

#ifndef MISC_BLENDCOLORCOMPONENT_INCLUDED
#define MISC_BLENDCOLORCOMPONENT_INCLUDED

#include <Misc/SizedTypes.h>

namespace Misc {

/***********************************************************************
Genetic templatized color scalar blending function for integer scalar
types:
***********************************************************************/

template <class ScalarParam>
ScalarParam blendColorComponent(ScalarParam c1,ScalarParam c2,double blend)
	{
	return ScalarParam(double(c1)*(1.0-blend)+double(c2)*blend+0.5);
	}

/***********************************************************************
Specialized color scalar blending functions for floating-point scalar
types:
***********************************************************************/

template <>
inline Float32 blendColorComponent(Float32 c1,Float32 c2,double blend)
	{
	return Float32(double(c1)*(1.0-blend)+double(c2)*blend);
	}

template <>
inline Float64 blendColorComponent(Float64 c1,Float64 c2,double blend)
	{
	return Float64(double(c1)*(1.0-blend)+double(c2)*blend);
	}

}

#endif
