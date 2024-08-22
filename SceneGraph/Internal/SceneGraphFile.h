/***********************************************************************
SceneGraphFile - Definition of the header and version of binary scene
graph files stored on disks or transmitted over networks.
Copyright (c) 2021 Oliver Kreylos

This file is part of the Simple Scene Graph Renderer (SceneGraph).

The Simple Scene Graph Renderer is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Simple Scene Graph Renderer is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Simple Scene Graph Renderer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef SCENEGRAPH_SCENEGRAPHFILE_INCLUDED
#define SCENEGRAPH_SCENEGRAPHFILE_INCLUDED

#include <stddef.h>

namespace SceneGraph {

struct SceneGraphFile
	{
	/* Elements: */
	public:
	static const size_t headerSize=24; // Length of identifying header string in bytes
	static const char headerString[headerSize]; // Identifying header string
	static const unsigned int majorVersion; // Major file format version
	static const unsigned int minorVersion; // Minor file format version
	};

}

#endif
