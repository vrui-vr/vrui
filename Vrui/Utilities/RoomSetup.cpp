/***********************************************************************
RoomSetup - Vrui application to calculate basic layout parameters of a
tracked VR environment.
Copyright (c) 2016-2024 Oliver Kreylos

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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utility>
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <Misc/SelfDestructPointer.h>
#include <Misc/StdError.h>
#include <Misc/FileTests.h>
#include <Misc/FunctionCalls.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/MessageLogger.h>
#include <Threads/EventDispatcherThread.h>
#include <Threads/TripleBuffer.h>
#include <Math/Math.h>
#include <Math/Interval.h>
#include <Math/Matrix.h>
#include <Geometry/Point.h>
#include <Geometry/AffineCombiner.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/ProjectiveTransformation.h>
#include <Geometry/Plane.h>
#include <Geometry/ValuedPoint.h>
#include <Geometry/PCACalculator.h>
#include <Geometry/OutputOperators.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLColor.h>
#include <GL/GLContextData.h>
#include <GL/GLObject.h>
#include <GL/Extensions/GLEXTFramebufferObject.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <GL/GLModels.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/Pager.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Margin.h>
#include <GLMotif/Label.h>
#include <GLMotif/Button.h>
#include <GLMotif/ToggleButton.h>
#include <GLMotif/TextField.h>
#include <GLMotif/DropdownBox.h>
#include <Vrui/Vrui.h>
#include <Vrui/CoordinateManager.h>
#include <Vrui/TransparentObject.h>
#include <Vrui/Application.h>
#include <Vrui/EnvironmentDefinition.h>
#include <Vrui/Internal/Config.h>
#include <Vrui/Internal/VRDeviceState.h>
#include <Vrui/Internal/VRDeviceDescriptor.h>
#include <Vrui/Internal/VRBaseStation.h>
#include <Vrui/Internal/VRDeviceClient.h>

class RoomSetup:public Vrui::Application,public Vrui::TransparentObject,public GLObject
	{
	/* Embedded classes: */
	private:
	typedef Vrui::VRDeviceState::TrackerState TS;
	typedef TS::PositionOrientation PO;
	typedef PO::Scalar Scalar;
	typedef PO::Point Point;
	typedef PO::Vector Vector;
	typedef PO::Rotation Rotation;
	typedef TS::LinearVelocity LV;
	typedef TS::AngularVelocity AV;
	typedef std::vector<Point> PointList;
	typedef Vrui::EnvironmentDefinition::Polygon Polygon;
	typedef Vrui::EnvironmentDefinition::PolygonList PolygonList;
	typedef Geometry::ValuedPoint<Point,unsigned int> CalibrationPoint;
	typedef std::vector<CalibrationPoint> CalibrationPointList;
	typedef Geometry::OrthonormalTransformation<double,3> ONTransform;
	typedef Geometry::ProjectiveTransformation<double,2> Homography;
	
	enum Modes // Enumerated type for setup modes
		{
		Controller, // Controller calibration
		Floor, // Floor calibration
		Forward, // Forward direction
		Boundary, // Environment boundary polygon (screen protector) setup
		Surfaces, // Horizontal surfaces to place controllers etc.
		ControlScreen // Calibrated secondary display screen
		};
	
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint floorTextureId; // ID of floor texture object
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	Threads::EventDispatcherThread dispatcher; // Event dispatcher to handle VRDeviceDaemon communication
	Vrui::VRDeviceClient* deviceClient; // Connection to the VRDeviceDaemon
	std::vector<const Vrui::VRDeviceDescriptor*> controllers; // List of input devices that have buttons
	Vrui::Point customProbeTip; // Probe tip position defined on the command line
	Vrui::Point probeTip; // Position of probe tip in controller's local coordinate system
	std::vector<Vrui::VRBaseStation> baseStations; // List of tracking base stations
	
	/* Environment definition: */
	std::string rootSectionName; // Name of root section to set up
	Vrui::EnvironmentDefinition initial; // Initial environment definition read from device daemon
	Vrui::Scalar meterScale; // Length of one meter in physical environment units
	Vrui::EnvironmentDefinition current; // Current edited environment definition
	Vrui::Scalar centerHeight; // Height of center point above the floor plane
	bool haveControlWindow; // Flag whether a control window configuration file fragment was found
	Vrui::Point controlViewerEyePos; // Eye position of control viewer relative to head device transformation
	Vrui::Point controlScreenCenter; // Center of control screen relative to pre-transformation
	
	/* Setup state: */
	Vrui::Scalar snapDistance; // Maximum distance to snap controller positions against existing points in physical coordinate units
	Modes mode; // Current set-up mode
	Vrui::Scalar wallHeight; // Height for boundary walls
	PointList floorPoints; // List of floor set-up points; first point is tentative environment center
	Polygon boundary; // Current boundary polygon
	Polygon currentSurface; // Current surface polygon
	PolygonList surfaces; // List of completed surface polygons
	Vrui::ISize screenCalibrationGridSize; // Width and height of secondary screen calibration grid
	CalibrationPointList screenCalibrationPoints; // Vector of points on the secondary display screen
	unsigned int nextCalibrationIndex; // Index of the next calibration point to be captured
	ONTransform screenTransform; // Calibrated control screen transformation
	double screenSize[2]; // Calibrated control screen size
	Homography screenHomography; // Calibrated control screen homography
	bool haveScreenCalibration; // Flag whether a screen calibration has been calculated
	
	/* Rendering state: */
	bool render3D; // Flag whether to render controller or base station positions and boundaries in 3D
	bool showBaseStations; // Flag whether to render base stations and their tracking volumes
	
	/* UI state: */
	GLMotif::PopupMenu* mainMenu; // The main menu
	GLMotif::PopupWindow* setupDialogPopup; // The setup dialog
	GLMotif::TextField* probeTipTextFields[3]; // Text fields displaying current probe tip position in device coordinates
	GLMotif::TextField* centerTextFields[3]; // Text fields displaying environment center position
	GLMotif::TextField* upTextFields[3]; // Text fields displaying environment up vector
	GLMotif::ToggleButton* measureFloorToggle; // Toggle button to measure the floor plane
	GLMotif::TextField* forwardTextFields[3]; // Text fields displaying environment forward vector
	
	/* Interaction state: */
	Threads::TripleBuffer<Vrui::TrackerState*> controllerStates; // Triple buffer of arrays of current controller tracking states
	int useButtonIndex; // Index of button to use for selection or -1 if any button can be used
	std::string useButtonName; // Name of button(s) to use for selection or empty if any button can be used
	int previousPressedButtonIndex; // Index of the last pressed controller button
	Threads::TripleBuffer<int> pressedButtonIndex; // Triple buffer containing index of the currently pressed controller button, or -1
	Vrui::Point::AffineCombiner pointCombiner; // Accumulator to sample controller positions
	Vrui::Vector vectorCombiner; // Accumulator to sample controller directions
	
	/* Private methods: */
	GLMotif::PopupMenu* createMainMenu(void); // Creates the main menu
	void setupDialogPageChangedCallback(GLMotif::Pager::PageChangedCallbackData* cbData);
	void controllerTypeValueChangedCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData);
	void probeTipTextFieldValueChangeCallback(GLMotif::TextField::ValueChangedCallbackData* cbData,const int& textFieldIndex);
	void measureFloorToggleValueChangedCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	void floorResetButtonCallback(Misc::CallbackData* cbData);
	void boundaryResetButtonCallback(Misc::CallbackData* cbData);
	void surfacesCloseSurfaceButtonCallback(Misc::CallbackData* cbData);
	void surfacesResetButtonCallback(Misc::CallbackData* cbData);
	void skipCalibrationPointCallback(Misc::CallbackData* cbData);
	void resetScreenCalibrationCallback(Misc::CallbackData* cbData);
	PolygonList createBoundary(void);
	void saveButtonCallback(Misc::CallbackData* cbData);
	GLMotif::PopupWindow* createSetupDialog(bool haveCustomProbeTip); // Creates a dialog window to control the setup process
	void trackingCallback(Vrui::VRDeviceClient* client); // Called when new tracking data arrives
	Vrui::Point project(const Vrui::Point& p) const // Projects a point to the current floor plane along the current up direction
		{
		return p+current.up*((current.center-p)*current.up);
		}
	Vrui::Vector project(const Vrui::Vector& v) const // Projects a vector into the current floor plane
		{
		return v-current.up*(v*current.up);
		}
	Vrui::Scalar calcRoomSize(void) const; // Calculates the radius of interesting stuff that should be rendered
	void calibrateControlScreen(void); // Calculates control screen parameters after a set of calibration points have been captured
	
	/* Constructors and destructors: */
	public:
	RoomSetup(int& argc,char**& argv);
	virtual ~RoomSetup(void);
	
	/* Methods from Vrui::Application: */
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	virtual void resetNavigation(void);
	
	/* Methods from Vrui::TransparentObject: */
	virtual void glRenderActionTransparent(GLContextData& contextData) const;
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	};

/************************************
Methods of class RoomSetup::DataItem:
************************************/

RoomSetup::DataItem::DataItem(void)
	:floorTextureId(0)
	{
	glGenTextures(1,&floorTextureId);
	}

RoomSetup::DataItem::~DataItem(void)
	{
	glDeleteTextures(1,&floorTextureId);
	}

/**************************
Methods of class RoomSetup:
**************************/

GLMotif::PopupMenu* RoomSetup::createMainMenu(void)
	{
	/* Create the main menu: */
	GLMotif::PopupMenu* mainMenu=new GLMotif::PopupMenu("MainMenu",Vrui::getWidgetManager());
	mainMenu->setTitle("Room Setup");
	
	/* Add a toggle button to render in 3D: */
	GLMotif::ToggleButton* render3DToggle=new GLMotif::ToggleButton("Render3DToggle",mainMenu,"Draw in 3D");
	render3DToggle->track(render3D);
	
	/* Add a toggle button to show base stations and their tracking volumes: */
	GLMotif::ToggleButton* showBaseStationsToggle=new GLMotif::ToggleButton("ShowBaseStationsToggle",mainMenu,"Show Base Stations");
	showBaseStationsToggle->track(showBaseStations);
	
	/* Finish and return the main menu: */
	mainMenu->manageMenu();
	return mainMenu;
	}

void RoomSetup::setupDialogPageChangedCallback(GLMotif::Pager::PageChangedCallbackData* cbData)
	{
	switch(cbData->newCurrentChildIndex)
		{
		case 0:
			mode=Controller;
			break;
		
		case 1:
			mode=Floor;
			break;
		
		case 2:
			mode=Forward;
			break;
		
		case 3:
			mode=Boundary;
			break;
		
		case 4:
			mode=Surfaces;
			break;
		
		case 5:
			mode=ControlScreen;
			break;
		}
	}

void RoomSetup::controllerTypeValueChangedCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData)
	{
	/* Update the probe tip position: */
	bool allowEditing=false;
	switch(cbData->newSelectedItem)
		{
		case 0: // Raw from device driver
			probeTip=Point::origin;
			break;
		
		case 1: // Custom controller
			probeTip=customProbeTip;
			allowEditing=true;
			break;
		
		case 2: // Vive DK1 controller
			probeTip=Point(0.0,-0.015,-0.041);
			break;
		
		case 3: // Vive and Vive Pre controller
			probeTip=Point(0.0,-0.075,-0.039);
			break;
		}
	
	/* Update the probe tip text fields: */
	for(int i=0;i<3;++i)
		{
		probeTipTextFields[i]->setEditable(allowEditing);
		probeTipTextFields[i]->setValue(probeTip[i]);
		}
	}

void RoomSetup::probeTipTextFieldValueChangeCallback(GLMotif::TextField::ValueChangedCallbackData* cbData,const int& textFieldIndex)
	{
	/* Store the new custom value and update the current value: */
	probeTip[textFieldIndex]=customProbeTip[textFieldIndex]=atof(cbData->value);
	}

void RoomSetup::measureFloorToggleValueChangedCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	if(cbData->set)
		{
		/* Reset the floor plane calculator: */
		floorPoints.clear();
		}
	else
		{
		/* Update the up direction if three or more floor points were captured: */
		if(floorPoints.size()>=3)
			{
			/* Calculate the floor plane via principal component analysis: */
			Geometry::PCACalculator<3> pca;
			for(PointList::iterator fIt=floorPoints.begin();fIt!=floorPoints.end();++fIt)
				pca.accumulatePoint(*fIt);
			
			pca.calcCovariance();
			double evs[3];
			pca.calcEigenvalues(evs);
			current.up=Geometry::normalize(Vrui::Vector(pca.calcEigenvector(evs[2])));
			if(current.up*initial.up<Vrui::Scalar(0))
				current.up=-current.up;
			
			for(int i=0;i<3;++i)
				upTextFields[i]->setValue(current.up[i]);
			
			/* Update the floor plane: */
			current.floor=Vrui::Plane(current.up,current.center);
			
			resetNavigation();
			}
		}
	}

void RoomSetup::floorResetButtonCallback(Misc::CallbackData* cbData)
	{
	/* Reset floor calibration: */
	current.center=initial.center;
	current.up=initial.up;
	current.floor=initial.floor;
	floorPoints.clear();
	for(int i=0;i<3;++i)
		{
		centerTextFields[i]->setValue(current.center[i]);
		upTextFields[i]->setValue(current.up[i]);
		}
	
	resetNavigation();
	}

void RoomSetup::boundaryResetButtonCallback(Misc::CallbackData* cbData)
	{
	/* Reset boundary setup: */
	current.radius=initial.radius;
	boundary.clear();
	
	resetNavigation();
	}

void RoomSetup::surfacesCloseSurfaceButtonCallback(Misc::CallbackData* cbData)
	{
	/* Add the current surface to the surfaces list if it has at least three vertices and start a new current surface: */
	if(currentSurface.size()>=3)
		surfaces.push_back(currentSurface);
	currentSurface.clear();
	}

void RoomSetup::skipCalibrationPointCallback(Misc::CallbackData* cbData)
	{
	/* Advance the calibration counter if calibration is not done: */
	if(nextCalibrationIndex<screenCalibrationGridSize.volume())
		{
		++nextCalibrationIndex;
		
		/* Calibrate the screen if a full set of calibration points have been captured: */
		if(nextCalibrationIndex==screenCalibrationGridSize.volume())
			calibrateControlScreen();
		}
	}

void RoomSetup::resetScreenCalibrationCallback(Misc::CallbackData* cbData)
	{
	/* Reset screen calibration procedure: */
	screenCalibrationPoints.clear();
	nextCalibrationIndex=0;
	haveScreenCalibration=false;
	}

void RoomSetup::surfacesResetButtonCallback(Misc::CallbackData* cbData)
	{
	/* Reset surfaces setup: */
	currentSurface.clear();
	surfaces.clear();
	}

RoomSetup::PolygonList RoomSetup::createBoundary(void)
	{
	PolygonList result;
	
	if(boundary.size()>=3)
		{
		/* Create one wall segment for each boundary line segment: */
		unsigned int i0=boundary.size()-1;
		for(unsigned int i1=0;i1<boundary.size();i0=i1,++i1)
			{
			/* Turn the two boundary vertices into a wall rectangle: */
			Polygon wall;
			wall.push_back(project(boundary[i0]));
			wall.push_back(project(boundary[i1]));
			wall.push_back(project(boundary[i1])+current.up*wallHeight);
			wall.push_back(project(boundary[i0])+current.up*wallHeight);
			result.push_back(wall);
			}
	
		/* Create the floor polygon: */
		Polygon floor;
		for(Polygon::iterator bIt=boundary.begin();bIt!=boundary.end();++bIt)
			floor.push_back(project(*bIt));
		result.push_back(floor);
		}
	
	/* Create a screen protector area for each horizontal surface: */
	Vrui::Scalar upSqr=current.up.sqr();
	for(PolygonList::iterator sIt=surfaces.begin();sIt!=surfaces.end();++sIt)
		{
		/* Calculate the average height of this surface: */
		Vrui::Scalar averageHeight(0);
		for(Polygon::iterator svIt=sIt->begin();svIt!=sIt->end();++svIt)
			averageHeight+=(*svIt-Vrui::Point::origin)*current.up;
		averageHeight/=Vrui::Scalar(sIt->size());
		
		/* Project this surface to its average height: */
		Polygon surface;
		for(Polygon::iterator svIt=sIt->begin();svIt!=sIt->end();++svIt)
			{
			Vrui::Scalar lambda=(averageHeight-(*svIt-Vrui::Point::origin)*current.up)/upSqr;
			surface.push_back(*svIt+current.up*lambda);
			}
		result.push_back(surface);
		}
	
	return result;
	}

void RoomSetup::saveButtonCallback(Misc::CallbackData* cbData)
	{
	/* Create a temporary environment definition and upload it to the VR device daemon: */
	Vrui::EnvironmentDefinition upload=current;
	upload.center+=upload.up*centerHeight;
	upload.boundary=createBoundary();
	deviceClient->updateEnvironmentDefinition(upload);
	
	/* Create a per-user or system-wide environment definition configuration file: */
	#if VRUI_INTERNAL_CONFIG_HAVE_USERCONFIGFILE
	const char* home=getenv("HOME");
	if(home==0||home[0]=='\0')
		{
		Misc::userError("Save Layout: No $HOME variable defined; cannot create per-user environment definition file");
		return;
		}
	std::string configDirName=home;
	configDirName.push_back('/');
	configDirName.append(VRUI_INTERNAL_CONFIG_USERCONFIGDIR);
	#else
	std::string configDirName=VRUI_INTERNAL_CONFIG_SYSCONFIGDIR;
	#endif
	
	/* Create the configuration directory if it doesn't exist yet: */
	if(!Misc::doesPathExist(configDirName.c_str()))
		if(mkdir(configDirName.c_str(),S_IRWXU|S_IRWXG|S_IRWXO)!=0)
			{
			int error=errno;
			Misc::formattedUserError("Save Layout: Unable to create per-user configuration directory due to error %d (%s)",error,strerror(error));
			return;
			}
	
	try
		{
		/* Write the environment definition configuration file: */
		std::string environmentDefinitionFileName=configDirName;
		environmentDefinitionFileName.push_back('/');
		environmentDefinitionFileName.append("Environment");
		environmentDefinitionFileName.append(VRUI_INTERNAL_CONFIG_CONFIGFILESUFFIX);
		
		std::ofstream environmentDefinitionFile(environmentDefinitionFileName.c_str());
		
		/* Write basic layout parameters: */
		environmentDefinitionFile<<"unit "<<Misc::ValueCoder<Geometry::LinearUnit>::encode(upload.unit)<<std::endl;
		environmentDefinitionFile<<"up "<<Misc::ValueCoder<Vrui::Vector>::encode(upload.up)<<std::endl;
		environmentDefinitionFile<<"forward "<<Misc::ValueCoder<Vrui::Vector>::encode(upload.forward)<<std::endl;
		environmentDefinitionFile<<"center "<<Misc::ValueCoder<Vrui::Point>::encode(upload.center)<<std::endl;
		environmentDefinitionFile<<"radius "<<Misc::ValueCoder<Vrui::Scalar>::encode(upload.radius)<<std::endl;
		environmentDefinitionFile<<"floorPlane "<<Misc::ValueCoder<Vrui::Plane>::encode(upload.floor)<<std::endl;
		
		/* Write the list of boundary polygons: */
		environmentDefinitionFile<<"boundary (";
		for(PolygonList::iterator bIt=upload.boundary.begin();bIt!=upload.boundary.end();++bIt)
			{
			if(bIt!=upload.boundary.begin())
				environmentDefinitionFile<<", \\"<<std::endl<<"          ";
			Polygon::iterator pIt=bIt->begin();
			environmentDefinitionFile<<'('<<Misc::ValueCoder<Vrui::Point>::encode(*pIt);
			unsigned int numVertices=1;
			for(++pIt;pIt!=bIt->end();++pIt,++numVertices)
				{
				environmentDefinitionFile<<", ";
				if(numVertices%4==0)
					environmentDefinitionFile<<'\\'<<std::endl<<"           ";
				environmentDefinitionFile<<Misc::ValueCoder<Vrui::Point>::encode(*pIt);
				}
			environmentDefinitionFile<<')';
			}
		environmentDefinitionFile<<')'<<std::endl;
		
		#if VRUI_INTERNAL_CONFIG_HAVE_USERCONFIGFILE
		Misc::formattedUserNote("Save Layout: Room layout saved to per-user configuration file %s",environmentDefinitionFileName.c_str());
		#else
		Misc::formattedUserNote("Save Layout: Room layout saved to system-wide configuration file %s",environmentDefinitionFileName.c_str());
		#endif
		}
	catch(const std::runtime_error& err)
		{
		Misc::formattedUserError("Save Layout: Unable to save room layout due to exception %s",err.what());
		return;
		}
	
	if(haveControlWindow)
		{
		if(haveScreenCalibration)
			{
			/* Calculate a control viewer transformation that faces the calibrated screen: */
			Vrui::Vector viewX=upload.up^screenTransform.getDirection(2);
			Vrui::Vector viewY=upload.up^viewX;
			Vrui::Scalar viewerDist=Math::sqrt(Math::sqr(screenSize[0])+Math::sqr(screenSize[1]))*3.0;
			Vrui::Point viewPos=screenTransform.transform(Vrui::Point(screenSize[0]*0.5,screenSize[1]*0.5,viewerDist));
			Vrui::ONTransform viewTransform(viewPos-Vrui::Point::origin,Vrui::Rotation::fromBaseVectors(viewX,viewY));
			Vrui::Vector viewDir(0,1,0);
			Vrui::Vector viewUpDir(0,0,1);
			Vrui::Point monoEyePos=Vrui::Point::origin;
			Vrui::Scalar eyeSep=Vrui::Scalar(0.0635)*meterScale;
			Vrui::Point leftEyePos(-Math::div2(eyeSep),0,0);
			Vrui::Point rightEyePos(Math::div2(eyeSep),0,0);
			
			/* Transform the screen homography to accept input points in clip space: */
			Homography clipTransform=Homography::identity;
			for(int i=0;i<2;++i)
				{
				clipTransform.getMatrix()(i,i)=0.5;
				clipTransform.getMatrix()(i,2)=0.5;
				}
			Homography clipHomography=screenHomography*clipTransform;
			
			/* Write the full transformation, size, and homography of the calibrated screen to the configuration file: */
			try
				{
				/* Try to open and adapt the standard control window configuration file fragment: */
				std::string cwConfigFileName=configDirName;
				cwConfigFileName.push_back('/');
				cwConfigFileName.append("ControlWindow");
				cwConfigFileName.append(VRUI_INTERNAL_CONFIG_CONFIGFILESUFFIX);
				
				/* Check if the target configuration file already exists: */
				if(Misc::doesPathExist(cwConfigFileName.c_str()))
					{
					/* Patch the target configuration file: */
					std::string tagPath="Vrui/";
					tagPath.append(rootSectionName);
					tagPath.push_back('/');
					Misc::ConfigurationFile::patchFile(cwConfigFileName.c_str(),(tagPath+"/ControlViewer/headDeviceTransformation").c_str(),Misc::ValueCoder<Vrui::ONTransform>::encode(viewTransform).c_str());
					Misc::ConfigurationFile::patchFile(cwConfigFileName.c_str(),(tagPath+"/ControlViewer/viewDirection").c_str(),Misc::ValueCoder<Vrui::Vector>::encode(viewDir).c_str());
					Misc::ConfigurationFile::patchFile(cwConfigFileName.c_str(),(tagPath+"/ControlViewer/upDirection").c_str(),Misc::ValueCoder<Vrui::Vector>::encode(viewUpDir).c_str());
					Misc::ConfigurationFile::patchFile(cwConfigFileName.c_str(),(tagPath+"/ControlViewer/monoEyePosition").c_str(),Misc::ValueCoder<Vrui::Point>::encode(monoEyePos).c_str());
					Misc::ConfigurationFile::patchFile(cwConfigFileName.c_str(),(tagPath+"/ControlViewer/leftEyePosition").c_str(),Misc::ValueCoder<Vrui::Point>::encode(leftEyePos).c_str());
					Misc::ConfigurationFile::patchFile(cwConfigFileName.c_str(),(tagPath+"/ControlViewer/rightEyePosition").c_str(),Misc::ValueCoder<Vrui::Point>::encode(rightEyePos).c_str());
					Misc::ConfigurationFile::patchFile(cwConfigFileName.c_str(),(tagPath+"/ControlViewer/headLightPosition").c_str(),Misc::ValueCoder<Vrui::Point>::encode(monoEyePos).c_str());
					Misc::ConfigurationFile::patchFile(cwConfigFileName.c_str(),(tagPath+"/ControlViewer/headLightDirection").c_str(),Misc::ValueCoder<Vrui::Vector>::encode(viewDir).c_str());
					
					Misc::ConfigurationFile::patchFile(cwConfigFileName.c_str(),(tagPath+"/ControlScreen/transform").c_str(),Misc::ValueCoder<Vrui::ONTransform>::encode(screenTransform).c_str());
					Misc::ConfigurationFile::patchFile(cwConfigFileName.c_str(),(tagPath+"/ControlScreen/width").c_str(),Misc::ValueCoder<double>::encode(screenSize[0]).c_str());
					Misc::ConfigurationFile::patchFile(cwConfigFileName.c_str(),(tagPath+"/ControlScreen/height").c_str(),Misc::ValueCoder<double>::encode(screenSize[1]).c_str());
					Misc::ConfigurationFile::patchFile(cwConfigFileName.c_str(),(tagPath+"/ControlScreen/offAxis").c_str(),Misc::ValueCoder<bool>::encode(true).c_str());
					Misc::ConfigurationFile::patchFile(cwConfigFileName.c_str(),(tagPath+"/ControlScreen/homography").c_str(),Misc::ValueCoder<Homography>::encode(clipHomography).c_str());
					}
				else
					{
					/* Write a new configuration file: */
					std::ofstream configFile(cwConfigFileName.c_str());
					configFile<<"section Vrui"<<std::endl;
					configFile<<"\tsection "<<rootSectionName<<std::endl;
					
					/* Write control viewer transformation: */
					configFile<<"\t\tsection ControlViewer"<<std::endl;
					configFile<<"\t\t\theadDeviceTransformation "<<Misc::ValueCoder<Vrui::ONTransform>::encode(viewTransform)<<std::endl;
					configFile<<"\t\tendsection"<<std::endl;
					
					configFile<<"\t\t"<<std::endl;
					
					/* Write control screen transformation: */
					configFile<<"\t\tsection ControlScreen"<<std::endl;
					configFile<<"\t\t\ttransform "<<Misc::ValueCoder<Vrui::ONTransform>::encode(screenTransform)<<std::endl;
					configFile<<"\t\t\twidth "<<Misc::ValueCoder<double>::encode(screenSize[0])<<std::endl;
					configFile<<"\t\t\theight "<<Misc::ValueCoder<double>::encode(screenSize[1])<<std::endl;
					configFile<<"\t\t\toffAxis "<<Misc::ValueCoder<bool>::encode(true)<<std::endl;
					configFile<<"\t\t\thomography "<<Misc::ValueCoder<Homography>::encode(screenHomography)<<std::endl;
					
					configFile<<"\t\tendsection"<<std::endl;
					
					configFile<<"\tendsection"<<std::endl;
					configFile<<"endsection"<<std::endl;
					}
				}
			catch(const std::runtime_error& err)
				{
				Misc::formattedUserError("Save Layout: Unable to adjust control window configuration due to exception %s",err.what());
				}
			}
		else
			{
			/* Calculate a transformation to center the control window with the environment's center point and look along the forward direction: */
			Vrui::ONTransform transform=Vrui::ONTransform::translateFromOriginTo(upload.center);
			Vrui::Vector horizontalForward=upload.forward;
			horizontalForward.orthogonalize(upload.up);
			Vrui::Vector horizontalControlView=controlScreenCenter-controlViewerEyePos;
			horizontalControlView.orthogonalize(upload.up);
			transform*=Vrui::ONTransform::rotate(Vrui::Rotation::rotateFromTo(horizontalControlView,horizontalForward));
			transform.renormalize();
			
			try
				{
				/* Try to open and adapt the standard control window configuration file fragment: */
				std::string cwConfigFileName=configDirName;
				cwConfigFileName.push_back('/');
				cwConfigFileName.append("ControlWindow");
				cwConfigFileName.append(VRUI_INTERNAL_CONFIG_CONFIGFILESUFFIX);
				
				/* Check if the target configuration file already exists: */
				if(Misc::doesPathExist(cwConfigFileName.c_str()))
					{
					/* Patch the target configuration file: */
					std::string tagPath="Vrui/";
					tagPath.append(rootSectionName);
					tagPath.push_back('/');
					Misc::ConfigurationFile::patchFile(cwConfigFileName.c_str(),(tagPath+"/ControlViewer/headDeviceTransformation").c_str(),Misc::ValueCoder<Vrui::ONTransform>::encode(transform).c_str());
					Misc::ConfigurationFile::patchFile(cwConfigFileName.c_str(),(tagPath+"/ControlScreen/preTransform").c_str(),Misc::ValueCoder<Vrui::ONTransform>::encode(transform).c_str());
					}
				else
					{
					/* Write a new configuration file: */
					std::ofstream configFile(cwConfigFileName.c_str());
					configFile<<"section Vrui"<<std::endl;
					configFile<<"\tsection "<<rootSectionName<<std::endl;
					
					/* Write control viewer transformation: */
					configFile<<"\t\tsection ControlViewer"<<std::endl;
					configFile<<"\t\t\theadDeviceTransformation "<<Misc::ValueCoder<Vrui::ONTransform>::encode(transform)<<std::endl;
					configFile<<"\t\tendsection"<<std::endl;
					
					configFile<<"\t\t"<<std::endl;
					
					/* Write control screen transformation: */
					configFile<<"\t\tsection ControlScreen"<<std::endl;
					configFile<<"\t\t\tpreTransform "<<Misc::ValueCoder<Vrui::ONTransform>::encode(transform)<<std::endl;
					configFile<<"\t\tendsection"<<std::endl;
					
					configFile<<"\tendsection"<<std::endl;
					configFile<<"endsection"<<std::endl;
					}
				}
			catch(const std::runtime_error& err)
				{
				Misc::formattedUserError("Save Layout: Unable to adjust control window configuration due to exception %s",err.what());
				}
			}
		}
	}

GLMotif::PopupWindow* RoomSetup::createSetupDialog(bool haveCustomProbeTip)
	{
	/* Get the style sheet: */
	const GLMotif::StyleSheet& ss=*Vrui::getUiStyleSheet();
	
	/* Create the dialog window: */
	GLMotif::PopupWindow* setupDialogPopup=new GLMotif::PopupWindow("SetupDialogPopup",Vrui::getWidgetManager(),"Environment Setup");
	setupDialogPopup->setHideButton(true);
	setupDialogPopup->setResizableFlags(true,false);
	
	GLMotif::RowColumn* setupDialog=new GLMotif::RowColumn("SetupDialog",setupDialogPopup,false);
	setupDialog->setOrientation(GLMotif::RowColumn::VERTICAL);
	setupDialog->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	setupDialog->setNumMinorWidgets(1);
	
	/* Create a multi-page notebook: */
	GLMotif::Pager* pager=new GLMotif::Pager("Pager",setupDialog,false);
	pager->setMarginWidth(ss.size);
	pager->getPageChangedCallbacks().add(this,&RoomSetup::setupDialogPageChangedCallback);
	
	/* Create the controller setup page: */
	pager->setNextPageName("Controller");
	
	GLMotif::Margin* controllerPaneMargin=new GLMotif::Margin("ControllerPaneMargin",pager,false);
	controllerPaneMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::VCENTER));
	
	GLMotif::RowColumn* controllerPane=new GLMotif::RowColumn("ControllerPane",controllerPaneMargin,false);
	controllerPane->setOrientation(GLMotif::RowColumn::VERTICAL);
	controllerPane->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	controllerPane->setNumMinorWidgets(2);
	
	/* Create a drop-down menu to select controller types: */
	new GLMotif::Label("ControllerTypeLabel",controllerPane,"Controller Type");
	
	GLMotif::DropdownBox* controllerTypeBox=new GLMotif::DropdownBox("ControllerTypeBox",controllerPane);
	controllerTypeBox->addItem("From Driver");
	controllerTypeBox->addItem("Custom");
	controllerTypeBox->addItem("Vive DK1");
	controllerTypeBox->addItem("Vive");
	controllerTypeBox->getValueChangedCallbacks().add(this,&RoomSetup::controllerTypeValueChangedCallback);
	controllerTypeBox->setSelectedItem(haveCustomProbeTip?1:0);
	
	/* Create a set of text fields to display the probe tip position: */
	new GLMotif::Label("ProbeTipLabel",controllerPane,"Probe Tip");
	
	GLMotif::RowColumn* probeTipBox=new GLMotif::RowColumn("ProbeTipBox",controllerPane,false);
	probeTipBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	probeTipBox->setPacking(GLMotif::RowColumn::PACK_GRID);
	probeTipBox->setNumMinorWidgets(1);
	
	for(int i=0;i<3;++i)
		{
		char textFieldName[]="ProbeTipTextField0";
		textFieldName[17]='0'+i;
		probeTipTextFields[i]=new GLMotif::TextField(textFieldName,probeTipBox,6);
		probeTipTextFields[i]->setPrecision(3);
		probeTipTextFields[i]->setFloatFormat(GLMotif::TextField::FIXED);
		probeTipTextFields[i]->setValue(probeTip[i]);
		probeTipTextFields[i]->getValueChangedCallbacks().add(this,&RoomSetup::probeTipTextFieldValueChangeCallback,i);
		}
	
	probeTipBox->manageChild();
	
	controllerPane->manageChild();
	
	controllerPaneMargin->manageChild();
	
	/* Create the floor setup page: */
	pager->setNextPageName("Floor Plane");
	
	GLMotif::Margin* floorPaneMargin=new GLMotif::Margin("FloorPaneMargin",pager,false);
	floorPaneMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::VCENTER));
	
	GLMotif::RowColumn* floorPane=new GLMotif::RowColumn("FloorPane",floorPaneMargin,false);
	floorPane->setOrientation(GLMotif::RowColumn::VERTICAL);
	floorPane->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	floorPane->setNumMinorWidgets(1);
	
	GLMotif::RowColumn* floorDisplayBox=new GLMotif::RowColumn("FloorDisplayBox",floorPane,false);
	floorDisplayBox->setOrientation(GLMotif::RowColumn::VERTICAL);
	floorDisplayBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	floorDisplayBox->setNumMinorWidgets(4);
	
	new GLMotif::Label("CenterLabel",floorDisplayBox,"Center");
	for(int i=0;i<3;++i)
		{
		char textFieldName[]="CenterTextField0";
		textFieldName[15]='0'+i;
		centerTextFields[i]=new GLMotif::TextField(textFieldName,floorDisplayBox,8);
		centerTextFields[i]->setPrecision(3);
		centerTextFields[i]->setFloatFormat(GLMotif::TextField::FIXED);
		centerTextFields[i]->setValue(initial.center[i]);
		}
	
	new GLMotif::Label("UpLabel",floorDisplayBox,"Up");
	for(int i=0;i<3;++i)
		{
		char textFieldName[]="UpTextField0";
		textFieldName[11]='0'+i;
		upTextFields[i]=new GLMotif::TextField(textFieldName,floorDisplayBox,8);
		upTextFields[i]->setPrecision(3);
		upTextFields[i]->setFloatFormat(GLMotif::TextField::FIXED);
		upTextFields[i]->setValue(initial.up[i]);
		}
	
	for(int i=1;i<4;++i)
		floorDisplayBox->setColumnWeight(i,1.0f);
	floorDisplayBox->manageChild();
	
	GLMotif::Margin* floorButtonMargin=new GLMotif::Margin("FloorButtonMargin",floorPane,false);
	floorButtonMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::HCENTER));
	
	GLMotif::RowColumn* floorButtonBox=new GLMotif::RowColumn("FloorButtonBox",floorButtonMargin,false);
	floorButtonBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	floorButtonBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	
	measureFloorToggle=new GLMotif::ToggleButton("MeasureFloorToggle",floorButtonBox,"Measure Floor Plane");
	measureFloorToggle->getValueChangedCallbacks().add(this,&RoomSetup::measureFloorToggleValueChangedCallback);
	
	GLMotif::Button* floorResetButton=new GLMotif::Button("FloorResetButton",floorButtonBox,"Reset");
	floorResetButton->getSelectCallbacks().add(this,&RoomSetup::floorResetButtonCallback);
	
	floorButtonBox->manageChild();
	
	floorButtonMargin->manageChild();
	
	floorPane->manageChild();
	
	floorPaneMargin->manageChild();
	
	/* Create the forward direction setup page: */
	pager->setNextPageName("Forward Direction");
	
	GLMotif::Margin* forwardPaneMargin=new GLMotif::Margin("ForwardPaneMargin",pager,false);
	forwardPaneMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::VCENTER));
	
	GLMotif::RowColumn* forwardPane=new GLMotif::RowColumn("ForwardPane",forwardPaneMargin,false);
	forwardPane->setOrientation(GLMotif::RowColumn::VERTICAL);
	forwardPane->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	forwardPane->setNumMinorWidgets(4);
	
	new GLMotif::Label("ForwardLabel",forwardPane,"Forward");
	for(int i=0;i<3;++i)
		{
		char textFieldName[]="ForwardTextField0";
		textFieldName[16]='0'+i;
		forwardTextFields[i]=new GLMotif::TextField(textFieldName,forwardPane,8);
		forwardTextFields[i]->setPrecision(3);
		forwardTextFields[i]->setFloatFormat(GLMotif::TextField::FIXED);
		forwardTextFields[i]->setValue(initial.forward[i]);
		}
	
	for(int i=1;i<4;++i)
		forwardPane->setColumnWeight(i,1.0f);
	forwardPane->manageChild();
	
	forwardPaneMargin->manageChild();
	
	/* Create the boundary polygon setup page: */
	pager->setNextPageName("Boundary Polygon");
	
	GLMotif::Margin* boundaryMargin=new GLMotif::Margin("BoundaryMargin",pager,false);
	boundaryMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::HCENTER,GLMotif::Alignment::VCENTER));
	
	GLMotif::Button* boundaryResetButton=new GLMotif::Button("BoundaryResetButton",boundaryMargin,"Reset");
	boundaryResetButton->getSelectCallbacks().add(this,&RoomSetup::boundaryResetButtonCallback);
	
	boundaryMargin->manageChild();
	
	/* Create the surface polygon setup page: */
	pager->setNextPageName("Surface Polygons");
	
	GLMotif::Margin* surfacesMargin=new GLMotif::Margin("SurfacesMargin",pager,false);
	surfacesMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::HCENTER,GLMotif::Alignment::VCENTER));
	
	GLMotif::RowColumn* surfacesButtons=new GLMotif::RowColumn("SurfacesButtons",surfacesMargin,false);
	surfacesButtons->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	surfacesButtons->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	
	GLMotif::Button* surfacesCloseSurfaceButton=new GLMotif::Button("SurfacesCloseSurfaceButton",surfacesButtons,"Close Surface");
	surfacesCloseSurfaceButton->getSelectCallbacks().add(this,&RoomSetup::surfacesCloseSurfaceButtonCallback);
	
	GLMotif::Button* surfacesResetButton=new GLMotif::Button("SurfacesResetButton",surfacesButtons,"Reset");
	surfacesResetButton->getSelectCallbacks().add(this,&RoomSetup::surfacesResetButtonCallback);
	
	surfacesButtons->manageChild();
	
	surfacesMargin->manageChild();
	
	if(haveControlWindow)
		{
		/* Create the secondary screen calibration page: */
		pager->setNextPageName("Control Screen");
		
		GLMotif::Margin* controlScreenMargin=new GLMotif::Margin("ControlScreenMargin",pager,false);
		controlScreenMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::HCENTER,GLMotif::Alignment::VCENTER));
		
		GLMotif::RowColumn* controlScreenPane=new GLMotif::RowColumn("ControlScreenPane",controlScreenMargin,false);
		controlScreenPane->setOrientation(GLMotif::RowColumn::VERTICAL);
		controlScreenPane->setPacking(GLMotif::RowColumn::PACK_TIGHT);
		
		GLMotif::Margin* gridSizeMargin=new GLMotif::Margin("GridSizeMargin",controlScreenPane,false);
		gridSizeMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::LEFT,GLMotif::Alignment::VCENTER));
		
		GLMotif::RowColumn* gridSizeBox=new GLMotif::RowColumn("GridSizeBox",gridSizeMargin,false);
		gridSizeBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
		gridSizeBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
		
		new GLMotif::Label("GridSizeLabel",gridSizeBox,"Grid Size");
		
		GLMotif::TextField* gridSizeX=new GLMotif::TextField("GridSizeX",gridSizeBox,6);
		gridSizeX->setValueType(GLMotif::TextField::UINT);
		gridSizeX->setEditable(true);
		gridSizeX->track(screenCalibrationGridSize[0]);
		gridSizeX->getValueChangedCallbacks().add(this,&RoomSetup::resetScreenCalibrationCallback);
		
		GLMotif::TextField* gridSizeY=new GLMotif::TextField("GridSizeY",gridSizeBox,6);
		gridSizeY->setValueType(GLMotif::TextField::UINT);
		gridSizeY->setEditable(true);
		gridSizeY->track(screenCalibrationGridSize[1]);
		gridSizeY->getValueChangedCallbacks().add(this,&RoomSetup::resetScreenCalibrationCallback);
		
		gridSizeBox->manageChild();
		
		gridSizeMargin->manageChild();
		
		GLMotif::Margin* buttonMargin=new GLMotif::Margin("ButtonMargin",controlScreenPane,false);
		buttonMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::LEFT,GLMotif::Alignment::VCENTER));
		
		GLMotif::RowColumn* buttonBox=new GLMotif::RowColumn("ButtonBox",buttonMargin,false);
		buttonBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
		buttonBox->setPacking(GLMotif::RowColumn::PACK_GRID);
		
		GLMotif::Button* skipCalibrationPointButton=new GLMotif::Button("SkipCalibrationPointButton",buttonBox,"Skip Point");
		skipCalibrationPointButton->getSelectCallbacks().add(this,&RoomSetup::skipCalibrationPointCallback);
		
		GLMotif::Button* resetScreenCalibrationButton=new GLMotif::Button("ResetScreenCalibrationButton",buttonBox,"Reset");
		resetScreenCalibrationButton->getSelectCallbacks().add(this,&RoomSetup::resetScreenCalibrationCallback);
		
		buttonBox->manageChild();
		
		buttonMargin->manageChild();
		
		controlScreenPane->manageChild();
		
		controlScreenMargin->manageChild();
		}
	
	pager->setCurrentChildIndex(0);
	pager->manageChild();
	
	GLMotif::Margin* buttonMargin=new GLMotif::Margin("ButtonMargin",setupDialog,false);
	buttonMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::RIGHT));
	
	GLMotif::Button* saveButton=new GLMotif::Button("SaveButton",buttonMargin,"Save Layout");
	saveButton->getSelectCallbacks().add(this,&RoomSetup::saveButtonCallback);
	
	buttonMargin->manageChild();
	
	setupDialog->manageChild();
	
	return setupDialogPopup;
	}

void RoomSetup::trackingCallback(Vrui::VRDeviceClient* client)
	{
	/* Lock and retrieve the most recent input device states: */
	deviceClient->lockState();
	const Vrui::VRDeviceState& state=deviceClient->getState();
	
	/* Extract all controller's current tracking states into a new triple buffer slot: */
	Vrui::TrackerState* tss=controllerStates.startNewValue();
	for(unsigned int i=0;i<controllers.size();++i)
		tss[i]=state.getTrackerState(controllers[i]->trackerIndex).positionOrientation;
	
	/* Check if the button state changed: */
	int newPressedButtonIndex=previousPressedButtonIndex;
	if(newPressedButtonIndex==-1)
		{
		if(useButtonIndex>=0)
			{
			/* Check if the selected button is pressed: */
			if(state.getButtonState(useButtonIndex))
				newPressedButtonIndex=useButtonIndex;
			}
		else if(!useButtonName.empty())
			{
			/* Check if any controller buttons that have the selected name are pressed: */
			for(unsigned int i=0;i<controllers.size();++i)
				{
				for(int j=0;j<controllers[i]->numButtons;++j)
					if(controllers[i]->buttonNames[j]==useButtonName)
						{
						int buttonIndex=controllers[i]->buttonIndices[j];
						if(state.getButtonState(buttonIndex))
							newPressedButtonIndex=buttonIndex;
						}
				}
			}
		else
			{
			/* Check if any controller buttons are pressed: */
			for(unsigned int i=0;i<controllers.size();++i)
				{
				for(int j=0;j<controllers[i]->numButtons;++j)
					{
					int buttonIndex=controllers[i]->buttonIndices[j];
					if(state.getButtonState(buttonIndex))
						newPressedButtonIndex=buttonIndex;
					}
				}
			}
		}
	else
		{
		/* Check if the previous pressed button is still pressed: */
		if(!state.getButtonState(newPressedButtonIndex))
			newPressedButtonIndex=-1;
		}
	if(previousPressedButtonIndex!=newPressedButtonIndex)
		{
		pressedButtonIndex.postNewValue(newPressedButtonIndex);
		previousPressedButtonIndex=newPressedButtonIndex;
		}
	
	/* Release input device state lock: */
	deviceClient->unlockState();
	
	/* Post the new controller states and wake up the main thread: */
	controllerStates.postNewValue();
	Vrui::requestUpdate();
	}

Vrui::Scalar RoomSetup::calcRoomSize(void) const
	{
	/* Initialize room size to the current display size: */
	Vrui::Scalar roomSize(current.radius);
	
	/* Add the locations of all tracking base stations: */
	for(std::vector<Vrui::VRBaseStation>::const_iterator bsIt=baseStations.begin();bsIt!=baseStations.end();++bsIt)
		roomSize=Math::max(roomSize,Geometry::dist(current.center,project(bsIt->getPositionOrientation().getOrigin())));
	
	/* Add the boundary polygon and all surface polygons: */
	for(Polygon::const_iterator bIt=boundary.begin();bIt!=boundary.end();++bIt)
		roomSize=Math::max(roomSize,Geometry::dist(current.center,project(*bIt)));
	for(Polygon::const_iterator csIt=currentSurface.begin();csIt!=currentSurface.end();++csIt)
		roomSize=Math::max(roomSize,Geometry::dist(current.center,project(*csIt)));
	for(PolygonList::const_iterator sIt=surfaces.begin();sIt!=surfaces.end();++sIt)
		for(Polygon::const_iterator svIt=sIt->begin();svIt!=sIt->end();++svIt)
			roomSize=Math::max(roomSize,Geometry::dist(current.center,project(*svIt)));
	
	return roomSize;
	}

void RoomSetup::calibrateControlScreen(void)
	{
	/* Approximate the screen calibration points with a plane: */
	Geometry::PCACalculator<3> screenPca;
	for(CalibrationPointList::iterator scIt=screenCalibrationPoints.begin();scIt!=screenCalibrationPoints.end();++scIt)
		screenPca.accumulatePoint(*scIt);
	ONTransform::Point screenCenter(screenPca.calcCentroid());
	screenPca.calcCovariance();
	double evs[3];
	screenPca.calcEigenvalues(evs);
	ONTransform::Vector screenNormal=Geometry::normalize(ONTransform::Vector(screenPca.calcEigenvector(evs[2])));
	
	/* Ensure that the screen normal is pointing into the environment, assuming that the screen is on a wall outside the environment: */
	if(screenNormal*(current.center-screenCenter)<Vrui::Scalar(0))
		screenNormal=-screenNormal;
	
	/* Create an unaligned screen transformation for the plane fitting the calibration points: */
	screenTransform=ONTransform(screenCenter-ONTransform::Point::origin,ONTransform::Rotation::rotateFromTo(ONTransform::Vector(0,0,1),screenNormal));
	
	/* Project the screen calibration points into the approximate plane, calculate matching ideal screen points, and create the homography estimation linear system: */
	Math::Matrix ptp(9,9,0.0);
	for(CalibrationPointList::iterator scIt=screenCalibrationPoints.begin();scIt!=screenCalibrationPoints.end();++scIt)
		{
		/* Project the measured point into the screen plane: */
		ONTransform::Point m=screenTransform.inverseTransform(ONTransform::Point(*scIt));
		
		/* Calculate the associated ideal screen plane point: */
		unsigned int y=scIt->value;
		unsigned int x=y%screenCalibrationGridSize[0];
		y/=screenCalibrationGridSize[0];
		double ix=(double(x)+0.25)/(double(screenCalibrationGridSize[0])-0.5);
		double iy=(double(y)+0.25)/(double(screenCalibrationGridSize[1])-0.5);
		
		/* Enter the point pair's first equation into the matrix: */
		double eq[9];
		eq[0]=ix;
		eq[1]=iy;
		eq[2]=1.0;
		eq[3]=0.0;
		eq[4]=0.0;
		eq[5]=0.0;
		eq[6]=-ix*m[0];
		eq[7]=-iy*m[0];
		eq[8]=-m[0];
		
		for(int i=0;i<9;++i)
			for(int j=0;j<9;++j)
				ptp(i,j)+=eq[i]*eq[j];
		
		/* Enter the point pair's second equation into the matrix: */
		eq[0]=0.0;
		eq[1]=0.0;
		eq[2]=0.0;
		eq[3]=ix;
		eq[4]=iy;
		eq[5]=1.0;
		eq[6]=-ix*m[1];
		eq[7]=-iy*m[1];
		eq[8]=-m[1];
		
		for(int i=0;i<9;++i)
			for(int j=0;j<9;++j)
				ptp(i,j)+=eq[i]*eq[j];
		}
	
	/* Solve the linear system by finding its smallest eigenvector: */
	std::pair<Math::Matrix,Math::Matrix> ji=ptp.jacobiIteration();
	int minEvIndex=0;
	double minEv=Math::abs(ji.second(0));
	for(int i=1;i<9;++i)
		{
		if(minEv>Math::abs(ji.second(i)))
			{
			minEvIndex=i;
			minEv=Math::abs(ji.second(i));
			}
		}
	
	/* Create the screen homography: */
	double s=ji.first(8,minEvIndex);
	for(int i=0;i<3;++i)
		for(int j=0;j<3;++j)
			screenHomography.getMatrix()(i,j)=ji.first(i*3+j,minEvIndex)/s;
	
	/* Rotate the screen transformation to align with the screen homography's primary axes: */
	Homography::Point l=screenHomography.transform(Homography::Point(0.0,0.5));
	Homography::Point r=screenHomography.transform(Homography::Point(1.0,0.5));
	Homography::Point b=screenHomography.transform(Homography::Point(0.5,0.0));
	Homography::Point t=screenHomography.transform(Homography::Point(0.5,1.0));
	Homography::Vector x=r-l;
	Homography::Vector y=t-b;
	Homography::Vector h=x/x.mag()+y/y.mag();
	double alpha=Math::acos((h*Homography::Vector(1,1))/Math::sqrt(h.sqr()*2.0));
	if(h[0]<h[1])
		alpha=-alpha;
	
	/* Find the screen homography's origin point: */
	Homography screenHomRot=Homography::rotate(Homography::Rotation(alpha));
	l=screenHomRot.transform(l);
	r=screenHomRot.transform(r);
	b=screenHomRot.transform(b);
	t=screenHomRot.transform(t);
	double x0=Math::min(l[0],r[0]);
	double x1=Math::max(l[0],r[0]);
	screenSize[0]=x1-x0;
	double y0=Math::min(b[1],t[1]);
	double y1=Math::max(b[1],t[1]);
	screenSize[1]=y1-y0;
	std::cout<<"Screen size "<<screenSize[0]<<'x'<<screenSize[1]<<std::endl;
	
	/* Update the screen transformation and screen homography: */
	screenTransform*=ONTransform::rotate(ONTransform::Rotation::rotateZ(-alpha));
	screenTransform*=ONTransform::translate(ONTransform::Vector(x0,y0,0));
	screenTransform.renormalize();
	
	screenHomography.leftMultiply(screenHomRot);
	screenHomography.leftMultiply(Homography::translate(Homography::Vector(-x0,-y0)));
	
	#if 0
	
	/* Compare the calibration results against the original target points: */
	for(unsigned int y=0;y<screenCalibrationGridSize[1];++y)
		{
		double iy=(double(y)+0.25)/(double(screenCalibrationGridSize[1])-0.5)*screenSize[1];
		for(unsigned int x=0;x<screenCalibrationGridSize[0];++x)
			{
			double ix=(double(x)+0.25)/(double(screenCalibrationGridSize[0])-0.5)*screenSize[0];
			
			Homography::Point sp=screenHomography.transform(Homography::Point(ix,iy));
			Vrui::Point pp=screenTransform.transform(Vrui::Point(sp[0],sp[1],0));
			std::cout<<Geometry::dist(pp,Vrui::Point(screenCalibrationPoints[y*screenCalibrationGridSize[0]+x]))<<std::endl;
			}
		}
	
	#endif
	
	/* We now have a screen calibration: */
	haveScreenCalibration=true;
	}

namespace {

/****************
Helper functions:
****************/

Misc::ConfigurationFile* loadConfigFile(const char* configFileName) // Loads a system-wide configuration file and merges it with a per-user configuration file of the same name
	{
	/* Open the system-wide configuration file: */
	std::string systemConfigFileName=VRUI_INTERNAL_CONFIG_SYSCONFIGDIR;
	systemConfigFileName.push_back('/');
	systemConfigFileName.append(configFileName);
	systemConfigFileName.append(VRUI_INTERNAL_CONFIG_CONFIGFILESUFFIX);
	Misc::ConfigurationFile* configFile=new Misc::ConfigurationFile(systemConfigFileName.c_str());
	
	#if VRUI_INTERNAL_CONFIG_HAVE_USERCONFIGFILE
	/* Merge per-user configuration file, if it exists: */
	try
		{
		const char* home=getenv("HOME");
		if(home!=0&&home[0]!='\0')
			{
			std::string userConfigFileName=home;
			userConfigFileName.push_back('/');
			userConfigFileName.append(VRUI_INTERNAL_CONFIG_USERCONFIGDIR);
			userConfigFileName.push_back('/');
			userConfigFileName.append(configFileName);
			userConfigFileName.append(VRUI_INTERNAL_CONFIG_CONFIGFILESUFFIX);
			configFile->merge(userConfigFileName.c_str());
			}
		}
	catch(const std::runtime_error&)
		{
		/* Ignore error and carry on... */
		}
	#endif
	
	return configFile;
	}

}

RoomSetup::RoomSetup(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 deviceClient(0),
	 customProbeTip(Point::origin),
	 haveControlWindow(false),
	 mode(Floor),
	 wallHeight(2.5),
	 haveScreenCalibration(false),
	 render3D(false),showBaseStations(false),
	 mainMenu(0),setupDialogPopup(0),
	 useButtonIndex(-1)
	{
	/* Parse command line: */
	const char* serverName="localhost:8555";
	const char* rootSectionNameStr=0;
	bool haveCustomProbeTip=false;
	std::vector<const char*> ignoredDevices;
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i]+1,"server")==0)
				{
				++i;
				if(i<argc)
					serverName=argv[i];
				}
			else if(strcasecmp(argv[i]+1,"probe")==0)
				{
				haveCustomProbeTip=true;
				for(int j=0;j<3;++j)
					{
					++i;
					customProbeTip[j]=Vrui::Scalar(atof(argv[i]));
					}
				std::cout<<"Custom probe tip position: "<<customProbeTip[0]<<", "<<customProbeTip[1]<<", "<<customProbeTip[2]<<std::endl;
				}
			else if(strcasecmp(argv[i]+1,"button")==0)
				{
				++i;
				if(i<argc)
					useButtonIndex=atoi(argv[i]);
				}
			else if(strcasecmp(argv[i]+1,"buttonName")==0)
				{
				++i;
				if(i<argc)
					useButtonName=argv[i];
				}
			else if(strcasecmp(argv[i]+1,"ignore")==0)
				{
				++i;
				if(i<argc)
					ignoredDevices.push_back(argv[i]);
				}
			}
		else if(rootSectionNameStr==0)
			rootSectionNameStr=argv[i];
		}
	if(rootSectionNameStr==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No root section name provided");
	rootSectionName=rootSectionNameStr;
	
	/* Split the server name into hostname:port: */
	const char* colonPtr=0;
	for(const char* cPtr=serverName;*cPtr!='\0';++cPtr)
		if(*cPtr==':')
			colonPtr=cPtr;
	int portNumber=0;
	if(colonPtr!=0)
		portNumber=atoi(colonPtr+1);
	std::string hostName=colonPtr!=0?std::string(serverName,colonPtr):std::string(serverName);
	
	/* Initialize the device client: */
	deviceClient=new Vrui::VRDeviceClient(dispatcher,hostName.c_str(),portNumber);
	
	/* Query a list of virtual devices that have buttons: */
	for(int i=0;i<deviceClient->getNumVirtualDevices();++i)
		{
		/* Store the device as a controller if it has position and direction tracking and at least one button: */
		const Vrui::VRDeviceDescriptor* device=&deviceClient->getVirtualDevice(i);
		if((device->trackType&Vrui::VRDeviceDescriptor::TRACK_POS)&&(device->trackType&Vrui::VRDeviceDescriptor::TRACK_DIR)&&device->numButtons>0)
			{
			/* Check if the device is in the list of ignored devices: */
			bool ignore=false;
			for(std::vector<const char*>::iterator idIt=ignoredDevices.begin();idIt!=ignoredDevices.end()&&!ignore;++idIt)
				ignore=device->name==*idIt;
			if(!ignore)
				controllers.push_back(device);
			}
		}
	
	/* Initialize the probe tip location: */
	probeTip=customProbeTip;
	
	/* Query the list of tracking base stations: */
	baseStations=deviceClient->getBaseStations();
	
	/* Read the server's environment definition: */
	if(!deviceClient->getEnvironmentDefinition(initial))
		{
		/* Server doesn't support environment definitions; read the environment configuration file instead: */
		std::string environmentDefinitionFileName=VRUI_INTERNAL_CONFIG_SYSCONFIGDIR;
		environmentDefinitionFileName.push_back('/');
		environmentDefinitionFileName.append("Environment.cfg");
		Misc::ConfigurationFile environmentDefinitionFile(environmentDefinitionFileName.c_str());
		
		/* Merge the per-user environment configuration file if it exists: */
		#if VRUI_INTERNAL_CONFIG_HAVE_USERCONFIGFILE
		const char* home=getenv("HOME");
		if(home!=0&&home[0]!='\0')
			{
			/* Construct the name of the per-user environment configuration file: */
			std::string userEnvironmentDefinitionFileName=home;
			userEnvironmentDefinitionFileName.push_back('/');
			userEnvironmentDefinitionFileName.append(VRUI_INTERNAL_CONFIG_USERCONFIGDIR);
			userEnvironmentDefinitionFileName.push_back('/');
			userEnvironmentDefinitionFileName.append("Environment.cfg");
			
			/* Merge the per-user environment configuration file if it exists: */
			if(Misc::doesPathExist(userEnvironmentDefinitionFileName.c_str()))
				environmentDefinitionFile.merge(userEnvironmentDefinitionFileName.c_str());
			}
		#endif
		
		/* Configure the environment: */
		initial.configure(environmentDefinitionFile.getCurrentSection());
		}
	
	/* Retrieve the length of one meter in physical coordinate units: */
	meterScale=initial.unit.getMeterFactor();
	
	/* Project the environment to the floor: */
	current=initial;
	current.center=initial.calcFloorPoint(initial.center);
	centerHeight=Geometry::dist(current.center,initial.center);
	
	/* Find the floor polygon and any horizontal surfaces in the initial environment definition's list of boundary polygons: */
	Vrui::Scalar floorTolerance=Vrui::Scalar(0.01)*meterScale; // 1cm expressed in environment physical units
	bool haveFloor=false;
	for(PolygonList::iterator bIt=initial.boundary.begin();bIt!=initial.boundary.end();++bIt)
		{
		bool isFloor=true;
		Math::Interval<Vrui::Scalar> heightRange=Math::Interval<Vrui::Scalar>::empty;
		for(Polygon::iterator pIt=bIt->begin();pIt!=bIt->end();++pIt)
			{
			/* Check if the current vertex is on the floor: */
			isFloor=isFloor&&initial.floor.calcDistance(*pIt)<floorTolerance;
			
			/* Add the current vertex's height to the height range to check for horizontal surfaces: */
			heightRange.addValue((*pIt-Vrui::Point::origin)*initial.up);
			}
		
		if(isFloor&&!haveFloor)
			{
			/* Create the initial boundary polygon: */
			boundary=*bIt;
			haveFloor=true;
			}
		else if(heightRange.getSize()<floorTolerance)
			{
			/* Store the polygon as a horizontal surface: */
			surfaces.push_back(*bIt);
			}
		}
	
	try
		{
		/* Open the standard control window configuration file fragment and go to the selected root section: */
		Misc::SelfDestructPointer<Misc::ConfigurationFile> configFile(loadConfigFile("ControlWindow"));
		Misc::ConfigurationFileSection rootSection=configFile->getSection("Vrui");
		rootSection.setSection(rootSectionNameStr);
		
		/* Retrieve the control viewer's mono eye position: */
		controlViewerEyePos=rootSection.retrieveValue<Vrui::Point>("./ControlViewer/monoEyePosition");
		
		/* Retrieve the control screen's screen rectangle: */
		Vrui::Point controlScreenOrigin=rootSection.retrieveValue<Vrui::Point>("./ControlScreen/origin");
		Vrui::Vector xAxis=rootSection.retrieveValue<Vrui::Vector>("./ControlScreen/horizontalAxis");
		xAxis*=rootSection.retrieveValue<Vrui::Scalar>("./ControlScreen/width");
		Vrui::Vector yAxis=rootSection.retrieveValue<Vrui::Vector>("./ControlScreen/verticalAxis");
		yAxis*=rootSection.retrieveValue<Vrui::Scalar>("./ControlScreen/height");
		controlScreenCenter=controlScreenOrigin+(xAxis+yAxis)*Vrui::Scalar(0.5);
		
		/* Initialize control window configuration state: */
		screenCalibrationGridSize=Vrui::ISize(4,3);
		nextCalibrationIndex=0;
		
		/* Remember that we found a control window configuration file: */
		haveControlWindow=true;
		}
	catch(const std::runtime_error& err)
		{
		/* Ignore the error and carry on: */
		}
	
	/* Initialize interaction state: */
	snapDistance=Vrui::Scalar(0.03)*meterScale;
	for(int i=0;i<3;++i)
		controllerStates.getBuffer(i)=new Vrui::TrackerState[controllers.size()];
	previousPressedButtonIndex=-1;
	
	/* Create the main menu: */
	mainMenu=createMainMenu();
	Vrui::setMainMenu(mainMenu);
	
	/* Create and show the setup dialog: */
	setupDialogPopup=createSetupDialog(haveCustomProbeTip);
	Vrui::popupPrimaryWidget(setupDialogPopup);
	
	/* Set up Vrui's navigation-space coordinate unit: */
	Vrui::getCoordinateManager()->setUnit(initial.unit);
	
	/* Activate the device client and start streaming: */
	deviceClient->activate();
	deviceClient->startStream(Misc::createFunctionCall(this,&RoomSetup::trackingCallback));
	}

RoomSetup::~RoomSetup(void)
	{
	/* Stop streaming and deactivate the device client: */
	deviceClient->stopStream();
	deviceClient->deactivate();
	delete deviceClient;
	
	/* Destroy the GUI: */
	delete mainMenu;
	delete setupDialogPopup;
	
	/* Clean up: */
	for(int i=0;i<3;++i)
		delete[] controllerStates.getBuffer(i);
	}

void RoomSetup::frame(void)
	{
	/* Lock the most recent controller state: */
	controllerStates.lockNewValue();
	
	/* Check if a new button was pressed: */
	if(pressedButtonIndex.lockNewValue())
		{
		if(pressedButtonIndex.getLockedValue()>=0)
			{
			switch(mode)
				{
				case Controller:
					// Do nothing yet...
					break;
				
				case Floor:
				case Boundary:
				case Surfaces:
				case ControlScreen:
					/* Reset the point combiner: */
					pointCombiner.reset();
					break;
				
				case Forward:
					/* Reset the vector combiner: */
					vectorCombiner=Vector::zero;
					break;
				}
			}
		else
			{
			switch(mode)
				{
				case Controller:
					// Do nothing yet...
					break;
				
				case Floor:
					if(measureFloorToggle->getToggle())
						{
						/* Add the sampled point to the floor points set: */
						floorPoints.push_back(pointCombiner.getPoint());
						}
					else
						{
						/* Set the display center: */
						current.center=pointCombiner.getPoint();
						for(int i=0;i<3;++i)
							centerTextFields[i]->setValue(current.center[i]);
						
						/* Update the floor plane: */
						current.floor=Vrui::Plane(current.up,current.center);
						
						resetNavigation();
						}
					
					break;
				
				case Forward:
					/* Set the forward direction: */
					current.forward=Geometry::normalize(project(vectorCombiner));
					for(int i=0;i<3;++i)
						forwardTextFields[i]->setValue(current.forward[i]);
					
					resetNavigation();
					break;
				
				case Boundary:
					/* Add the sampled point to the boundary polygon: */
					boundary.push_back(pointCombiner.getPoint());
					break;
				
				case Surfaces:
					{
					/* Snap the sampled point to the boundary polygon: */
					Vrui::Point point=pointCombiner.getPoint();
					Vrui::Point snappedPoint=project(point);
					Vrui::Scalar height=Geometry::dist(snappedPoint,point);
					point=snappedPoint;
					Vrui::Scalar snapDepth(0);
					Polygon::iterator bIt0=boundary.end()-1;
					for(Polygon::iterator bIt1=boundary.begin();bIt1!=boundary.end();bIt0=bIt1,++bIt1)
						{
						/* Check the point against the edge's start vertex: */
						Vrui::Vector sp=point-*bIt0;
						Vrui::Scalar depth=sp.mag()-snapDistance;
						if(snapDepth>depth)
							{
							snappedPoint=*bIt0;
							snapDepth=depth;
							}
						
						/* Check the point against the edge itself: */
						Vrui::Vector edge=*bIt1-*bIt0;
						Vrui::Scalar edgeLen=edge.mag();
						if(edgeLen>=snapDistance*Vrui::Scalar(0.667))
							{
							Vrui::Scalar edgeX=(sp*edge)/Math::sqr(edgeLen);
							if(edgeX>=Vrui::Scalar(0)&&edgeX<=Vrui::Scalar(1))
								{
								Vrui::Point edgePoint=Geometry::addScaled(*bIt0,edge,edgeX);
								depth=Geometry::dist(point,edgePoint)-snapDistance*Vrui::Scalar(0.667);
								if(snapDepth>depth)
									{
									snappedPoint=edgePoint;
									snapDepth=depth;
									}
								}
							}
						}
					
					/* Check if the sampled point closes the current surface polygon: */
					if(currentSurface.size()>=3&&Geometry::dist(snappedPoint,project(currentSurface.front()))<snapDistance)
						{
						/* Close the current surface and start a new one: */
						surfaces.push_back(currentSurface);
						currentSurface.clear();
						}
					else
						{
						if(currentSurface.empty())
							{
							/* Set the snapped point back to its original height: */
							snappedPoint+=current.up*height;
							}
						else
							{
							/* Set the snapped point to the same height as the first surface point: */
							snappedPoint-=current.up*(((snappedPoint-currentSurface.front())*current.up)/current.up.sqr());
							}
						
						/* Add the sampled point to the current surface: */
						currentSurface.push_back(snappedPoint);
						}
					
					break;
					}
				
				case ControlScreen:
					if(nextCalibrationIndex<screenCalibrationGridSize.volume())
						{
						/* Add the sampled point to the screen calibration point set: */
						screenCalibrationPoints.push_back(CalibrationPoint(pointCombiner.getPoint(),nextCalibrationIndex));
						++nextCalibrationIndex;
						
						/* Calibrate the screen if a full set of calibration points have been captured: */
						if(nextCalibrationIndex==screenCalibrationGridSize.volume())
							calibrateControlScreen();
						}
					break;
				}
			}
		}
	if(pressedButtonIndex.getLockedValue()>=0)
		{
		/* Find the controller to which this button belongs: */
		for(unsigned int i=0;i<controllers.size();++i)
			for(int j=0;j<controllers[i]->numButtons;++j)
				if(controllers[i]->buttonIndices[j]==pressedButtonIndex.getLockedValue())
					{
					/* Sample the controller whose button is pressed depending on setup mode: */
					switch(mode)
						{
						case Controller:
							// Do nothing yet...
							break;
						
						case Floor:
						case Boundary:
						case Surfaces:
						case ControlScreen:
							/* Accumulate the new controller position: */
							pointCombiner.addPoint(controllerStates.getLockedValue()[i].transform(probeTip));
							break;
						
						case Forward:
							/* Accumulate the controller's pointing direction: */
							vectorCombiner+=controllerStates.getLockedValue()[i].transform(controllers[i]->rayDirection);
							break;
						}
					
					goto foundController;
					}
		
		foundController:
		;
		}
	}

namespace {

/****************
Helper functions:
****************/

Vrui::Point calcFovPoint(Vrui::Scalar h,Vrui::Scalar v,Vrui::Scalar d)
	{
	Vrui::Point result(Math::sin(h)*Math::cos(v),Math::cos(h)*Math::sin(v),-Math::cos(h)*Math::cos(v));
	Vrui::Scalar scale=d/result.mag();
	for(int i=0;i<3;++i)
		result[i]*=scale;
	return result;
	}

}

void RoomSetup::display(GLContextData& contextData) const
	{
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT|GL_POINT_BIT);
	glDisable(GL_LIGHTING);
	
	if(mode==ControlScreen)
		{
		/* Find the position of the next calibration point to be captured: */
		unsigned int ya=nextCalibrationIndex;
		unsigned int xa=ya%screenCalibrationGridSize[0];
		ya/=screenCalibrationGridSize[0];
		if(ya>=screenCalibrationGridSize[1])
			ya=xa=-1;
		
		/* Draw the calibration grid: */
		glLineWidth(1.0f);
		
		glPushMatrix();
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		
		glBegin(GL_LINES);
		
		/* Draw horizontal lines: */
		for(unsigned int y=0;y<screenCalibrationGridSize[1];++y)
			{
			if(y==ya)
				glColor3f(0.0f,1.0f,0.0f);
			else
				glColor3f(0.0f,0.25f,0.0f);
			float ccy=(float(y)+0.25f)/(float(screenCalibrationGridSize[1])-0.5f)*2.0f-1.0f;
			glVertex3f(-1.0f,ccy,-1.0f);
			glVertex3f(1.0f,ccy,-1.0f);
			}
		
		/* Draw vertical lines: */
		for(unsigned int x=0;x<screenCalibrationGridSize[0];++x)
			{
			if(x==xa)
				glColor3f(0.0f,1.0f,0.0f);
			else
				glColor3f(0.0f,0.25f,0.0f);
			float ccx=(float(x)+0.25f)/(float(screenCalibrationGridSize[0])-0.5f)*2.0f-1.0f;
			glVertex3f(ccx,-1.0f,-1.0f);
			glVertex3f(ccx,1.0f,-1.0f);
			}
		
		glEnd();
		
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		}
	else
		{
		glLineWidth(3.0f);
		glPointSize(7.0f);
		
		/* Set up the floor coordinate system: */
		glPushMatrix();
		Vrui::Vector x=Geometry::normalize(current.forward^current.up);
		Vrui::Vector y=Geometry::normalize(current.up^x);
		glTranslate(current.center-Vrui::Point::origin);
		glRotate(Vrui::Rotation::fromBaseVectors(x,y));
		
		Vrui::Scalar size=Vrui::Scalar(Vrui::getUiSize())*current.radius*Vrui::Scalar(2)/Vrui::getDisplaySize();
		
		/* Draw the floor plane: */
		DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D,dataItem->floorTextureId);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
		
		Vrui::Scalar roomSize=calcRoomSize();
		Vrui::Scalar floorSize=Math::ceil(roomSize*Vrui::Scalar(2));
		
		double dt=1.0/1024.0;
		double fs=floorSize;
		glBegin(GL_QUADS);
		glTexCoord2d(dt,dt);
		glVertex3d(-fs,-fs,-0.001);
		glTexCoord2d(2.0*floorSize+dt,dt);
		glVertex3d(fs,-fs,-0.001);
		glTexCoord2d(2.0*floorSize+dt,2.0*floorSize+dt);
		glVertex3d(fs,fs,-0.001);
		glTexCoord2d(dt,2.0*floorSize+dt);
		glVertex3d(-fs,fs,-0.001);
		glEnd();
		glBindTexture(GL_TEXTURE_2D,0);
		glDisable(GL_TEXTURE_2D);
		
		glColor3f(1.0f,1.0f,1.0f);
		
		/* Draw the display center: */
		glBegin(GL_LINES);
		glVertex2d(-size*2.0,-size*2.0);
		glVertex2d(size*2.0,size*2.0);
		glVertex2d(-size*2.0,size*2.0);
		glVertex2d(size*2.0,-size*2.0);
		glEnd();
		
		/* Draw the display area: */
		glBegin(GL_LINE_LOOP);
		for(int i=0;i<64;++i)
			{
			Vrui::Scalar angle=Vrui::Scalar(2)*Vrui::Scalar(i)*Math::Constants<Vrui::Scalar>::pi/Vrui::Scalar(64);
			glVertex2d(Math::cos(angle)*current.radius,Math::sin(angle)*current.radius);
			}
		glEnd();
		
		/* Draw the forward direction: */
		glBegin(GL_LINE_LOOP);
		glVertex2d(size,0.0);
		glVertex2d(size,current.radius*0.5);
		glVertex2d(size*2.0,current.radius*0.5);
		glVertex2d(0.0,current.radius*0.5+size*2.0);
		glVertex2d(-size*2.0,current.radius*0.5);
		glVertex2d(-size,current.radius*0.5);
		glVertex2d(-size,0.0);
		glEnd();
		
		glPopMatrix();
		
		/* Draw the current boundary polygon: */
		glColor3f(1.0f,0.0f,0.0f);
		if(boundary.size()>1)
			{
			glBegin(GL_LINE_LOOP);
			for(Polygon::const_iterator bIt=boundary.begin();bIt!=boundary.end();++bIt)
				glVertex(project(*bIt));
			glEnd();
			}
		else if(boundary.size()==1)
			{
			glBegin(GL_POINTS);
			glVertex(project(boundary.front()));
			glEnd();
			}
		
		/* Draw all completed surfaces: */
		glColor3f(0.0f,0.5f,0.0f);
		for(PolygonList::const_iterator sIt=surfaces.begin();sIt!=surfaces.end();++sIt)
			{
			if(render3D)
				{
				glBegin(GL_LINE_LOOP);
				for(Polygon::const_iterator svIt=sIt->begin();svIt!=sIt->end();++svIt)
					glVertex(*svIt);
				glEnd();
				glBegin(GL_LINE_LOOP);
				for(Polygon::const_iterator svIt=sIt->begin();svIt!=sIt->end();++svIt)
					glVertex(project(*svIt));
				glEnd();
				glBegin(GL_LINES);
				for(Polygon::const_iterator svIt=sIt->begin();svIt!=sIt->end();++svIt)
					{
					glVertex(*svIt);
					glVertex(project(*svIt));
					}
				glEnd();
				}
			else
				{
				glBegin(GL_LINE_LOOP);
				for(Polygon::const_iterator svIt=sIt->begin();svIt!=sIt->end();++svIt)
					glVertex(project(*svIt));
				glEnd();
				}
			}
		
		/* Draw the current surface: */
		if(!currentSurface.empty())
			{
			if(render3D)
				{
				if(currentSurface.size()>1)
					{
					glBegin(GL_LINE_STRIP);
					for(Polygon::const_iterator csIt=currentSurface.begin();csIt!=currentSurface.end();++csIt)
						glVertex(*csIt);
					glEnd();
					glBegin(GL_LINE_STRIP);
					for(Polygon::const_iterator csIt=currentSurface.begin();csIt!=currentSurface.end();++csIt)
						glVertex(project(*csIt));
					glEnd();
					glBegin(GL_LINES);
					for(Polygon::const_iterator csIt=currentSurface.begin();csIt!=currentSurface.end();++csIt)
						{
						glVertex(*csIt);
						glVertex(project(*csIt));
						}
					glEnd();
					}
				glBegin(GL_POINTS);
				glVertex(currentSurface.back());
				glEnd();
				}
			else
				{
				if(currentSurface.size()>1)
					{
					glBegin(GL_LINE_STRIP);
					for(Polygon::const_iterator csIt=currentSurface.begin();csIt!=currentSurface.end();++csIt)
						glVertex(project(*csIt));
					glEnd();
					}
				glBegin(GL_POINTS);
				glVertex(project(currentSurface.back()));
				glEnd();
				}
			}
		
		/* Display the current controller positions: */
		glColor(Vrui::getForegroundColor());
		const Vrui::TrackerState* tss=controllerStates.getLockedValue();
		if(render3D)
			{
			glBegin(GL_POINTS);
			for(unsigned int i=0;i<controllers.size();++i)
				glVertex(tss[i].transform(probeTip));
			glEnd();
			glBegin(GL_LINES);
			for(unsigned int i=0;i<controllers.size();++i)
				{
				glVertex(tss[i].transform(probeTip));
				glVertex(project(tss[i].transform(probeTip)));
				}
			glEnd();
			}
		else
			{
			glBegin(GL_POINTS);
			for(unsigned int i=0;i<controllers.size();++i)
				glVertex(project(tss[i].transform(probeTip)));
			glEnd();
			}
		
		/* Draw the current calibrated display screen if there is one: */
		if(haveScreenCalibration)
			{
			/* Draw the original calibration points: */
			glPointSize(3.0f);
			glBegin(GL_POINTS);
			glColor3f(1.0f,1.0f,0.0f);
			for(std::vector<CalibrationPoint>::const_iterator scIt=screenCalibrationPoints.begin();scIt!=screenCalibrationPoints.end();++scIt)
				glVertex(*scIt);
			glEnd();
			
			/* Draw the optimally fitted screen rectangle: */
			glBegin(GL_LINE_LOOP);
			glColor3f(0.5f,0.5f,0.0f);
			glVertex(screenTransform.transform(Point(0,0,0)));
			glVertex(screenTransform.transform(Point(screenSize[0],0,0)));
			glVertex(screenTransform.transform(Point(screenSize[0],screenSize[1],0)));
			glVertex(screenTransform.transform(Point(0,screenSize[1],0)));
			glEnd();
			
			/* Draw the full screen homography: */
			glLineWidth(1.0f);
			glBegin(GL_LINES);
			glColor3f(1.0f,1.0f,0.0f);
			double* ys=new double[screenCalibrationGridSize[1]+2];
			ys[0]=0.0;
			for(unsigned int y=0;y<screenCalibrationGridSize[1];++y)
				ys[y+1]=(double(y)+0.25)/(double(screenCalibrationGridSize[1])-0.5);
			ys[screenCalibrationGridSize[1]+1]=1.0;
			for(unsigned int y=0;y<screenCalibrationGridSize[1]+2;++y)
				{
				Homography::Point p0=screenHomography.transform(Homography::Point(0,ys[y]));
				glVertex(screenTransform.transform(Point(p0[0],p0[1],0)));
				Homography::Point p1=screenHomography.transform(Homography::Point(1,ys[y]));
				glVertex(screenTransform.transform(Point(p1[0],p1[1],0)));
				}
			delete[] ys;
			
			double* xs=new double[screenCalibrationGridSize[0]+2];
			xs[0]=0.0;
			for(unsigned int x=0;x<screenCalibrationGridSize[0];++x)
				xs[x+1]=(double(x)+0.25)/(double(screenCalibrationGridSize[0])-0.5);
			xs[screenCalibrationGridSize[0]+1]=1.0;
			for(unsigned int x=0;x<screenCalibrationGridSize[0]+2;++x)
				{
				Homography::Point p0=screenHomography.transform(Homography::Point(xs[x],0));
				glVertex(screenTransform.transform(Point(p0[0],p0[1],0)));
				Homography::Point p1=screenHomography.transform(Homography::Point(xs[x],1));
				glVertex(screenTransform.transform(Point(p1[0],p1[1],0)));
				}
			delete[] xs;
			glEnd();
			glLineWidth(3.0f);
			}
		
		if(showBaseStations)
			{
			/* Draw the tracking base stations: */
			glColor3f(1.0f,0.5f,0.0f);
			if(render3D)
				{
				glBegin(GL_POINTS);
				for(std::vector<Vrui::VRBaseStation>::const_iterator bsIt=baseStations.begin();bsIt!=baseStations.end();++bsIt)
					if(bsIt->getTracking())
						glVertex(bsIt->getPositionOrientation().getOrigin());
				glEnd();
				glBegin(GL_LINES);
				for(std::vector<Vrui::VRBaseStation>::const_iterator bsIt=baseStations.begin();bsIt!=baseStations.end();++bsIt)
					if(bsIt->getTracking())
						{
						glVertex(bsIt->getPositionOrientation().getOrigin());
						glVertex(project(bsIt->getPositionOrientation().getOrigin()));
						}
				glEnd();
				}
			else
				{
				glBegin(GL_POINTS);
				for(std::vector<Vrui::VRBaseStation>::const_iterator bsIt=baseStations.begin();bsIt!=baseStations.end();++bsIt)
					if(bsIt->getTracking())
						glVertex(project(bsIt->getPositionOrientation().getOrigin()));
				glEnd();
				}
			
			if(render3D)
				for(std::vector<Vrui::VRBaseStation>::const_iterator bsIt=baseStations.begin();bsIt!=baseStations.end();++bsIt)
					if(bsIt->getTracking())
						{
						/* Go to the base station's coordinate system: */
						glPushMatrix();
						glMultMatrix(bsIt->getPositionOrientation());
						
						/* Draw the outer extents of the base station's tracking volume: */
						Vrui::Scalar l=Math::atan(bsIt->getFov()[0]);
						Vrui::Scalar r=Math::atan(bsIt->getFov()[1]);
						Vrui::Scalar b=Math::atan(bsIt->getFov()[2]);
						Vrui::Scalar t=Math::atan(bsIt->getFov()[3]);
						Vrui::Scalar n=bsIt->getRange()[0];
						Vrui::Scalar f=bsIt->getRange()[1];
						glBegin(GL_LINES);
						glVertex(calcFovPoint(l,b,n));
						glVertex(calcFovPoint(l,b,f));
						glVertex(calcFovPoint(r,b,n));
						glVertex(calcFovPoint(r,b,f));
						glVertex(calcFovPoint(r,t,n));
						glVertex(calcFovPoint(r,t,f));
						glVertex(calcFovPoint(l,t,n));
						glVertex(calcFovPoint(l,t,f));
						glEnd();
						
						glBegin(GL_LINE_LOOP);
						for(int hi=0;hi<=20;++hi)
							glVertex(calcFovPoint(Vrui::Scalar(hi)*(r-l)/Vrui::Scalar(20)+l,b,n));
						for(int vi=0;vi<=20;++vi)
							glVertex(calcFovPoint(r,Vrui::Scalar(vi)*(t-b)/Vrui::Scalar(20)+b,n));
						for(int hi=20;hi>=0;--hi)
							glVertex(calcFovPoint(Vrui::Scalar(hi)*(r-l)/Vrui::Scalar(20)+l,t,n));
						for(int vi=20;vi>=0;--vi)
							glVertex(calcFovPoint(l,Vrui::Scalar(vi)*(t-b)/Vrui::Scalar(20)+b,n));
						glEnd();
						
						glBegin(GL_LINE_LOOP);
						for(int hi=0;hi<=20;++hi)
							glVertex(calcFovPoint(Vrui::Scalar(hi)*(r-l)/Vrui::Scalar(20)+l,b,f));
						for(int vi=0;vi<=20;++vi)
							glVertex(calcFovPoint(r,Vrui::Scalar(vi)*(t-b)/Vrui::Scalar(20)+b,f));
						for(int hi=20;hi>=0;--hi)
							glVertex(calcFovPoint(Vrui::Scalar(hi)*(r-l)/Vrui::Scalar(20)+l,t,f));
						for(int vi=20;vi>=0;--vi)
							glVertex(calcFovPoint(l,Vrui::Scalar(vi)*(t-b)/Vrui::Scalar(20)+b,f));
						glEnd();
						
						glPopMatrix();
						}
			}
		}
	
	/* Reset OpenGL state: */
	glPopAttrib();
	}

void RoomSetup::resetNavigation(void)
	{
	/* Find the size of stuff that needs to be displayed: */
	Vrui::Scalar roomSize=calcRoomSize();
	
	/* Align the environment display: */
	Vrui::NavTransform nav;
	nav=Vrui::NavTransform::translateFromOriginTo(Vrui::getDisplayCenter());
	Vrui::Vector vruiRight=Geometry::normalize(Vrui::getForwardDirection()^Vrui::getUpDirection());
	Vrui::Rotation vruiBase=Vrui::Rotation::fromBaseVectors(vruiRight,Vrui::getUpDirection());
	Vrui::Vector right=Geometry::normalize(current.forward^current.up);
	Vrui::Rotation base=Vrui::Rotation::fromBaseVectors(right,current.forward);
	nav*=Vrui::NavTransform::rotate(vruiBase*Geometry::invert(base));
	nav*=Vrui::NavTransform::scale(Vrui::getDisplaySize()/(roomSize*Vrui::Scalar(2)));
	nav*=Vrui::NavTransform::translateToOriginFrom(current.center);
	Vrui::setNavigationTransformation(nav);
	}

void RoomSetup::glRenderActionTransparent(GLContextData& contextData) const
	{
	if(mode!=ControlScreen&&showBaseStations&&render3D)
		{
		/* Set up OpenGL state: */
		glPushAttrib(GL_COLOR_BUFFER_BIT|GL_ENABLE_BIT);
		glDisable(GL_LIGHTING);
		glDisable(GL_CULL_FACE);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		
		/* Go to navigational coordinates: */
		Vrui::goToNavigationalSpace(contextData);
		
		/* Render all base station's fields of view: */
		glColor4f(1.0f,0.5f,0.0f,0.1f);
		for(std::vector<Vrui::VRBaseStation>::const_iterator bsIt=baseStations.begin();bsIt!=baseStations.end();++bsIt)
			{
			if(bsIt->getTracking())
				{
				/* Go to the base station's coordinate system: */
				glPushMatrix();
				glMultMatrix(bsIt->getPositionOrientation());
				
				/* Retrieve FoV parameters: */
				Vrui::Scalar l=Math::atan(bsIt->getFov()[0]);
				Vrui::Scalar r=Math::atan(bsIt->getFov()[1]);
				Vrui::Scalar b=Math::atan(bsIt->getFov()[2]);
				Vrui::Scalar t=Math::atan(bsIt->getFov()[3]);
				Vrui::Scalar n=bsIt->getRange()[0];
				Vrui::Scalar f=bsIt->getRange()[1];
				
				/* Draw the near shell: */
				for(int vi=0;vi<20;++vi)
					{
					Vrui::Scalar v0=Vrui::Scalar(vi)*(t-b)/Vrui::Scalar(20)+b;
					Vrui::Scalar v1=Vrui::Scalar(vi+1)*(t-b)/Vrui::Scalar(20)+b;
					glBegin(GL_QUAD_STRIP);
					for(int hi=0;hi<=20;++hi)
						{
						Vrui::Scalar h=Vrui::Scalar(hi)*(r-l)/Vrui::Scalar(20)+l;
						glVertex(calcFovPoint(h,v1,n));
						glVertex(calcFovPoint(h,v0,n));
						}
					glEnd();
					}
				
				/* Draw the far shell: */
				for(int vi=0;vi<20;++vi)
					{
					Vrui::Scalar v0=Vrui::Scalar(vi)*(t-b)/Vrui::Scalar(20)+b;
					Vrui::Scalar v1=Vrui::Scalar(vi+1)*(t-b)/Vrui::Scalar(20)+b;
					glBegin(GL_QUAD_STRIP);
					for(int hi=0;hi<=20;++hi)
						{
						Vrui::Scalar h=Vrui::Scalar(hi)*(r-l)/Vrui::Scalar(20)+l;
						glVertex(calcFovPoint(h,v0,f));
						glVertex(calcFovPoint(h,v1,f));
						}
					glEnd();
					}
				
				/* Draw the left plane: */
				glBegin(GL_QUAD_STRIP);
				for(int vi=0;vi<=20;++vi)
					{
					Vrui::Scalar v=Vrui::Scalar(vi)*(t-b)/Vrui::Scalar(20)+b;
					glVertex(calcFovPoint(l,v,f));
					glVertex(calcFovPoint(l,v,n));
					}
				glEnd();
				
				/* Draw the right plane: */
				glBegin(GL_QUAD_STRIP);
				for(int vi=0;vi<=20;++vi)
					{
					Vrui::Scalar v=Vrui::Scalar(vi)*(t-b)/Vrui::Scalar(20)+b;
					glVertex(calcFovPoint(r,v,n));
					glVertex(calcFovPoint(r,v,f));
					}
				glEnd();
				
				/* Draw the bottom plane: */
				glBegin(GL_QUAD_STRIP);
				for(int hi=0;hi<=20;++hi)
					{
					Vrui::Scalar h=Vrui::Scalar(hi)*(r-l)/Vrui::Scalar(20)+l;
					glVertex(calcFovPoint(h,b,n));
					glVertex(calcFovPoint(h,b,f));
					}
				glEnd();
				
				/* Draw the top plane: */
				glBegin(GL_QUAD_STRIP);
				for(int hi=0;hi<=20;++hi)
					{
					Vrui::Scalar h=Vrui::Scalar(hi)*(r-l)/Vrui::Scalar(20)+l;
					glVertex(calcFovPoint(h,t,f));
					glVertex(calcFovPoint(h,t,n));
					}
				glEnd();
				
				glPopMatrix();
				}
			}
		
		/* Return to physical coordinates: */
		glPopMatrix();
		
		/* Reset OpenGL state: */
		glPopAttrib();
		}
	}

void RoomSetup::initContext(GLContextData& contextData) const
	{
	/* Create a context data item and associate it with the OpenGL context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Create the floor texture: */
	glBindTexture(GL_TEXTURE_2D,dataItem->floorTextureId);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,5);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
	
	GLColor<GLubyte,4>* tex=new GLColor<GLubyte,4>[512*512];
	for(int x=0;x<512;++x)
		{
		tex[0*512+x]=GLColor<GLubyte,4>(128,128,128);
		}
	for(int y=1;y<512;++y)
		{
		tex[y*512+0]=GLColor<GLubyte,4>(128,128,128);
		for(int x=1;x<512;++x)
			tex[y*512+x]=GLColor<GLubyte,4>(32,32,32);
		}
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,512,512,0,GL_RGBA,GL_UNSIGNED_BYTE,tex[0].getRgba());
	delete[] tex;
	
	/* Generate a mipmap: */
	if(GLEXTFramebufferObject::isSupported())
		{
		/* Initialize the framebuffer extension: */
		GLEXTFramebufferObject::initExtension();
		
		/* Auto-generate all requested mipmap levels: */
		glGenerateMipmapEXT(GL_TEXTURE_2D);
		}
	
	glBindTexture(GL_TEXTURE_2D,0);
	}

VRUI_APPLICATION_RUN(RoomSetup)
