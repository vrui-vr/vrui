/***********************************************************************
RGBA - Class to represent colors in the RGB color space with opacity
components.
Copyright (c) 2020-2023 Oliver Kreylos

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

#include <Misc/RGBA.h>
#include <Misc/RGBA.icpp>

#include <Misc/SizedTypes.h>

namespace Misc {

/************************************************
Force instantiation of all standard RGBA classes:
************************************************/

template class RGBA<SInt8>;
template void RGBA<SInt8>::convertAndCopy<3,UInt8>(const UInt8* sComponents);
template void RGBA<SInt8>::convertAndCopy<3,SInt16>(const SInt16* sComponents);
template void RGBA<SInt8>::convertAndCopy<3,UInt16>(const UInt16* sComponents);
template void RGBA<SInt8>::convertAndCopy<3,SInt32>(const SInt32* sComponents);
template void RGBA<SInt8>::convertAndCopy<3,UInt32>(const UInt32* sComponents);
template void RGBA<SInt8>::convertAndCopy<3,Float32>(const Float32* sComponents);
template void RGBA<SInt8>::convertAndCopy<3,Float64>(const Float64* sComponents);
template void RGBA<SInt8>::convertAndCopy<4,UInt8>(const UInt8* sComponents);
template void RGBA<SInt8>::convertAndCopy<4,SInt16>(const SInt16* sComponents);
template void RGBA<SInt8>::convertAndCopy<4,UInt16>(const UInt16* sComponents);
template void RGBA<SInt8>::convertAndCopy<4,SInt32>(const SInt32* sComponents);
template void RGBA<SInt8>::convertAndCopy<4,UInt32>(const UInt32* sComponents);
template void RGBA<SInt8>::convertAndCopy<4,Float32>(const Float32* sComponents);
template void RGBA<SInt8>::convertAndCopy<4,Float64>(const Float64* sComponents);
template RGBA<SInt8> blend(const RGBA<SInt8>& c1,const RGBA<SInt8>& c2,double blend);

template class RGBA<UInt8>;
template void RGBA<UInt8>::convertAndCopy<3,SInt8>(const SInt8* sComponents);
template void RGBA<UInt8>::convertAndCopy<3,SInt16>(const SInt16* sComponents);
template void RGBA<UInt8>::convertAndCopy<3,UInt16>(const UInt16* sComponents);
template void RGBA<UInt8>::convertAndCopy<3,SInt32>(const SInt32* sComponents);
template void RGBA<UInt8>::convertAndCopy<3,UInt32>(const UInt32* sComponents);
template void RGBA<UInt8>::convertAndCopy<3,Float32>(const Float32* sComponents);
template void RGBA<UInt8>::convertAndCopy<3,Float64>(const Float64* sComponents);
template void RGBA<UInt8>::convertAndCopy<4,SInt8>(const SInt8* sComponents);
template void RGBA<UInt8>::convertAndCopy<4,SInt16>(const SInt16* sComponents);
template void RGBA<UInt8>::convertAndCopy<4,UInt16>(const UInt16* sComponents);
template void RGBA<UInt8>::convertAndCopy<4,SInt32>(const SInt32* sComponents);
template void RGBA<UInt8>::convertAndCopy<4,UInt32>(const UInt32* sComponents);
template void RGBA<UInt8>::convertAndCopy<4,Float32>(const Float32* sComponents);
template void RGBA<UInt8>::convertAndCopy<4,Float64>(const Float64* sComponents);
template RGBA<UInt8> blend(const RGBA<UInt8>& c1,const RGBA<UInt8>& c2,double blend);

template class RGBA<SInt16>;
template void RGBA<SInt16>::convertAndCopy<3,SInt8>(const SInt8* sComponents);
template void RGBA<SInt16>::convertAndCopy<3,UInt8>(const UInt8* sComponents);
template void RGBA<SInt16>::convertAndCopy<3,UInt16>(const UInt16* sComponents);
template void RGBA<SInt16>::convertAndCopy<3,SInt32>(const SInt32* sComponents);
template void RGBA<SInt16>::convertAndCopy<3,UInt32>(const UInt32* sComponents);
template void RGBA<SInt16>::convertAndCopy<3,Float32>(const Float32* sComponents);
template void RGBA<SInt16>::convertAndCopy<3,Float64>(const Float64* sComponents);
template void RGBA<SInt16>::convertAndCopy<4,SInt8>(const SInt8* sComponents);
template void RGBA<SInt16>::convertAndCopy<4,UInt8>(const UInt8* sComponents);
template void RGBA<SInt16>::convertAndCopy<4,UInt16>(const UInt16* sComponents);
template void RGBA<SInt16>::convertAndCopy<4,SInt32>(const SInt32* sComponents);
template void RGBA<SInt16>::convertAndCopy<4,UInt32>(const UInt32* sComponents);
template void RGBA<SInt16>::convertAndCopy<4,Float32>(const Float32* sComponents);
template void RGBA<SInt16>::convertAndCopy<4,Float64>(const Float64* sComponents);
template RGBA<SInt16> blend(const RGBA<SInt16>& c1,const RGBA<SInt16>& c2,double blend);

template class RGBA<UInt16>;
template void RGBA<UInt16>::convertAndCopy<3,SInt8>(const SInt8* sComponents);
template void RGBA<UInt16>::convertAndCopy<3,UInt8>(const UInt8* sComponents);
template void RGBA<UInt16>::convertAndCopy<3,SInt16>(const SInt16* sComponents);
template void RGBA<UInt16>::convertAndCopy<3,SInt32>(const SInt32* sComponents);
template void RGBA<UInt16>::convertAndCopy<3,UInt32>(const UInt32* sComponents);
template void RGBA<UInt16>::convertAndCopy<3,Float32>(const Float32* sComponents);
template void RGBA<UInt16>::convertAndCopy<3,Float64>(const Float64* sComponents);
template void RGBA<UInt16>::convertAndCopy<4,SInt8>(const SInt8* sComponents);
template void RGBA<UInt16>::convertAndCopy<4,UInt8>(const UInt8* sComponents);
template void RGBA<UInt16>::convertAndCopy<4,SInt16>(const SInt16* sComponents);
template void RGBA<UInt16>::convertAndCopy<4,SInt32>(const SInt32* sComponents);
template void RGBA<UInt16>::convertAndCopy<4,UInt32>(const UInt32* sComponents);
template void RGBA<UInt16>::convertAndCopy<4,Float32>(const Float32* sComponents);
template void RGBA<UInt16>::convertAndCopy<4,Float64>(const Float64* sComponents);
template RGBA<UInt16> blend(const RGBA<UInt16>& c1,const RGBA<UInt16>& c2,double blend);

template class RGBA<SInt32>;
template void RGBA<SInt32>::convertAndCopy<3,SInt8>(const SInt8* sComponents);
template void RGBA<SInt32>::convertAndCopy<3,UInt8>(const UInt8* sComponents);
template void RGBA<SInt32>::convertAndCopy<3,SInt16>(const SInt16* sComponents);
template void RGBA<SInt32>::convertAndCopy<3,UInt16>(const UInt16* sComponents);
template void RGBA<SInt32>::convertAndCopy<3,UInt32>(const UInt32* sComponents);
template void RGBA<SInt32>::convertAndCopy<3,Float32>(const Float32* sComponents);
template void RGBA<SInt32>::convertAndCopy<3,Float64>(const Float64* sComponents);
template void RGBA<SInt32>::convertAndCopy<4,SInt8>(const SInt8* sComponents);
template void RGBA<SInt32>::convertAndCopy<4,UInt8>(const UInt8* sComponents);
template void RGBA<SInt32>::convertAndCopy<4,SInt16>(const SInt16* sComponents);
template void RGBA<SInt32>::convertAndCopy<4,UInt16>(const UInt16* sComponents);
template void RGBA<SInt32>::convertAndCopy<4,UInt32>(const UInt32* sComponents);
template void RGBA<SInt32>::convertAndCopy<4,Float32>(const Float32* sComponents);
template void RGBA<SInt32>::convertAndCopy<4,Float64>(const Float64* sComponents);
template RGBA<SInt32> blend(const RGBA<SInt32>& c1,const RGBA<SInt32>& c2,double blend);

template class RGBA<UInt32>;
template void RGBA<UInt32>::convertAndCopy<3,SInt8>(const SInt8* sComponents);
template void RGBA<UInt32>::convertAndCopy<3,UInt8>(const UInt8* sComponents);
template void RGBA<UInt32>::convertAndCopy<3,SInt16>(const SInt16* sComponents);
template void RGBA<UInt32>::convertAndCopy<3,UInt16>(const UInt16* sComponents);
template void RGBA<UInt32>::convertAndCopy<3,SInt32>(const SInt32* sComponents);
template void RGBA<UInt32>::convertAndCopy<3,Float32>(const Float32* sComponents);
template void RGBA<UInt32>::convertAndCopy<3,Float64>(const Float64* sComponents);
template void RGBA<UInt32>::convertAndCopy<4,SInt8>(const SInt8* sComponents);
template void RGBA<UInt32>::convertAndCopy<4,UInt8>(const UInt8* sComponents);
template void RGBA<UInt32>::convertAndCopy<4,SInt16>(const SInt16* sComponents);
template void RGBA<UInt32>::convertAndCopy<4,UInt16>(const UInt16* sComponents);
template void RGBA<UInt32>::convertAndCopy<4,SInt32>(const SInt32* sComponents);
template void RGBA<UInt32>::convertAndCopy<4,Float32>(const Float32* sComponents);
template void RGBA<UInt32>::convertAndCopy<4,Float64>(const Float64* sComponents);
template RGBA<UInt32> blend(const RGBA<UInt32>& c1,const RGBA<UInt32>& c2,double blend);

template class RGBA<Float32>;
template void RGBA<Float32>::convertAndCopy<3,SInt8>(const SInt8* sComponents);
template void RGBA<Float32>::convertAndCopy<3,UInt8>(const UInt8* sComponents);
template void RGBA<Float32>::convertAndCopy<3,SInt16>(const SInt16* sComponents);
template void RGBA<Float32>::convertAndCopy<3,UInt16>(const UInt16* sComponents);
template void RGBA<Float32>::convertAndCopy<3,SInt32>(const SInt32* sComponents);
template void RGBA<Float32>::convertAndCopy<3,UInt32>(const UInt32* sComponents);
template void RGBA<Float32>::convertAndCopy<3,Float64>(const Float64* sComponents);
template void RGBA<Float32>::convertAndCopy<4,SInt8>(const SInt8* sComponents);
template void RGBA<Float32>::convertAndCopy<4,UInt8>(const UInt8* sComponents);
template void RGBA<Float32>::convertAndCopy<4,SInt16>(const SInt16* sComponents);
template void RGBA<Float32>::convertAndCopy<4,UInt16>(const UInt16* sComponents);
template void RGBA<Float32>::convertAndCopy<4,SInt32>(const SInt32* sComponents);
template void RGBA<Float32>::convertAndCopy<4,UInt32>(const UInt32* sComponents);
template void RGBA<Float32>::convertAndCopy<4,Float64>(const Float64* sComponents);
template RGBA<Float32> blend(const RGBA<Float32>& c1,const RGBA<Float32>& c2,double blend);

template class RGBA<Float64>;
template void RGBA<Float64>::convertAndCopy<3,SInt8>(const SInt8* sComponents);
template void RGBA<Float64>::convertAndCopy<3,UInt8>(const UInt8* sComponents);
template void RGBA<Float64>::convertAndCopy<3,SInt16>(const SInt16* sComponents);
template void RGBA<Float64>::convertAndCopy<3,UInt16>(const UInt16* sComponents);
template void RGBA<Float64>::convertAndCopy<3,SInt32>(const SInt32* sComponents);
template void RGBA<Float64>::convertAndCopy<3,UInt32>(const UInt32* sComponents);
template void RGBA<Float64>::convertAndCopy<3,Float32>(const Float32* sComponents);
template void RGBA<Float64>::convertAndCopy<4,SInt8>(const SInt8* sComponents);
template void RGBA<Float64>::convertAndCopy<4,UInt8>(const UInt8* sComponents);
template void RGBA<Float64>::convertAndCopy<4,SInt16>(const SInt16* sComponents);
template void RGBA<Float64>::convertAndCopy<4,UInt16>(const UInt16* sComponents);
template void RGBA<Float64>::convertAndCopy<4,SInt32>(const SInt32* sComponents);
template void RGBA<Float64>::convertAndCopy<4,UInt32>(const UInt32* sComponents);
template void RGBA<Float64>::convertAndCopy<4,Float32>(const Float32* sComponents);
template RGBA<Float64> blend(const RGBA<Float64>& c1,const RGBA<Float64>& c2,double blend);

}
