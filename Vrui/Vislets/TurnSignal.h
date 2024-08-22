/***********************************************************************
TurnSignal - Vislet class to keep track of the main viewer's total
horizontal rotation and remind users to turn the other way once in a
goddamn while. It's like they're a bunch of Zoolanders, for crying out
loud.
Copyright (c) 2021-2023 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef VRUI_VISLETS_TURNSIGNAL_INCLUDED
#define VRUI_VISLETS_TURNSIGNAL_INCLUDED

#include <Geometry/Rotation.h>
#include <GL/GLColor.h>
#include <Vrui/Types.h>
#include <Vrui/Vislet.h>

namespace Vrui {

namespace Vislets {

class TurnSignal;

class TurnSignalFactory:public VisletFactory
	{
	friend class TurnSignal;
	
	/* Elements: */
	private:
	Scalar arrowSize; // Base arrow size in physical coordinate units
	Scalar arrowDist; // Horizontal distance from viewer to arrow in physical coordinate units
	Scalar arrowHeight; // Height of arrow above viewer in physical coordinate units
	Color arrowColor; // Color to draw turn indicator arrows
	
	/* Constructors and destructors: */
	public:
	TurnSignalFactory(VisletManager& visletManager);
	virtual ~TurnSignalFactory(void);
	
	/* Methods: */
	virtual Vislet* createVislet(int numVisletArguments,const char* const visletArguments[]) const;
	virtual void destroyVislet(Vislet* vislet) const;
	};

class TurnSignal:public Vislet
	{
	friend class TurnSignalFactory;
	
	/* Elements: */
	private:
	static TurnSignalFactory* factory; // Pointer to the factory object for this class
	Rotation baseRot; // Current base rotation of the main viewer
	Scalar baseAngle; // Horizontal rotation angle associated with the current base rotation in degrees
	Scalar angle; // Current horizontal rotation angle in degrees
	int turn; // Number of 360 degree left (turn>0) or right (turn<0) turns required
	Scalar turnAngle; // Angle from which the turn was started; turn ends when at (angle mod 360)
	
	/* Constructors and destructors: */
	public:
	TurnSignal(int numArguments,const char* const arguments[]);
	virtual ~TurnSignal(void);
	
	/* Methods from class Vislet: */
	public:
	virtual VisletFactory* getFactory(void) const;
	virtual void enable(bool startup);
	virtual void disable(bool shutdown);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	};

}

}

#endif
