/***********************************************************************
ToolKillZone - Base class for "kill zones" for tools and input devices.
Copyright (c) 2004-2023 Oliver Kreylos

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

#ifndef VRUI_INTERNAL_TOOLKILLZONE_INCLUDED
#define VRUI_INTERNAL_TOOLKILLZONE_INCLUDED

#include <GL/gl.h>
#include <GL/GLMaterial.h>
#include <GL/GLObject.h>
#include <Vrui/Types.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}
namespace Vrui {
class InputDevice;
}

namespace Vrui {

class ToolKillZone:public GLObject
	{
	/* Embedded classes: */
	private:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint modelDisplayListId; // Display list ID to render kill zone model
		unsigned int modelVersion; // Version number of the model currently in the display list
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	protected:
	InputDevice* baseDevice; // Pointer to input device kill zone is attached to
	bool active; // Flag if the kill zone is active, i.e., reacts to input devices entering
	bool render; // Flag if the kill zone is supposed to be rendered
	GLMaterial material; // Material to render the kill zone model
	unsigned int modelVersion; // Version number of the current model
	
	/* Protected methods: */
	virtual void renderModel(void) const =0; // Renders the kill zone's graphical representation
	void updateModel(void); // Notifies the base class that the model geometry has changed
	
	/* Constructors and destructors: */
	public:
	ToolKillZone(const Misc::ConfigurationFileSection& configFileSection); // Initializes kill zone by reading from given configuration file section
	virtual ~ToolKillZone(void);
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	
	/* New methods: */
	bool isActive(void) const // Returns the tool kill zone's active flag
		{
		return active;
		}
	bool getRender(void) const // Returns the tool kill zone's render flag
		{
		return render;
		}
	void setActive(bool newActive); // Sets the tool kill zone's active flag
	void setRender(bool newRender); // Sets the tool kill zone's render flag
	virtual Size getSize(void) const =0; // Returns axis-aligned sizes of the kill zone in physical coordinate units
	virtual Point getCenter(void) const =0; // Returns the current position of the kill zone in physical space
	virtual void setCenter(const Point& newCenter) =0; // Moves the kill zone to a new position in physical space
	virtual bool isDeviceIn(const InputDevice* device) const =0; // Returns true if the given device is in (or pointing at) the kill zone
	void glRenderAction(GLContextData& contextData) const; // Renders the kill zone
	};

}

#endif
