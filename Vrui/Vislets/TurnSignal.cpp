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

#include <Vrui/Vislets/TurnSignal.h>

#include <iostream>
#include <iomanip>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthonormalTransformation.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GL/GLValueCoders.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>
#include <Vrui/Viewer.h>
#include <Vrui/VisletManager.h>

namespace Vrui {

namespace Vislets {

/**********************************
Methods of class TurnSignalFactory:
**********************************/

TurnSignalFactory::TurnSignalFactory(VisletManager& visletManager)
	:VisletFactory("TurnSignal",visletManager),
	 arrowSize(getInchFactor()),
	 arrowDist(arrowSize*Scalar(9)),arrowHeight(arrowSize*Scalar(2)),
	 arrowColor(0.0f,1.0f,0.0f)
	{
	#if 0
	/* Insert class into class hierarchy: */
	VisletFactory* visletFactory=visletManager.loadClass("Vislet");
	visletFactory->addChildClass(this);
	addParentClass(visletFactory);
	#endif
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=visletManager.getVisletClassSection(getClassName());
	cfs.updateValue("./arrowSize",arrowSize);
	cfs.updateValue("./arrowDist",arrowDist);
	cfs.updateValue("./arrowHeight",arrowHeight);
	cfs.updateValue("./arrowColor",arrowColor);
	
	/* Set tool class' factory pointer: */
	TurnSignal::factory=this;
	}

TurnSignalFactory::~TurnSignalFactory(void)
	{
	/* Reset tool class' factory pointer: */
	TurnSignal::factory=0;
	}

Vislet* TurnSignalFactory::createVislet(int numArguments,const char* const arguments[]) const
	{
	return new TurnSignal(numArguments,arguments);
	}

void TurnSignalFactory::destroyVislet(Vislet* vislet) const
	{
	delete vislet;
	}

extern "C" void resolveTurnSignalDependencies(Plugins::FactoryManager<VisletFactory>& manager)
	{
	#if 0
	/* Load base classes: */
	manager.loadClass("Vislet");
	#endif
	}

extern "C" VisletFactory* createTurnSignalFactory(Plugins::FactoryManager<VisletFactory>& manager)
	{
	/* Get pointer to vislet manager: */
	VisletManager* visletManager=static_cast<VisletManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	TurnSignalFactory* factory=new TurnSignalFactory(*visletManager);
	
	/* Return factory object: */
	return factory;
	}

extern "C" void destroyTurnSignalFactory(VisletFactory* factory)
	{
	delete factory;
	}

/***********************************
Static elements of class TurnSignal:
***********************************/

TurnSignalFactory* TurnSignal::factory=0;

/***************************
Methods of class TurnSignal:
***************************/

TurnSignal::TurnSignal(int numArguments,const char* const arguments[])
	:baseAngle(0),angle(0),turn(0),turnAngle(0)
	{
	}

TurnSignal::~TurnSignal(void)
	{
	}

VisletFactory* TurnSignal::getFactory(void) const
	{
	return factory;
	}

Scalar angleMin(360);
Scalar angleMax(-360);

void TurnSignal::enable(bool startup)
	{
	/* Call the base class method: */
	Vislet::enable(startup);
	
	if(startup)
		{
		/* Initialize the base rotation: */
		baseRot=getMainViewer()->getHeadTransformation().getRotation();
		}
	}

void TurnSignal::disable(bool shutdown)
	{
	/* Call the base class method: */
	Vislet::disable(shutdown);
	
	if(shutdown)
		{
		// DEBUGGING
		std::cout<<std::endl;
		std::cout<<"Rotation range: "<<std::setiosflags(std::ios::fixed)<<std::setprecision(1)<<angleMin<<" - "<<angleMax<<std::endl;
		
		/* Print the final rotation angle: */
		std::cout<<"Vrui::TurnSignal: Final rotation angle is "<<std::setiosflags(std::ios::fixed)<<std::setprecision(1)<<angle<<std::endl;
		}
	}

void TurnSignal::frame(void)
	{
	/* Get the main viewer's current rotation: */
	Rotation rot=getMainViewer()->getHeadTransformation().getRotation();
	
	/* Calculate the incremental rotation vector: */
	Vector rotVec=(rot/baseRot).getScaledAxis();
	const Vector& up=Vrui::getUpDirection();
	Scalar relAngle=Math::deg(rotVec*up);
	
	/* Update the base rotation if the relative angle is too large: */
	if(relAngle>=Scalar(90))
		{
		/* Rotate the base rotation to the left: */
		relAngle-=Scalar(90);
		baseRot.leftMultiply(Rotation(up*Math::rad(Scalar(90))));
		baseAngle+=Scalar(90);
		}
	else if(relAngle<=Scalar(-90))
		{
		/* Rotate the base rotation to the right: */
		relAngle+=Scalar(90);
		baseRot.leftMultiply(Rotation(up*Math::rad(Scalar(-90))));
		baseAngle-=Scalar(90);
		}
	
	/* Calculate the current total angle: */
	angle=baseAngle+relAngle;
	
	// DEBUGGING
	if(angleMin>angle)
		angleMin=angle;
	if(angleMax<angle)
		angleMax=angle;
	if(isActive())
		{
		// DEBUGGING
		std::cout<<"Total rotation: "<<std::setiosflags(std::ios::fixed)<<std::setprecision(1)<<std::setw(8)<<angle<<"  \r"<<std::flush;
		}
	
	/* Manage turns: */
	if(turn>0)
		{
		/* Update the turn angle if user refuses to comply: */
		if(turnAngle>angle)
			turnAngle=angle;
		
		/* Calculate the number of turns to complete: */
		Scalar endAngle=Math::mod(turnAngle,Scalar(360));
		turn=Math::max(int(Math::ceil((endAngle-angle)/Scalar(360))),0);
		}
	else if(turn<0)
		{
		/* Update the turn angle if user refuses to comply: */
		if(turnAngle<angle)
			turnAngle=angle;
		
		/* Calculate the number of turns to complete: */
		Scalar endAngle=Math::mod(turnAngle,Scalar(360));
		turn=-Math::max(int(Math::ceil((angle-endAngle)/Scalar(360))),0);
		}
	else if(angle<=Scalar(-360))
		{
		/* Start a left turn: */
		turnAngle=angle;
		Scalar endAngle=Math::mod(turnAngle,Scalar(360));
		turn=Math::max(int(Math::ceil((endAngle-angle)/Scalar(360))),0);
		}
	else if(angle>=Scalar(360))
		{
		/* Start a right turn: */
		turnAngle=angle;
		Scalar endAngle=Math::mod(turnAngle,Scalar(360));
		turn=-Math::max(int(Math::ceil((angle-endAngle)/Scalar(360))),0);
		}
	}

void TurnSignal::display(GLContextData& contextData) const
	{
	/* Check if a turn is currently indicated: */
	int numArrows=Math::abs(turn);
	if(numArrows!=0&&isActive())
		{
		/* Set up OpenGL state: */
		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_LIGHTING);
		
		/* Set up a coordinate frame in front of the viewer: */
		Point viewerPos=getMainViewer()->getHeadPosition();
		Vector viewDir=getMainViewer()->getViewDirection();
		glPushMatrix();
		glTranslate(viewerPos-Point::origin);
		glRotate(Rotation::fromBaseVectors(viewDir^getUpDirection(),getUpDirection()));
		glTranslate(Scalar(0),factory->arrowHeight,-factory->arrowDist);
		glScale(factory->arrowSize,factory->arrowSize,factory->arrowSize);
		
		/* Draw the turn indicator arrow(s): */
		if(turn<0)
			{
			/* Indicate a right turn: */
			glTranslatef((numArrows-1)*-3,0,0);
			for(int arrow=0;arrow<numArrows;++arrow)
				{
				glBegin(GL_TRIANGLE_FAN);
				glColor(factory->arrowColor);
				glVertex(3,0);
				glVertex(1,2);
				glVertex(1,1);
				glVertex(-2,1);
				glVertex(-2,-1);
				glVertex(1,-1);
				glVertex(1,-2);
				glEnd();
				
				glTranslatef(6,0,0);
				}
			}
		else
			{
			/* Indicate a left turn: */
			glTranslatef((numArrows-1)*-3,0,0);
			for(int arrow=0;arrow<numArrows;++arrow)
				{
				glBegin(GL_TRIANGLE_FAN);
				glColor(factory->arrowColor);
				glVertex(-3,0);
				glVertex(-1,-2);
				glVertex(-1,-1);
				glVertex(2,-1);
				glVertex(2,1);
				glVertex(-1,1);
				glVertex(-1,2);
				glEnd();
				
				glTranslatef(6,0,0);
				}
			}
		
		/* Return to physical coordinates: */
		glPopMatrix();
		
		/* Restore OpenGL state: */
		glPopAttrib();
		}
	}

}

}
