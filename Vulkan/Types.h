/***********************************************************************
Types - Declarations of common types used throughout the Vulkan library.
Copyright (c) 2022-2023 Oliver Kreylos

This file is part of the Vulkan C++ Wrapper Library (Vulkan).

The Vulkan C++ Wrapper Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Vulkan C++ Wrapper Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vulkan C++ Wrapper Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef VULKAN_TYPES_INCLUDED
#define VULKAN_TYPES_INCLUDED

#include <Misc/Offset.h>
#include <Misc/Size.h>
#include <Misc/Rect.h>

namespace Vulkan {

typedef Misc::Offset<2> Offset; // Type for 2D offsets
typedef Misc::Size<2> Size; // Type for 2D sizes
typedef Misc::Rect<2> Rect; // Type for 2D rectangles

}

#endif
