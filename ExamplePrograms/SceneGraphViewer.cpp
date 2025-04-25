/***********************************************************************
SceneGraphViewer - Viewer for one or more scene graphs loaded from VRML
2.0 or binary scene graph files.
Copyright (c) 2010-2025 Oliver Kreylos

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

#include "SceneGraphViewer.h"

#include <stdio.h>
#include <vector>
#include <Misc/FileNameExtensions.h>
#include <Misc/FunctionCalls.h>
#include <Misc/MessageLogger.h>
#include <IO/File.h>
#include <IO/OpenFile.h>
#include <Math/Math.h>
#include <Geometry/ComponentArray.h>
#include <Geometry/Point.h>
#include <Geometry/Box.h>
#include <Geometry/Plane.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/Button.h>
#include <SceneGraph/GraphNode.h>
#include <SceneGraph/SphereCollisionQuery.h>
#include <Vrui/Vrui.h>
#include <Vrui/SceneGraphManager.h>
#include <Vrui/Viewer.h>

#include "SceneGraphViewerWalkNavigationTool.h"
#include "SceneGraphViewerTransformTool.h"
#include "SceneGraphViewerSurfaceTouchTransformTool.h"

// DEBUGGING
#include <iostream>
#include <Realtime/Time.h>
#include <Geometry/OutputOperators.h>

/*********************************
Methods of class SceneGraphViewer:
*********************************/

void SceneGraphViewer::goToPhysicalSpaceCallback(Misc::CallbackData* cbData)
	{
	Vrui::setNavigationTransformation(Vrui::NavTransform::identity);
	}

void SceneGraphViewer::showPhysicalSceneGraphListCallback(Misc::CallbackData* cbData)
	{
	/* Create and show the list dialog: */
	GLMotif::PopupWindow* list=physicalSceneGraphs.createSceneGraphDialog(*Vrui::getWidgetManager(),"Physical-Space Scene Graphs");
	Vrui::popupPrimaryWidget(list);
	}

void SceneGraphViewer::showNavigationalSceneGraphListCallback(Misc::CallbackData* cbData)
	{
	/* Create and show the list dialog: */
	GLMotif::PopupWindow* list=navigationalSceneGraphs.createSceneGraphDialog(*Vrui::getWidgetManager(),"Navigational-Space Scene Graphs");
	Vrui::popupPrimaryWidget(list);
	}

void SceneGraphViewer::alignSurfaceFrame(Vrui::SurfaceNavigationTool::AlignmentData& alignmentData)
	{
	/* Retrieve the user's current pose in physical space: */
	Vrui::Point headPosPhys=Vrui::getMainViewer()->getHeadPosition();
	Vrui::Point footPosPhys=Vrui::calcFloorPoint(headPosPhys);
	Vrui::Scalar heightPhys=Geometry::dist(headPosPhys,footPosPhys);
	
	/* Calculate the user's proposed pose in navigational space: */
	Point footPos=alignmentData.surfaceFrame.getOrigin();
	Vector up=alignmentData.surfaceFrame.getRotation().getDirection(2);
	Scalar height=heightPhys*alignmentData.surfaceFrame.getScaling();
	
	// DEBUGGING
	// std::cout<<"Foot: "<<footPos<<", head: "<<headPos<<std::endl;
	// std::cout<<"Up vector: "<<up<<", height: "<<height<<std::endl;
	// std::cout<<"Proposed move: "<<alignmentData.prevSurfaceFrame.getOrigin()<<" to "<<footPos<<std::endl;
	
	/* Retrieve alignment parameters: */
	Scalar ps(alignmentData.probeSize);
	Vector psvec=up*ps;
	Scalar mc(alignmentData.maxClimb);
	
	/* Check whether this is a continuing navigation sequence: */
	AlignmentState* as=static_cast<AlignmentState*>(alignmentData.alignmentState);
	if(as!=0)
		{
		/* Retrieve the user's previous head and foot positions: */
		Point prevFootPos=alignmentData.prevSurfaceFrame.getOrigin();
		Point prevHeadPos=prevFootPos+up*as->height;
		
		/* Calculate the foot's movement vector: */
		Vector moveVec=footPos-prevFootPos;
		
		/* Raise the user's foot to step over low obstacles: */
		prevFootPos+=up*(ps+Math::min(mc,Math::div2(as->height))); // Step by the tool's max climb, but not more than half the avatar's height
		
		/* Trace the head and foot from the previous to the intended positions along a common movement vector: */
		Point headPos;
		if(moveVec!=Vector::zero)
			{
			SceneGraph::SphereCollisionQuery headQuery(prevHeadPos,moveVec,ps);
			Vrui::getSceneGraphManager()->testNavigationalCollision(headQuery);
			SceneGraph::SphereCollisionQuery footQuery(prevFootPos,moveVec,ps);
			Vrui::getSceneGraphManager()->testNavigationalCollision(footQuery);
			
			/* Check if there was a collision: */
			if(headQuery.isHit()||footQuery.isHit())
				{
				/* Move head and foot to the collision position and get the collision plane normal: */
				Vector step,collisionNormal;
				if(headQuery.getHitLambda()<=footQuery.getHitLambda())
					{
					step=moveVec*headQuery.getHitLambda();
					collisionNormal=headQuery.getHitNormal();
					}
				else
					{
					step=moveVec*footQuery.getHitLambda();
					collisionNormal=footQuery.getHitNormal();
					}
				headPos=prevHeadPos+step;
				footPos=prevFootPos+step;
				
				/* Align the remaining movement vector with the collision plane: */
				Vector n1=up;
				n1.orthogonalize(moveVec);
				if(n1!=Vector::zero)
					{
					/* Find a sliding vector that is in both the collision plane and the current movement plane: */
					Vector slide=n1^collisionNormal;
					Vector moveVec2=slide*(((moveVec-step)*slide)/slide.sqr());
					
					/* Add a fudge factor away from the collision plane to avoid numerical errors: */
					moveVec2.addScaled(collisionNormal,Scalar(1.0e-5)/collisionNormal.mag());
					
					/* Trace the head and foot again: */
					SceneGraph::SphereCollisionQuery headQuery2(headPos,moveVec2,ps);
					Vrui::getSceneGraphManager()->testNavigationalCollision(headQuery2);
					SceneGraph::SphereCollisionQuery footQuery2(footPos,moveVec2,ps);
					Vrui::getSceneGraphManager()->testNavigationalCollision(footQuery2);
					Scalar stepLambda=Math::min(headQuery2.getHitLambda(),footQuery2.getHitLambda());
					step=moveVec2*stepLambda;
					headPos+=step;
					footPos+=step;
					
					// DEBUGGING
					// std::cout<<collisionNormal*moveVec2<<", "<<stepLambda<<std::endl;
					}
				}
			else
				{
				headPos=prevHeadPos+moveVec;
				footPos=prevFootPos+moveVec;
				}
			}
		else
			{
			headPos=prevHeadPos;
			footPos=prevFootPos;
			}
		
		/* Drop the foot down to extend the avatar to its new height and a maxClimb beyond to test for falling: */
		Point dropFootPos=Geometry::subtractScaled(headPos,up,height+mc-ps);
		SceneGraph::SphereCollisionQuery footDropQuery(footPos,dropFootPos-footPos,ps);
		Vrui::getSceneGraphManager()->testNavigationalCollision(footDropQuery);
		
		/* Check if the avatar could not be extended to its full height: */
		Scalar extendHeight=(height+mc-ps)*footDropQuery.getHitLambda()+ps;
		if(extendHeight<height)
			{
			footPos=footDropQuery.getHitPoint();
			
			#if 0
			
			/* Check if the foot landed on a surface that is too steep to stand: */
			const Vector& hitNormal=footDropQuery.getHitNormal();
			if(hitNormal*up<Math::cos(Math::rad(Scalar(45)))*hitNormal.mag())
				{
				/* Find a slide vector that is in the plane of the current movement, to avoid having to re-trace the head position: */
				Vector slideVec=(moveVec^up)^hitNormal;
				Scalar svup=slideVec*up;
				if(svup<Scalar(0))
					{
					// DEBUGGING
					// std::cout<<"Sliding!"<<std::endl;
					
					/* Slide back until the avatar is fully extended or the current motion is undone: */
					Vector horMoveVec=moveVec;
					horMoveVec.orthogonalize(up);
					Scalar slideLambda=Math::min((extendHeight-height)/svup,-((footPos-prevHeadPos)*horMoveVec)/(slideVec*horMoveVec));
					SceneGraph::SphereCollisionQuery footSlideQuery(footPos,slideVec*slideLambda,ps);
					Vrui::getSceneGraphManager()->testNavigationalCollision(footSlideQuery);
					footPos=footSlideQuery.getHitPoint();
					}
				}
			
			#endif
			}
		else
			footPos=dropFootPos;
		
		// DEBUGGING
		// Vector step=footPos-prevFootPos;
		// step[2]=Scalar(0);
		// moveVec[2]=Scalar(0);
		// std::cout<<moveVec.mag()<<", "<<step.mag()<<std::endl;
		}
	else
		{
		/* Initialize a new navigation sequence: */
		alignmentData.alignmentState=as=new AlignmentState;
		as->floorLift=Scalar(0);
		
		/* Drop down from the proposed foot position to let the navigation tool implement falling: */
		footPos+=psvec;
		SceneGraph::SphereCollisionQuery footDropQuery(footPos,-up*mc,ps);
		Vrui::getSceneGraphManager()->testNavigationalCollision(footDropQuery);
		footPos=footDropQuery.getHitPoint();
		}
	
	/* Position the proposed frame at the final foot position: */
	alignmentData.surfaceFrame=Vrui::NavTransform(Vrui::Point(footPos-psvec)-Vrui::Point::origin,alignmentData.surfaceFrame.getRotation(),alignmentData.surfaceFrame.getScaling());
	
	/* Update the alignment state: */
	as->height=height;
	}

SceneGraphViewer::SceneGraphViewer(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 physicalSceneGraphs(Vrui::getSceneGraphManager()->getPhysicalRoot(),*IO::Directory::getCurrent()),
	 navigationalSceneGraphs(Vrui::getSceneGraphManager()->getNavigationalRoot(),*IO::Directory::getCurrent()),
	 mainMenu(0)
	{
	// DEBUGGING
	// Realtime::TimePointMonotonic loadTimer;
	
	/* Keep track if any of the loaded scene graphs require an audio processing pass: */
	bool requireAudio=false;
	
	/* Parse the command line: */
	SceneGraph::SceneGraphList* currentList=&navigationalSceneGraphs;
	bool enable=true;
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i]+1,"navigational")==0||strcasecmp(argv[i]+1,"n")==0)
				currentList=&navigationalSceneGraphs;
			else if(strcasecmp(argv[i]+1,"physical")==0||strcasecmp(argv[i]+1,"p")==0)
				currentList=&physicalSceneGraphs;
			else if(strcasecmp(argv[i]+1,"enable")==0||strcasecmp(argv[i]+1,"e")==0)
				enable=true;
			else if(strcasecmp(argv[i]+1,"disable")==0||strcasecmp(argv[i]+1,"d")==0)
				enable=false;
			}
		else
			{
			/* Try loading the scene graph: */
			try
				{
				SceneGraph::GraphNodePointer sceneGraph=currentList->addSceneGraph(argv[i],enable);
				
				/* Check if the scene graph requires an audio pass: */
				requireAudio=requireAudio||(sceneGraph->getPassMask()&SceneGraph::GraphNode::ALRenderPass)!=0x0U;
				}
			catch(const std::runtime_error& err)
				{
				/* Print an error message and keep going: */
				Misc::formattedUserWarning("Scene Graph Viewer: Ignoring file %s due to exception %s",argv[i],err.what());
				}
			}
		}
	
	// DEBUGGING
	// double loadTime(loadTimer.setAndDiff());
	// std::cout<<"Time to load all scene graphs: "<<loadTime*1000.0<<" ms"<<std::endl;
	
	/* Request audio processing if needed: */
	if(requireAudio)
		Vrui::requestSound();
	
	/* Create the main menu shell: */
	mainMenu=new GLMotif::PopupMenu("MainMenu",Vrui::getWidgetManager());
	mainMenu->setTitle("Scene Graph Viewer");
	
	/* Add a button to go to physical space: */
	GLMotif::Button* goToPhysicalSpaceButton=new GLMotif::Button("GoToPhysicalSpaceButton",mainMenu,"Go To Physical Space");
	goToPhysicalSpaceButton->getSelectCallbacks().add(this,&SceneGraphViewer::goToPhysicalSpaceCallback);
	
	/* Add toggles to show the physical or navigational scene graph list dialogs: */
	GLMotif::Button* showPhysicalSceneGraphListButton=new GLMotif::Button("ShowPhysicalSceneGraphListButton",mainMenu,"Show Physical-Space Scene Graphs");
	showPhysicalSceneGraphListButton->getSelectCallbacks().add(this,&SceneGraphViewer::showPhysicalSceneGraphListCallback);
	
	GLMotif::Button* showNavigationalSceneGraphListButton=new GLMotif::Button("ShowNavigationalSceneGraphListButton",mainMenu,"Show Navigational-Space Scene Graphs");
	showNavigationalSceneGraphListButton->getSelectCallbacks().add(this,&SceneGraphViewer::showNavigationalSceneGraphListCallback);
	
	/* Finish and install the main menu: */
	mainMenu->manageMenu();
	Vrui::setMainMenu(mainMenu);
	
	/* Initialize the custom tool classes: */
	WalkNavigationToolFactory* walkNavigationToolFactory=new WalkNavigationToolFactory(*Vrui::getToolManager());
	Vrui::getToolManager()->addClass(walkNavigationToolFactory,Vrui::ToolManager::defaultToolFactoryDestructor);
	TransformToolFactory* transformToolFactory=new TransformToolFactory(*Vrui::getToolManager());
	Vrui::getToolManager()->addClass(transformToolFactory,Vrui::ToolManager::defaultToolFactoryDestructor);
	SurfaceTouchTransformToolFactory* surfaceTouchTransformToolFactory=new SurfaceTouchTransformToolFactory(*Vrui::getToolManager());
	Vrui::getToolManager()->addClass(surfaceTouchTransformToolFactory,Vrui::ToolManager::defaultToolFactoryDestructor);
	}

SceneGraphViewer::~SceneGraphViewer(void)
	{
	delete mainMenu;
	}

void SceneGraphViewer::toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData* cbData)
	{
	/* Let the base class at it first: */
	Vrui::Application::toolCreationCallback(cbData);
	
	/* Check if the new tool is a surface navigation tool: */
	Vrui::SurfaceNavigationTool* surfaceNavigationTool=dynamic_cast<Vrui::SurfaceNavigationTool*>(cbData->tool);
	if(surfaceNavigationTool!=0)
		{
		/* Set the new tool's alignment function: */
		surfaceNavigationTool->setAlignFunction(Misc::createFunctionCall(this,&SceneGraphViewer::alignSurfaceFrame));
		}
	}

void SceneGraphViewer::display(GLContextData& contextData) const
	{
	/* Actually, got nothing to do! */
	}

void SceneGraphViewer::resetNavigation(void)
	{
	/* Calculate the bounding box of all enabled navigational-space scene graphs: */
	SceneGraph::Box bbox=Vrui::getSceneGraphManager()->calcNavigationalBoundingBox();
	
	/* Show the entire bounding box: */
	Vrui::setNavigationTransformation(Geometry::mid(bbox.min,bbox.max),Math::div2(Geometry::dist(bbox.min,bbox.max)),Vrui::Vector(0,1,0));
	}

VRUI_APPLICATION_RUN(SceneGraphViewer)
