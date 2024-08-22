/***********************************************************************
SceneGraphViewerTransformTool - Transform tool to place a virtual input
device at the intersection of a ray with a scene graph's geometry.
Copyright (c) 2023 Oliver Kreylos

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef SCENEGRAPHVIEWERTRANSFORMTOOL_INCLUDED
#define SCENEGRAPHVIEWERTRANSFORMTOOL_INCLUDED

#include <Vrui/Types.h>
#include <Vrui/TransformTool.h>

#include "SceneGraphViewer.h"

/****************
Embedded classes:
****************/

class SceneGraphViewer::TransformToolFactory:public Vrui::ToolFactory
	{
	friend class TransformTool;
	
	/* Embedded classes: */
	private:
	typedef Vrui::Scalar Scalar;
	
	/* Constructors and destructors: */
	public:
	TransformToolFactory(Vrui::ToolManager& toolManager);
	virtual ~TransformToolFactory(void);
	
	/* Methods from Vrui::ToolFactory: */
	virtual const char* getName(void) const;
	virtual const char* getButtonFunction(int buttonSlotIndex) const;
	virtual const char* getValuatorFunction(int valuatorSlotIndex) const;
	virtual Vrui::Tool* createTool(const Vrui::ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Vrui::Tool* tool) const;
	};

class SceneGraphViewer::TransformTool:public Vrui::TransformTool
	{
	friend class TransformToolFactory;
	
	/* Embedded classes: */
	private:
	typedef Vrui::Scalar Scalar;
	typedef Vrui::Point Point;
	typedef Vrui::Vector Vector;
	
	/* Elements: */
	private:
	static TransformToolFactory* factory; // Pointer to the factory object for this class
	
	/* Constructors and destructors: */
	public:
	TransformTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment);
	
	/* Methods from Tool: */
	virtual void initialize(void);
	virtual const Vrui::ToolFactory* getFactory(void) const;
	virtual void frame(void);
	};

#endif
