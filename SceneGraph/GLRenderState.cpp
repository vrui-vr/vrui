/***********************************************************************
GLRenderState - Class encapsulating the traversal state of a scene graph
during OpenGL rendering.
Copyright (c) 2009-2025 Oliver Kreylos

This file is part of the Simple Scene Graph Renderer (SceneGraph).

The Simple Scene Graph Renderer is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Simple Scene Graph Renderer is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Simple Scene Graph Renderer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <SceneGraph/GLRenderState.h>

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Box.h>
#include <Geometry/Matrix.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLTexEnvTemplates.h>
#include <GL/GLMatrixTemplates.h>
#include <GL/GLVertexArrayParts.h>
#include <GL/GLTransformationWrappers.h>
#include <SceneGraph/GraphNode.h>

namespace SceneGraph {

/******************************
Methods of class GLRenderState:
******************************/

void GLRenderState::calcFrustum(void)
	{
	/* Calculate points on the six frustum planes in eye space: */
	DPTransform invP=Geometry::invert(projection);
	for(int i=0;i<6;++i)
		{
		DPoint ccv=DPoint::origin;
		ccv[i/2]=(i&0x1)!=0x0?1.0:-1.0;
		frustumPoints[i]=invP.transform(ccv);
		}
	
	/* Calculate normalized normal vectors for the six frustum planes in eye space: */
	DPTransform transP;
	for(int i=0;i<4;++i)
		for(int j=0;j<4;++j)
			transP.getMatrix()(i,j)=projection.getMatrix()(j,i);
	for(int i=0;i<6;++i)
		{
		DPTransform::HVector ccv=DPTransform::HVector::origin;
		ccv[i/2]=(i&0x1)!=0x0?-1.0:1.0;
		frustumNormals[i]=transP.transform(ccv).toVector().normalize();
		}
	}

void GLRenderState::loadCurrentTransform(void)
	{
	/* Write the transformation into a 4x4 matrix: */
	Geometry::Matrix<double,4,4> matrix(1.0);
	currentTransform.writeMatrix(matrix);
	
	/* Flip the matrix to column-major order: */
	double temp[4*4];
	double* tPtr=temp;
	for(int j=0;j<4;++j)
		for(int i=0;i<4;++i)
			*(tPtr++)=matrix(i,j);
	
	/* Set OpenGL's matrix mode to modelview and upload the new modelview matrix: */
	if(currentState.matrixMode!=GL_MODELVIEW)
		glMatrixMode(GL_MODELVIEW);
	currentState.matrixMode=GL_MODELVIEW;
	glLoadMatrix(temp);
	
	/* Mark OpenGL's modelview matrix as up-to-date: */
	modelviewOutdated=false;
	}

void GLRenderState::changeVertexArraysMask(int currentMask,int newMask)
	{
	/* Enable those vertex arrays that are active in the new state but not in the current state: */
	int onMask=newMask&(~currentMask);
	GLVertexArrayParts::enable(onMask);
	
	/* Disable those vertex arrays that are active in the current state but not in the new state: */
	int offMask=currentMask&(~newMask);
	GLVertexArrayParts::disable(offMask);
	}

GLRenderState::GLRenderState(GLContextData& sContextData,const Point& sBaseEyePos,const GLRenderState::Rect& sViewport,const GLRenderState::DPTransform& sProjection,const DOGTransform& initialTransform,const Point& sBaseViewerPos,const Vector& sBaseUpVector)
	:contextData(sContextData),
	 baseEyePos(sBaseEyePos),
	 viewport(sViewport),
	 projection(sProjection),
	 modelviewOutdated(true),
	 haveTextureTransform(false)
	{
	/* Update the viewer position, up vector, and initial model transformation: */
	TraversalState::startTraversal(initialTransform,sBaseViewerPos,sBaseUpVector);
	
	/* Calculate the view frustum's planes in eye space: */
	calcFrustum();
	
	/* Initialize OpenGL state tracking elements: */
	GLint tempFrontFace;
	glGetIntegerv(GL_FRONT_FACE,&tempFrontFace);
	initialState.frontFace=tempFrontFace;
	initialState.cullingEnabled=glIsEnabled(GL_CULL_FACE);
	GLint tempCulledFace;
	glGetIntegerv(GL_CULL_FACE_MODE,&tempCulledFace);
	initialState.culledFace=tempCulledFace;
	initialState.lightingEnabled=glIsEnabled(GL_LIGHTING);
	initialState.normalizeEnabled=glIsEnabled(GL_NORMALIZE);
	GLint tempLightModelTwoSide;
	glGetIntegerv(GL_LIGHT_MODEL_TWO_SIDE,&tempLightModelTwoSide);
	initialState.lightModelTwoSide=tempLightModelTwoSide;
	initialState.emissiveColor=Color(0.0f,0.0f,0.0f);
	initialState.colorMaterialEnabled=glIsEnabled(GL_COLOR_MATERIAL);
	
	initialState.highestTexturePriority=-1;
	if(glIsEnabled(GL_TEXTURE_1D))
		initialState.highestTexturePriority=0;
	if(glIsEnabled(GL_TEXTURE_2D))
		initialState.highestTexturePriority=1;
	if(glIsEnabled(GL_TEXTURE_3D_EXT))
		initialState.highestTexturePriority=2;
	if(glIsEnabled(GL_TEXTURE_CUBE_MAP_EXT))
		initialState.highestTexturePriority=3;
	initialState.boundTextures[3]=initialState.boundTextures[2]=initialState.boundTextures[1]=initialState.boundTextures[0]=0;
	
	GLint tempLightModelColorControl;
	glGetIntegerv(GL_LIGHT_MODEL_COLOR_CONTROL,&tempLightModelColorControl);
	initialState.lightModelColorControl=tempLightModelColorControl;
	
	/* Check what rendering pass we're currently in: */
	if(glIsEnabled(GL_BLEND))
		{
		/* It's the transparent rendering pass: */
		initialRenderPass=GraphNode::GLTransparentRenderPass;
		
		/* Set the default alpha blending function: */
		initialState.blendSrcFactor=GL_SRC_ALPHA;
		initialState.blendDstFactor=GL_ONE_MINUS_SRC_ALPHA;
		glBlendFunc(initialState.blendSrcFactor,initialState.blendDstFactor);
		}
	else
		{
		/* It's the opaque rendering pass: */
		initialRenderPass=GraphNode::GLRenderPass;
		
		/* Set the initial blending function parameters: */
		initialState.blendSrcFactor=GL_ONE;
		initialState.blendDstFactor=GL_ZERO;
		}
	currentRenderPass=initialRenderPass;
	
	/* Retrieve the initial OpenGL matrix mode: */
	GLint imm;
	glGetIntegerv(GL_MATRIX_MODE,&imm);
	initialState.matrixMode=imm;
	
	initialState.activeVertexArraysMask=0x0;
	initialState.vertexBuffer=0;
	initialState.indexBuffer=0;
	
	GLint programHandle;
	glGetIntegerv(GL_CURRENT_PROGRAM,&programHandle);
	initialState.shaderProgram=GLhandleARB(programHandle);
	
	/* Copy current state from initial state: */
	currentState=initialState;
	
	/* Set up initial combined OpenGL mode for scene graph rendering: */
	if(currentState.lightingEnabled)
		{
		if(!currentState.normalizeEnabled)
			glEnable(GL_NORMALIZE);
		currentState.normalizeEnabled=true;
		
		GLenum newLightModelTwoSide=currentState.cullingEnabled?GL_FALSE:GL_TRUE;
		if(currentState.lightModelTwoSide!=newLightModelTwoSide)
			glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,newLightModelTwoSide);
		currentState.lightModelTwoSide=newLightModelTwoSide;
		}
	}

namespace {

/****************
Helper functions:
****************/

void setGLState(GLenum flag,bool value)
	{
	/* Enable or disable the state component: */
	if(value)
		glEnable(flag);
	else
		glDisable(flag);
	}

}

GLRenderState::~GLRenderState(void)
	{
	/* Go back to the initial rendering pass: */
	setRenderPass(initialRenderPass);
	
	/* Reset OpenGL state: */
	resetState();
	}

void GLRenderState::startTraversal(const Point& newBaseEyePos,const GLRenderState::Rect& newViewport,const GLRenderState::DPTransform& newProjection,const DOGTransform& newCurrentTransform,const Point& newBaseViewerPos,const Vector& newBaseUpVector)
	{
	/* Call the base class method: */
	TraversalState::startTraversal(newCurrentTransform,newBaseViewerPos,newBaseUpVector);
	
	/* Copy the eye position and viewport: */
	baseEyePos=newBaseEyePos;
	viewport=newViewport;
	
	/* Store the projection matrix: */
	projection=newProjection;
	
	/* Calculate the view frustum's planes in eye space: */
	calcFrustum();
	
	/* Mark OpenGL's modelview matrix as outdated: */
	modelviewOutdated=true;
	}

GLRenderState::DPlane GLRenderState::getFrustumPlane(int planeIndex) const
	{
	/* Transform the plane's point with the inverse current transformation: */
	DPoint p=currentTransform.inverseTransform(frustumPoints[planeIndex]);
	
	/* Transform the plane's normal vector with the inverse current transformation's rotation only: */
	DVector n=currentTransform.getRotation().inverseTransform(frustumNormals[planeIndex]);
	
	/* Return a plane: */
	return DPlane(n,p);
	}

bool GLRenderState::doesBoxIntersectFrustum(const Box& box) const
	{
	/* Get the current transformation's direction axes in eye space: */
	DVector axis[3];
	for(int i=0;i<3;++i)
		axis[i]=currentTransform.getDirection(i);
	
	/* Check the box against each frustum plane in eye space: */
	for(int planeIndex=0;planeIndex<6;++planeIndex)
		{
		/* Find the point on the bounding box which is closest to the frustum plane: */
		DPoint p;
		for(int i=0;i<3;++i)
			p[i]=frustumNormals[planeIndex]*axis[i]>0.0?double(box.max[i]):double(box.min[i]);
		
		/* Check if the point is inside the view frustum: */
		if(frustumNormals[planeIndex]*(currentTransform.transform(p)-frustumPoints[planeIndex])<0.0)
			return false;
		}
	
	return true;
	}

void GLRenderState::setRenderPass(Misc::UInt32 newRenderPass)
	{
	/* Check if the rendering pass changed: */
	if(currentRenderPass!=newRenderPass)
		{
		/* Switch OpenGL state based on the given rendering pass: */
		if(newRenderPass==GraphNode::GLRenderPass)
			{
			/* Go back to standard opaque OpenGL rendering: */
			glDisable(GL_BLEND);
			glDepthMask(GL_TRUE);
			}
		else if(newRenderPass==GraphNode::GLTransparentRenderPass)
			{
			/* Go to transparent OpenGL rendering using standard alpha blending: */
			glEnable(GL_BLEND);
			currentState.blendSrcFactor=GL_SRC_ALPHA;
			currentState.blendDstFactor=GL_ONE_MINUS_SRC_ALPHA;
			glBlendFunc(currentState.blendSrcFactor,currentState.blendDstFactor);
			glDepthMask(GL_FALSE);
			}
		}
	currentRenderPass=newRenderPass;
	}

void GLRenderState::setTextureTransform(const GLRenderState::TextureTransform& newTextureTransform)
	{
	/* Set up the new texture transformation: */
	if(currentState.matrixMode!=GL_TEXTURE)
		glMatrixMode(GL_TEXTURE);
	currentState.matrixMode=GL_TEXTURE;
	
	/* Flip the texture transformation matrix to column-major order and upload it to OpenGL: */
	const TextureTransform::Matrix& matrix=newTextureTransform.getMatrix();
	Scalar temp[4*4];
	Scalar* tPtr=temp;
	for(int j=0;j<3;++j)
		{
		for(int i=0;i<3;++i)
			*(tPtr++)=matrix(i,j);
		*(tPtr++)=Scalar(0);
		}
	for(int i=0;i<3;++i)
		*(tPtr++)=matrix(i,3);
	*(tPtr++)=Scalar(1);
	glLoadMatrix(temp);
	
	/* Remember that we have a texture transformation: */
	haveTextureTransform=true;
	}

void GLRenderState::resetTextureTransform(void)
	{
	/* Reset the texture transformation: */
	if(currentState.matrixMode!=GL_TEXTURE)
		glMatrixMode(GL_TEXTURE);
	currentState.matrixMode=GL_TEXTURE;
	glLoadIdentity();
	
	/* Remember that we no longer have a texture transformation: */
	haveTextureTransform=false;
	}

void GLRenderState::resetState(void)
	{
	/* Unbind all bound texture objects: */
	if(currentState.boundTextures[0]!=0)
		glBindTexture(GL_TEXTURE_1D,0);
	currentState.boundTextures[0]=0;
	if(currentState.boundTextures[1]!=0)
		glBindTexture(GL_TEXTURE_2D,0);
	currentState.boundTextures[1]=0;
	if(currentState.boundTextures[2]!=0)
		glBindTexture(GL_TEXTURE_3D_EXT,0);
	currentState.boundTextures[2]=0;
	if(currentState.boundTextures[3]!=0)
		glBindTexture(GL_TEXTURE_CUBE_MAP_EXT,0);
	currentState.boundTextures[3]=0;
	
	/* Reset texture mapping: */
	if(initialState.highestTexturePriority<3&&currentState.highestTexturePriority>=3)
		glDisable(GL_TEXTURE_CUBE_MAP_EXT);
	if(initialState.highestTexturePriority<2&&currentState.highestTexturePriority>=2)
		glDisable(GL_TEXTURE_3D_EXT);
	if(initialState.highestTexturePriority<1&&currentState.highestTexturePriority>=1)
		glDisable(GL_TEXTURE_2D);
	if(initialState.highestTexturePriority<0&&currentState.highestTexturePriority>=0)
		glDisable(GL_TEXTURE_1D);
	if(initialState.highestTexturePriority==3&&currentState.highestTexturePriority<3)
		glEnable(GL_TEXTURE_CUBE_MAP_EXT);
	if(initialState.highestTexturePriority==2&&currentState.highestTexturePriority<2)
		glEnable(GL_TEXTURE_3D_EXT);
	if(initialState.highestTexturePriority==1&&currentState.highestTexturePriority<1)
		glEnable(GL_TEXTURE_2D);
	if(initialState.highestTexturePriority==0&&currentState.highestTexturePriority<0)
		glEnable(GL_TEXTURE_1D);
	currentState.highestTexturePriority=initialState.highestTexturePriority;
	
	if(haveTextureTransform)
		{
		/* Reset the texture transformation: */
		if(currentState.matrixMode!=GL_TEXTURE)
			{
			glMatrixMode(GL_TEXTURE);
			currentState.matrixMode=GL_TEXTURE;
			}
		glLoadIdentity();
		}
	
	/* Reset other state back to initial state: */
	if(currentState.frontFace!=initialState.frontFace)
		glFrontFace(initialState.frontFace);
	currentState.frontFace=initialState.frontFace;
	if(currentState.cullingEnabled!=initialState.cullingEnabled)
		setGLState(GL_CULL_FACE,initialState.cullingEnabled);
	currentState.cullingEnabled=initialState.cullingEnabled;
	if(currentState.culledFace!=initialState.culledFace)
		glCullFace(initialState.culledFace);
	currentState.culledFace=initialState.culledFace;
	if(currentState.lightingEnabled!=initialState.lightingEnabled)
		setGLState(GL_LIGHTING,initialState.lightingEnabled);
	currentState.lightingEnabled=initialState.lightingEnabled;
	if(currentState.normalizeEnabled!=initialState.normalizeEnabled)
		setGLState(GL_NORMALIZE,initialState.normalizeEnabled);
	currentState.normalizeEnabled=initialState.normalizeEnabled;
	if(currentState.lightModelTwoSide!=initialState.lightModelTwoSide)
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,initialState.lightModelTwoSide);
	currentState.lightModelTwoSide=initialState.lightModelTwoSide;
	if(currentState.colorMaterialEnabled!=initialState.colorMaterialEnabled)
		setGLState(GL_COLOR_MATERIAL,initialState.colorMaterialEnabled);
	currentState.colorMaterialEnabled=initialState.colorMaterialEnabled;
	if(currentState.lightModelColorControl!=initialState.lightModelColorControl)
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,initialState.lightModelColorControl);
	currentState.lightModelColorControl=initialState.lightModelColorControl;
	
	/* Reset the blending function if in the transparent rendering pass: */
	if(currentRenderPass==GraphNode::GLTransparentRenderPass)
		{
		currentState.blendSrcFactor=GL_SRC_ALPHA;
		currentState.blendDstFactor=GL_ONE_MINUS_SRC_ALPHA;
		glBlendFunc(currentState.blendSrcFactor,currentState.blendDstFactor);
		}
	
	/* Reset the modelview matrix, assuming that any pushed transformations have been popped: */
	if(modelviewOutdated)
		loadCurrentTransform();
	
	/* Reset the matrix mode: */
	if(currentState.matrixMode!=initialState.matrixMode)
		glMatrixMode(initialState.matrixMode);
	currentState.matrixMode=initialState.matrixMode;
	
	/* Reset active vertex arrays: */
	changeVertexArraysMask(currentState.activeVertexArraysMask,initialState.activeVertexArraysMask);
	currentState.activeVertexArraysMask=initialState.activeVertexArraysMask;
	
	/* Unbind active vertex and index buffers: */
	if(currentState.vertexBuffer!=initialState.vertexBuffer)
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,initialState.vertexBuffer);
	currentState.vertexBuffer=initialState.vertexBuffer;
	if(currentState.indexBuffer!=initialState.indexBuffer)
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,initialState.indexBuffer);
	currentState.indexBuffer=initialState.indexBuffer;
	
	/* Reset the bound shader program: */
	if(currentState.shaderProgram!=initialState.shaderProgram)
		glUseProgramObjectARB(initialState.shaderProgram);
	currentState.shaderProgram=initialState.shaderProgram;
	}

void GLRenderState::setFrontFace(GLenum newFrontFace)
	{
	/* Update the front face: */
	if(currentState.frontFace!=newFrontFace)
		glFrontFace(newFrontFace);
	currentState.frontFace=newFrontFace;
	}

void GLRenderState::enableCulling(GLenum newCulledFace)
	{
	/* Update culling: */
	if(!currentState.cullingEnabled)
		glEnable(GL_CULL_FACE);
	currentState.cullingEnabled=true;
	
	/* Update the culled face: */
	if(currentState.culledFace!=newCulledFace)
		glCullFace(newCulledFace);
	currentState.culledFace=newCulledFace;
	}

void GLRenderState::disableCulling(void)
	{
	/* Update culling: */
	if(currentState.cullingEnabled)
		glDisable(GL_CULL_FACE);
	currentState.cullingEnabled=false;
	}

void GLRenderState::enableMaterials(void)
	{
	/* Disable any active shader programs: */
	if(currentState.shaderProgram!=0)
		glUseProgramObjectARB(0);
	currentState.shaderProgram=0;
	
	/* Enable lighting: */
	if(!currentState.lightingEnabled)
		{
		glEnable(GL_LIGHTING);
		
		/* Enable normal vector normalization: */
		if(!currentState.normalizeEnabled)
			glEnable(GL_NORMALIZE);
		currentState.normalizeEnabled=true;
		
		/* Enable texture lighting: */
		if(currentState.highestTexturePriority>=0)
			glTexEnvMode(GLTexEnvEnums::TEXTURE_ENV,GLTexEnvEnums::MODULATE);
		}
	currentState.lightingEnabled=true;
	
	/* Enable texture specular lighting: */
	if(currentState.highestTexturePriority>=0&&currentState.lightModelColorControl!=GL_SEPARATE_SPECULAR_COLOR)
		{
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR);
		currentState.lightModelColorControl=GL_SEPARATE_SPECULAR_COLOR;
		}
	}

void GLRenderState::setTwoSidedLighting(bool enable)
	{
	/* Update two-sided lighting: */
	if(currentState.lightModelTwoSide!=GL_TRUE&&enable)
		{
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
		currentState.lightModelTwoSide=GL_TRUE;
		}
	else if(currentState.lightModelTwoSide!=GL_FALSE&&!enable)
		{
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);
		currentState.lightModelTwoSide=GL_FALSE;
		}
	}

void GLRenderState::setColorMaterial(bool enable)
	{
	/* Update color materials: */
	if(currentState.colorMaterialEnabled!=enable)
		{
		setGLState(GL_COLOR_MATERIAL,enable);
		if(enable)
			glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
		}
	currentState.colorMaterialEnabled=enable;
	}

void GLRenderState::disableMaterials(void)
	{
	/* Disable any active shader programs: */
	if(currentState.shaderProgram!=0)
		glUseProgramObjectARB(0);
	currentState.shaderProgram=0;
	
	/* Disable lighting: */
	if(currentState.lightingEnabled)
		{
		glDisable(GL_LIGHTING);
		
		/* Disable texture lighting: */
		if(currentState.highestTexturePriority>=0)
			glTexEnvMode(GLTexEnvEnums::TEXTURE_ENV,GLTexEnvEnums::REPLACE);
		}
	currentState.lightingEnabled=false;
	
	/* Set the emissive color as the current color: */
	glColor(currentState.emissiveColor);
	}

void GLRenderState::setEmissiveColor(const Color& newEmissiveColor)
	{
	currentState.emissiveColor=newEmissiveColor;
	glColor(newEmissiveColor);
	}

void GLRenderState::enableTexture1D(void)
	{
	/* Disable any active shader programs: */
	if(currentState.shaderProgram!=0)
		glUseProgramObjectARB(0);
	currentState.shaderProgram=0;
	
	/* Enable 1D texture mapping: */
	bool textureEnabled=currentState.highestTexturePriority>=0;
	if(currentState.highestTexturePriority>=3)
		glDisable(GL_TEXTURE_CUBE_MAP_EXT);
	if(currentState.highestTexturePriority>=2)
		glDisable(GL_TEXTURE_3D_EXT);
	if(currentState.highestTexturePriority>=1)
		glDisable(GL_TEXTURE_2D);
	if(currentState.highestTexturePriority<0)
		glEnable(GL_TEXTURE_1D);
	currentState.highestTexturePriority=0;
	
	/* Update texture lighting: */
	if(!textureEnabled)
		glTexEnvMode(GLTexEnvEnums::TEXTURE_ENV,currentState.lightingEnabled?GLTexEnvEnums::MODULATE:GLTexEnvEnums::REPLACE);
	if(currentState.lightingEnabled&&currentState.lightModelColorControl!=GL_SEPARATE_SPECULAR_COLOR)
		{
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR);
		currentState.lightModelColorControl=GL_SEPARATE_SPECULAR_COLOR;
		}
	}

void GLRenderState::enableTexture2D(void)
	{
	/* Disable any active shader programs: */
	if(currentState.shaderProgram!=0)
		glUseProgramObjectARB(0);
	currentState.shaderProgram=0;
	
	/* Enable 2D texture mapping: */
	bool textureEnabled=currentState.highestTexturePriority>=0;
	if(currentState.highestTexturePriority>=3)
		glDisable(GL_TEXTURE_CUBE_MAP_EXT);
	if(currentState.highestTexturePriority>=2)
		glDisable(GL_TEXTURE_3D_EXT);
	if(currentState.highestTexturePriority<1)
		glEnable(GL_TEXTURE_2D);
	currentState.highestTexturePriority=1;
	
	/* Update texture lighting: */
	if(!textureEnabled)
		glTexEnvMode(GLTexEnvEnums::TEXTURE_ENV,currentState.lightingEnabled?GLTexEnvEnums::MODULATE:GLTexEnvEnums::REPLACE);
	if(currentState.lightingEnabled&&currentState.lightModelColorControl!=GL_SEPARATE_SPECULAR_COLOR)
		{
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR);
		currentState.lightModelColorControl=GL_SEPARATE_SPECULAR_COLOR;
		}
	}

void GLRenderState::enableTexture3D(void)
	{
	/* Disable any active shader programs: */
	if(currentState.shaderProgram!=0)
		glUseProgramObjectARB(0);
	currentState.shaderProgram=0;
	
	/* Enable 3D texture mapping: */
	bool textureEnabled=currentState.highestTexturePriority>=0;
	if(currentState.highestTexturePriority>=3)
		glDisable(GL_TEXTURE_CUBE_MAP_EXT);
	if(currentState.highestTexturePriority<2)
		glEnable(GL_TEXTURE_2D);
	currentState.highestTexturePriority=2;
	
	/* Update texture lighting: */
	if(!textureEnabled)
		glTexEnvMode(GLTexEnvEnums::TEXTURE_ENV,currentState.lightingEnabled?GLTexEnvEnums::MODULATE:GLTexEnvEnums::REPLACE);
	if(currentState.lightingEnabled&&currentState.lightModelColorControl!=GL_SEPARATE_SPECULAR_COLOR)
		{
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR);
		currentState.lightModelColorControl=GL_SEPARATE_SPECULAR_COLOR;
		}
	}

void GLRenderState::disableTextures(void)
	{
	/* Disable any active shader programs: */
	if(currentState.shaderProgram!=0)
		glUseProgramObjectARB(0);
	currentState.shaderProgram=0;
	
	/* Disable texture mapping: */
	if(currentState.highestTexturePriority>=3)
		glDisable(GL_TEXTURE_CUBE_MAP_EXT);
	if(currentState.highestTexturePriority>=2)
		glDisable(GL_TEXTURE_3D_EXT);
	if(currentState.highestTexturePriority>=1)
		glDisable(GL_TEXTURE_2D);
	if(currentState.highestTexturePriority>=0)
		glDisable(GL_TEXTURE_1D);
	currentState.highestTexturePriority=-1;
	
	/* Update texture lighting: */
	if(currentState.lightingEnabled&&currentState.lightModelColorControl!=GL_SINGLE_COLOR)
		{
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SINGLE_COLOR);
		currentState.lightModelColorControl=GL_SINGLE_COLOR;
		}
	}

void GLRenderState::blendFunc(GLenum newBlendSrcFactor,GLenum newBlendDstFactor)
	{
	if(currentState.blendSrcFactor!=newBlendSrcFactor||currentState.blendDstFactor!=newBlendDstFactor)
		{
		currentState.blendSrcFactor=newBlendSrcFactor;
		currentState.blendDstFactor=newBlendDstFactor;
		glBlendFunc(currentState.blendSrcFactor,currentState.blendDstFactor);
		}
	}

void GLRenderState::bindShader(GLhandleARB newShaderProgram)
	{
	if(currentState.shaderProgram!=newShaderProgram)
		{
		glUseProgramObjectARB(newShaderProgram);
		currentState.shaderProgram=newShaderProgram;
		}
	}

void GLRenderState::disableShaders(void)
	{
	/* Disable any active shader programs: */
	if(currentState.shaderProgram!=0)
		glUseProgramObjectARB(0);
	currentState.shaderProgram=0;
	}

}
