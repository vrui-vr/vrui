/***********************************************************************
ColorSwatch - Class for widgets displaying an RGB color without
lighting, for color selection.
Copyright (c) 2021-2022 Oliver Kreylos

This file is part of the GLMotif Widget Library (GLMotif).

The GLMotif Widget Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GLMotif Widget Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the GLMotif Widget Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <GLMotif/ColorSwatch.h>

#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GLMotif/Container.h>

namespace GLMotif {

/****************************
Methods of class ColorSwatch:
****************************/

ColorSwatch::ColorSwatch(const char* sName,Container* sParent,bool sManageChild)
	:Widget(sName,sParent,false),preferredSize(0.0f,0.0f,0.0f)
	{
	/* Manage me: */
	if(sManageChild)
		manageChild();
	}

Vector ColorSwatch::calcNaturalSize(void) const
	{
	return calcExteriorSize(preferredSize);
	}

void ColorSwatch::draw(GLContextData& contextData) const
	{
	/* Draw the parent class widget: */
	Widget::draw(contextData);
	
	/* Temporarily disable lighting: */
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	
	/* Fill the interior rectangle with the foreground color: */
	glColor(foregroundColor);
	glBegin(GL_QUADS);
	glNormal3f(0.0f,0.0f,1.0f);
	glVertex(getInterior().getCorner(0));
	glVertex(getInterior().getCorner(1));
	glVertex(getInterior().getCorner(3));
	glVertex(getInterior().getCorner(2));
	glEnd();
	
	/* Restore OpenGL state: */
	glPopAttrib();
	}

void ColorSwatch::setPreferredSize(const Vector& newPreferredSize)
	{
	/* Set the new preferred size: */
	preferredSize=newPreferredSize;
	
	if(isManaged)
		{
		/* Try adjusting the widget size to accommodate the new preferred size: */
		parent->requestResize(this,calcNaturalSize());
		}
	else
		resize(Box(Vector(0.0f,0.0f,0.0f),calcNaturalSize()));
	}

}
