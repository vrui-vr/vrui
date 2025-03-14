/***********************************************************************
CAVERenderer - Vislet class to render the default KeckCAVES backround
image seamlessly inside a VR application.
Copyright (c) 2005-2021 Oliver Kreylos

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

#include <Vrui/Vislets/CAVERenderer.h>

#include <string.h>
#include <stdlib.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <IO/OpenFile.h>
#include <Geometry/Plane.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLVertex.h>
#include <GL/GLMatrixTemplates.h>
#include <GL/GLLight.h>
#include <GL/GLValueCoders.h>
#include <GL/GLClipPlaneTracker.h>
#include <GL/GLContextData.h>
#include <GL/GLTransformationWrappers.h>
#include <Images/ReadImageFile.h>
#include <SceneGraph/GLRenderState.h>
#include <Vrui/LightsourceManager.h>
#include <Vrui/SceneGraphManager.h>
#include <Vrui/Viewer.h>
#include <Vrui/VisletManager.h>
#include <Vrui/Vrui.h>
#include <Vrui/Internal/Config.h>

namespace Vrui {

namespace Vislets {

namespace {

/****************
Helper functions:
****************/

inline double clampAngle(double angle)
	{
	if(angle<0.0)
		return 0.0;
	else if(angle>180.0)
		return 180.0;
	else
		return angle;
	}

}

/************************************
Methods of class CAVERendererFactory:
************************************/

CAVERendererFactory::CAVERendererFactory(VisletManager& visletManager)
	:VisletFactory("CAVERenderer",visletManager),
	 alignToEnvironment(true),
	 surfaceMaterial(GLMaterial::Color(1.0f,1.0f,1.0f),GLMaterial::Color(0.0f,0.0f,0.0f),0.0f),
	 tilesPerFoot(12),
	 wallTextureFileName("KeckCAVESWall.png"),
	 floorTextureFileName("KeckCAVESFloor.png")
	{
	#if 0
	/* Insert class into class hierarchy: */
	VisletFactory* visletFactory=visletManager.loadClass("Vislet");
	visletFactory->addChildClass(this);
	addParentClass(visletFactory);
	#endif
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=visletManager.getVisletClassSection(getClassName());
	cfs.updateValue("./alignToEnvironment",alignToEnvironment);
	cfs.updateValue("./surfaceMaterial",surfaceMaterial);
	cfs.updateValue("./tilesPerFoot",tilesPerFoot);
	cfs.updateString("./wallTextureFileName",wallTextureFileName);
	cfs.updateString("./floorTextureFileName",floorTextureFileName);
	
	/* Set tool class' factory pointer: */
	CAVERenderer::factory=this;
	}

CAVERendererFactory::~CAVERendererFactory(void)
	{
	/* Reset tool class' factory pointer: */
	CAVERenderer::factory=0;
	}

Vislet* CAVERendererFactory::createVislet(int numArguments,const char* const arguments[]) const
	{
	return new CAVERenderer(numArguments,arguments);
	}

void CAVERendererFactory::destroyVislet(Vislet* vislet) const
	{
	delete vislet;
	}

extern "C" void resolveCAVERendererDependencies(Plugins::FactoryManager<VisletFactory>& manager)
	{
	#if 0
	/* Load base classes: */
	manager.loadClass("Vislet");
	#endif
	}

extern "C" VisletFactory* createCAVERendererFactory(Plugins::FactoryManager<VisletFactory>& manager)
	{
	/* Get pointer to vislet manager: */
	VisletManager* visletManager=static_cast<VisletManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	CAVERendererFactory* caveRendererFactory=new CAVERendererFactory(*visletManager);
	
	/* Return factory object: */
	return caveRendererFactory;
	}

extern "C" void destroyCAVERendererFactory(VisletFactory* factory)
	{
	delete factory;
	}

/***************************************
Methods of class CAVERenderer::DataItem:
***************************************/

CAVERenderer::DataItem::DataItem(void)
	:wallTextureObjectId(0),
	 floorTextureObjectId(0),
	 screenDisplayListId(glGenLists(1))
	{
	glGenTextures(1,&wallTextureObjectId);
	glGenTextures(1,&floorTextureObjectId);
	}

CAVERenderer::DataItem::~DataItem(void)
	{
	glDeleteTextures(1,&wallTextureObjectId);
	glDeleteTextures(1,&floorTextureObjectId);
	glDeleteLists(screenDisplayListId,1);
	}

/*************************************
Static elements of class CAVERenderer:
*************************************/

CAVERendererFactory* CAVERenderer::factory=0;
const char* CAVERenderer::className="Vrui::CAVERenderer";

/*****************************
Methods of class CAVERenderer:
*****************************/

void CAVERenderer::renderScreen(void) const
	{
	typedef GLVertex<GLfloat,2,void,0,GLfloat,GLfloat,3> Vertex;
	
	/* Set up the screen rendering parameters: */
	float tileSize=12.0f/float(tilesPerFoot);
	int numTilesX=10*tilesPerFoot;
	int numTilesY=8*tilesPerFoot;
	
	/* Render the screen as a series of textured quad strips: */
	Vertex v1,v2;
	v1.normal=v2.normal=Vertex::Normal(0.0f,0.0f,1.0f);
	v1.position[2]=v2.position[2]=0.0f;
	for(int y=0;y<numTilesY;++y)
		{
		v1.texCoord[1]=float(y)/float(numTilesY);
		v1.position[1]=float(y)*tileSize;
		v2.texCoord[1]=float(y+1)/float(numTilesY);
		v2.position[1]=float(y+1)*tileSize;
		glBegin(GL_QUAD_STRIP);
		for(int x=0;x<=numTilesX;++x)
			{
			v1.texCoord[0]=v2.texCoord[0]=float(x)/float(numTilesX);
			v1.position[0]=v2.position[0]=float(x)*tileSize;
			glVertex(v2);
			glVertex(v1);
			}
		glEnd();
		}
	}

CAVERenderer::CAVERenderer(int numArguments,const char* const arguments[])
	:GLObject(false),
	 surfaceMaterial(factory->surfaceMaterial),
	 tilesPerFoot(factory->tilesPerFoot),
	 numViewers(getNumViewers()),
	 viewerHeadlightStates(0),
	 angle(720.0),angleAnimStep(0.0),lastFrame(0.0)
	{
	/* Reference this object to prevent garbage collection: */
	ref();
	
	/* Parse the command line: */
	std::string wallTextureFileName=factory->wallTextureFileName;
	std::string floorTextureFileName=factory->floorTextureFileName;
	bool alignToEnvironment=factory->alignToEnvironment;
	for(int i=0;i<numArguments;++i)
		{
		if(arguments[i][0]=='-')
			{
			if(strcasecmp(arguments[i]+1,"wall")==0)
				{
				++i;
				wallTextureFileName=arguments[i];
				}
			else if(strcasecmp(arguments[i]+1,"floor")==0)
				{
				++i;
				floorTextureFileName=arguments[i];
				}
			else if(strcasecmp(arguments[i]+1,"tilesPerFoot")==0)
				{
				++i;
				tilesPerFoot=atoi(arguments[i]);
				}
			else if(strcasecmp(arguments[i]+1,"noAlign")==0)
				alignToEnvironment=false;
			}
		}
	
	if(alignToEnvironment)
		{
		/* Calculate a transformation to align the CAVE model with the local environment: */
		const Vector& normal=getFloorPlane().getNormal();
		const Vector& up=getUpDirection();
		Scalar lambda=(getFloorPlane().getOffset()-getDisplayCenter()*normal)/(up*normal);
		Point floorDisplayCenter=getDisplayCenter()+up*lambda;
		caveTransform=OGTransform::translateFromOriginTo(floorDisplayCenter);
		
		Vector floorForward=Geometry::normalize(getFloorPlane().project(getForwardDirection()));
		Vector floorRight=Geometry::normalize(Geometry::cross(floorForward,normal));
		Rotation rot=Rotation::fromBaseVectors(floorRight,floorForward);
		caveTransform*=OGTransform::rotate(rot);
		
		caveTransform*=OGTransform::scale(getInchFactor());
		}
	else
		caveTransform=OGTransform::identity;
	
	/* Load the texture images: */
	IO::DirectoryPtr textureDir=IO::openDirectory(VRUI_INTERNAL_CONFIG_SHAREDIR "/Textures");
	wallTextureImage=Images::readGenericImageFile(*textureDir,wallTextureFileName.c_str());
	floorTextureImage=Images::readGenericImageFile(*textureDir,floorTextureFileName.c_str());
	
	/* Create static ceiling light sources in the CAVE room: */
	GLLight::Color lightColor(0.25f,0.25f,0.25f);
	for(int i=0;i<4;++i)
		{
		Point pos(30,30,96);
		for(int j=0;j<2;++j)
			if(i&(0x1<<j))
				pos[j]=-pos[j];
		pos=caveTransform.transform(pos);
		lightsources[i]=getLightsourceManager()->createLightsource(true,GLLight(lightColor,GLLight::Position(float(pos[0]),float(pos[1]),float(pos[2]),1.0f)));
		}
	
	/* Set the scene graph node's pass mask: */
	passMask=SceneGraph::GraphNode::GLRenderPass;
	
	GLObject::init();
	}

CAVERenderer::~CAVERenderer(void)
	{
	delete[] viewerHeadlightStates;
	
	/* Destroy static ceiling light sources: */
	for(int i=0;i<4;++i)
		getLightsourceManager()->destroyLightsource(lightsources[i]);
	}

VisletFactory* CAVERenderer::getFactory(void) const
	{
	return factory;
	}

void CAVERenderer::enable(bool startup)
	{
	/* Enable the static ceiling light sources: */
	for(int i=0;i<4;++i)
		lightsources[i]->enable();
	
	/* Save all viewers' head light states and then turn them off: */
	viewerHeadlightStates=new bool[getNumViewers()];
	for(int i=0;i<getNumViewers()&&i<numViewers;++i)
		{
		viewerHeadlightStates[i]=getViewer(i)->getHeadlight().isEnabled();
		getViewer(i)->setHeadlightState(false);
		}
	
	/* Enable the vislet as far as the vislet manager is concerned: */
	Vislet::enable(startup);
	
	if(!startup)
		{
		/* Trigger the unfolding animation: */
		angleAnimStep=90.0;
		lastFrame=getApplicationTime();
		
		/* Request another frame: */
		scheduleUpdate(getNextAnimationTime());
		}
	
	/* Add this object to the central scene graph: */
	getSceneGraphManager()->addPhysicalNode(*this);
	}

void CAVERenderer::disable(bool shutdown)
	{
	if(!shutdown)
		{
		/* Trigger the folding animation: */
		angleAnimStep=-90.0;
		lastFrame=getApplicationTime();
			
		/* Request another frame: */
		scheduleUpdate(getNextAnimationTime());
		
		/* Frame function will disable vislet when animation is done */
		}
	else
		{
		/* Disable the vislet for reals: */
		Vislet::disable(shutdown);
		
		/* Remove this object from the central scene graph: */
		getSceneGraphManager()->removePhysicalNode(*this);
		}
	}

void CAVERenderer::frame(void)
	{
	if(angleAnimStep!=0.0)
		{
		/* Get the time since the last frame: */
		double delta=getApplicationTime()-lastFrame;
		lastFrame=getApplicationTime();
		
		/* Update the angle value: */
		angle+=angleAnimStep*delta;
		if(angle<0.0)
			{
			angle=0.0;
			angleAnimStep=0.0;
			
			/* Disable the static ceiling light sources: */
			for(int i=0;i<4;++i)
				lightsources[i]->disable();
			
			/* Set all viewers' head lights to the saved state: */
			for(int i=0;i<getNumViewers()&&i<numViewers;++i)
				getViewer(i)->setHeadlightState(viewerHeadlightStates[i]);
			delete[] viewerHeadlightStates;
			viewerHeadlightStates=0;
			
			/* Disable the vislet: */
			Vislet::disable(false);
			
			/* Remove this object from the central scene graph: */
			getSceneGraphManager()->removePhysicalNode(*this);
			}
		else if(angle>720.0)
			{
			angle=720.0;
			angleAnimStep=0.0;
			}
		else
			{
			/* Request another frame: */
			scheduleUpdate(getNextAnimationTime());
			}
		}
	}

const char* CAVERenderer::getClassName(void) const
	{
	return className;
	}

SceneGraph::Box CAVERenderer::calcBoundingBox(void) const
	{
	/* Return the static bounding box of the CAVE when not under animation: */
	return SceneGraph::Box(SceneGraph::Point(-60,-36,0),SceneGraph::Point(60,60,96));
	}

void CAVERenderer::glRenderAction(SceneGraph::GLRenderState& renderState) const
	{
	/* Get a pointer to the data item: */
	DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
	
	/* Set up OpenGL to render the walls and floor: */
	renderState.enableMaterials();
	renderState.enableTexture2D();
	renderState.setFrontFace(GL_CCW);
	renderState.enableCulling(GL_BACK);
	
	/* Set up the rendering mode for the CAVE room: */
	glMaterial(GLMaterialEnums::FRONT,surfaceMaterial);
	glColor(surfaceMaterial.diffuse);
	
	/* Install and save the modelview matrix: */
	renderState.uploadModelview();
	glPushMatrix();
	
	/* Go to base CAVE coordinates: */
	glMultMatrix(caveTransform);
	
	/* Bind the floor texture: */
	renderState.bindTexture2D(dataItem->floorTextureObjectId);
	
	/* Render the floor: */
	glTranslate(-60.0,-36.0,0.0);
	glRotate(clampAngle(angle)-180.0,1.0,0.0,0.0);
	glCallList(dataItem->screenDisplayListId);
	
	/* Bind the wall texture: */
	renderState.bindTexture2D(dataItem->wallTextureObjectId);
	
	/* Render the left wall: */
	glTranslate(0.0,-24.0,0.0);
	glRotate(90.0,0.0,0.0,1.0);
	glRotate(clampAngle(angle-180.0)-90.0,1.0,0.0,0.0);
	renderState.bindTexture2D(dataItem->wallTextureObjectId);
	glCallList(dataItem->screenDisplayListId);
	
	/* Render the back wall: */
	glTranslate(120.0,0.0,0.0);
	glRotate(90.0-clampAngle(angle-360.0),0.0,1.0,0.0);
	glCallList(dataItem->screenDisplayListId);
	
	/* Render the right wall: */
	glTranslate(120.0,0.0,0.0);
	glRotate(90.0-clampAngle(angle-540.0),0.0,1.0,0.0);
	glCallList(dataItem->screenDisplayListId);
	
	/* Restore the modelview matrix: */
	glPopMatrix();
	}

void CAVERenderer::initContext(GLContextData& contextData) const
	{
	/* Create the context data item: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Upload the wall texture image: */
	glBindTexture(GL_TEXTURE_2D,dataItem->wallTextureObjectId);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	wallTextureImage.glTexImage2DMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,0);
	
	/* Upload the floor texture image: */
	glBindTexture(GL_TEXTURE_2D,dataItem->floorTextureObjectId);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	floorTextureImage.glTexImage2DMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,0);
	
	/* Create the screen display list: */
	glNewList(dataItem->screenDisplayListId,GL_COMPILE);
	renderScreen();
	glEndList();
	}

}

}
