/***********************************************************************
PointSetNode - Class for sets of points as renderable geometry.
Copyright (c) 2009-2023 Oliver Kreylos

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

#ifndef SCENEGRAPH_POINTSETNODE_INCLUDED
#define SCENEGRAPH_POINTSETNODE_INCLUDED

#include <GL/gl.h>
#include <GL/GLObject.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/GeometryNode.h>
#include <SceneGraph/ColorNode.h>
#include <SceneGraph/CoordinateNode.h>

/* Forward declarations: */
class GLSphereRenderer;

namespace SceneGraph {

class PointSetNode:public GeometryNode,public GLObject
	{
	/* Embedded classes: */
	public:
	typedef SF<ColorNodePointer> SFColorNode;
	typedef SF<CoordinateNodePointer> SFCoordinateNode;
	
	protected:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint vertexBufferObjectId; // ID of vertex buffer object containing the point set, if supported
		unsigned int version; // Version of point set stored in vertex buffer object
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	public:
	static const char* className; // The class's name
	
	/* Fields: */
	SFColorNode color;
	SFCoordinateNode coord;
	SFBool drawSpheres;
	SFFloat pointSize;
	
	/* Derived state: */
	protected:
	GLSphereRenderer* sphereRenderer; // A helper object to render points as spheres
	unsigned int version; // Version number of point set
	
	/* Constructors and destructors: */
	public:
	PointSetNode(void); // Creates a default point set (no color or coord node, point size 1.0)
	virtual ~PointSetNode(void);
	
	/* Methods from class Node: */
	virtual const char* getClassName(void) const;
	virtual void parseField(const char* fieldName,VRMLFile& vrmlFile);
	virtual void update(void);
	virtual void read(SceneGraphReader& reader);
	virtual void write(SceneGraphWriter& writer) const;
	
	/* Methods from class GeometryNode: */
	virtual bool canCollide(void) const;
	virtual int getGeometryRequirementMask(void) const;
	virtual Box calcBoundingBox(void) const;
	virtual void testCollision(SphereCollisionQuery& collisionQuery) const;
	virtual void glRenderAction(int appearanceRequirementsMask,GLRenderState& renderState) const;
	
	/* Methods from class GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	};

}

#endif
