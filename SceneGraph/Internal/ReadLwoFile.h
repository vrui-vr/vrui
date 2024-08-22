/***********************************************************************
ReadLwoFile - Helper function to read a 3D polygon file in Lightwave
Object format into a list of shape nodes.
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

#ifndef SCENEGRAPH_INTERNAL_READLWOFILE_INCLUDED
#define SCENEGRAPH_INTERNAL_READLWOFILE_INCLUDED

#include <string>

/* Forward declarations: */
namespace IO {
class Directory;
}
namespace SceneGraph {
class MeshFileNode;
}

namespace SceneGraph {

void readLwoFile(const IO::Directory& directory,const std::string& fileName,MeshFileNode& node); // Reads the Lightwave Object file of the given name from the given directory and appends read shape nodes to the given mesh file node's representation

}

#endif
