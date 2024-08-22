/***********************************************************************
HMDConfigurationUpdater - Class to connect a rendering window for HMDs
to HMD configuration updates.
Copyright (c) 2024 Oliver Kreylos

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

#ifndef VRUI_HMDCONFIGURATIONUPDATER_INCLUDED
#define VRUI_HMDCONFIGURATIONUPDATER_INCLUDED

#include <Misc/Autopointer.h>
#include <Vrui/Types.h>

/* Forward declarations: */
namespace Threads {
template <class ParameterParam>
class FunctionCall;
}
namespace GLMotif {
class PopupWindow;
}
namespace Vrui {
class Viewer;
class HMDConfiguration;
class InputDeviceAdapterDeviceDaemon;
}

namespace Vrui {

class HMDConfigurationUpdater
	{
	/* Embedded classes: */
	public:
	typedef Threads::FunctionCall<const HMDConfiguration&> ConfigurationChangedCallback; // Type of callbacks called from frame sequence when the HMD configuration changes
	
	/* Elements: */
	private:
	Viewer* hmdViewer; // Viewer representing the HMD
	InputDeviceAdapterDeviceDaemon* hmdAdapter; // Input device adapter providing HMD configuration data
	int hmdTrackerIndex; // Tracker index associated with the HMD
	const HMDConfiguration* hmdConfiguration; // Pointer to the HMD configuration providing lens correction parameters
	Misc::Autopointer<ConfigurationChangedCallback> configurationChangedCallback; // Callback function called from frame sequence when HMD configuration changed
	double ipdDisplayDialogTimeout; // Time for which the IPD display dialog stays open in seconds
	unsigned int eyePosVersion; // Version number to keep track of eye position changes
	GLMotif::PopupWindow* ipdDisplayDialog; // A dialog window to notify the user of changed HMD configuration
	Scalar lastShownIpd; // Inter-pupillary distance currently shown in the IPD display dialog
	double ipdDisplayDialogTakedownTime; // Time at which the dialog window will be closed
	
	/* Private methods: */
	static bool hmdConfigurationUpdatedFrame(void* userData); // Callback called in frame sequence when HMD configuration changed
	void hmdConfigurationUpdated(const HMDConfiguration& hmdConfiguration); // Callback called from background thread when HMD configuration changes
	
	/* Constructors and destructors: */
	public:
	HMDConfigurationUpdater(Viewer* sHmdViewer,ConfigurationChangedCallback& sConfigurationChangedCallback); // Creates an HMD configuration updater for the HMD connected to the given viewer
	~HMDConfigurationUpdater(void);
	
	/* Methods: */
	};

}

#endif
