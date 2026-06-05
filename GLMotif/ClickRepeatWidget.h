/***********************************************************************
ClickRepeatWidget - Mix-in class for GLMotif UI components reacting to
timer-based click repeat events while in "button down" state.
Copyright (c) 2026 Oliver Kreylos

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

#ifndef GLMOTIF_CLICKREPEATWIDGET_INCLUDED
#define GLMOTIF_CLICKREPEATWIDGET_INCLUDED

/* Forward declarations: */
namespace GLMotif {
class Event;
class Widget;
}

namespace GLMotif {

class ClickRepeatWidget
	{
	/* New methods: */
	public:
	virtual bool wantClickRepeat(void) =0; // Returns true if the widget wants to receive click repeat events after a pointerButtonDown event
	virtual void clickRepeat(void) =0; // Sends a repeating click event to the widget
	};

}

#endif
