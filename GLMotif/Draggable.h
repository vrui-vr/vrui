/***********************************************************************
Draggable - Mix-in class for widgets that are used to change the widget
transformation of their containing top-level widget.
Copyright (c) 2010-2021 Oliver Kreylos

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

#ifndef GLMOTIF_DRAGGABLE_INCLUDED
#define GLMOTIF_DRAGGABLE_INCLUDED

/* Forward declarations: */
namespace GLMotif {
class Event;
}

namespace GLMotif {

class Draggable
	{
	/* Constructors and destructors: */
	public:
	virtual ~Draggable(void);
	
	/* Methods: */
	virtual bool canDrag(const Event& event) const; // Returns true if the widget can be dragged in response to the given event
	};

}

#endif
