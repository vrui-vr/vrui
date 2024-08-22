/***********************************************************************
GLRenderState - Class encapsulating the traversal state of a scene graph
during OpenGL rendering.
Copyright (c) 2009-2023 Oliver Kreylos

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

#ifndef SCENEGRAPH_GLRENDERSTATE_INCLUDED
#define SCENEGRAPH_GLRENDERSTATE_INCLUDED

#include <Misc/SizedTypes.h>
#include <Misc/Rect.h>
#include <Geometry/Plane.h>
#include <Geometry/AffineTransformation.h>
#include <Geometry/ProjectiveTransformation.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/Extensions/GLARBShaderObjects.h>
#include <GL/Extensions/GLEXTTexture3D.h>
#include <GL/Extensions/GLEXTTextureCubeMap.h>
#include <SceneGraph/TraversalState.h>

/* Forward declarations: */
class GLContextData;

namespace SceneGraph {

class GLRenderState:public TraversalState
	{
	/* Embedded classes: */
	public:
	typedef Misc::Rect<2> Rect; // Type for 2D integer rectangles
	typedef GLColor<GLfloat,4> Color; // Type for RGBA colors
	typedef Geometry::Point<double,3> DPoint; // Type for double-precision points
	typedef Geometry::Vector<double,3> DVector; // Type for double-precision vectors
	typedef Geometry::Plane<double,3> DPlane; // Type for double-precision plane equations
	typedef Geometry::ProjectiveTransformation<double,3> DPTransform; // Type for double-precision projective transformations
	typedef Geometry::AffineTransformation<Scalar,3> TextureTransform; // Affine texture transformation
	
	private:
	struct GLState // Structure to track current OpenGL state to minimize changes
		{
		/* Elements: */
		public:
		GLenum frontFace;
		bool cullingEnabled;
		GLenum culledFace;
		bool lightingEnabled;
		bool normalizeEnabled;
		GLenum lightModelTwoSide;
		Color emissiveColor;
		bool colorMaterialEnabled;
		int highestTexturePriority; // Priority level of highest enabled texture unit (None=-1, 1D=0, 2D, 3D, cube map)
		GLuint boundTextures[4]; // Texture object IDs of currently bound 1D, 2D, 3D, and cube map textures
		GLenum lightModelColorControl;
		GLenum blendSrcFactor,blendDstFactor; // Blend function coefficients for transparent rendering
		GLenum matrixMode; // Current matrix mode
		int activeVertexArraysMask; // Bit mask of currently active vertex arrays, from GLVertexArrayParts
		GLuint vertexBuffer; // ID of currently bound vertex buffer
		GLuint indexBuffer; // ID of currently bound index buffer
		GLhandleARB shaderProgram; // Currently bound shader program, or null
		};
	
	/* Elements: */
	public:
	GLContextData& contextData; // Context data of the current OpenGL context
	private:
	Point baseEyePos; // Actual eye position for this rendering pass in eye space
	Rect viewport; // The current window's viewport (x, y, w, h)
	DPTransform projection; // The rendering context's projection matrix
	DPoint frustumPoints[6]; // Points on the rendering context's six view frustum planes in eye space
	DVector frustumNormals[6]; // Normal vectors of the rendering context's six view frustum planes in eye space
	Misc::UInt32 initialRenderPass; // The initially active rendering pass
	Misc::UInt32 currentRenderPass; // The currently active rendering pass
	bool modelviewOutdated; // Flag if OpenGL's modelview matrix does not correspond to the current model transformation
	bool haveTextureTransform; // Flag if a texture transformation has been set
	
	/* Elements shadowing current OpenGL state: */
	public:
	GLState initialState; // OpenGL state when render state object was created
	GLState currentState; // Current OpenGL state
	
	/* Private methods: */
	private:
	void calcFrustum(void); // Initializes the view frustum planes
	void loadCurrentTransform(void); // Uploads the current transformation as OpenGL's modelview matrix
	void changeVertexArraysMask(int currentMask,int newMask); // Changes the set of active vertex arrays
	
	/* Constructors and destructors: */
	public:
	GLRenderState(GLContextData& sContextData,const Point& sBaseEyePos,const Rect& sViewport,const DPTransform& sProjection,const DOGTransform& initialTransform,const Point& sBaseViewerPos,const Vector& sBaseUpVector); // Creates a render state object
	~GLRenderState(void); // Releases OpenGL state and destroys render state object
	
	/* Methods from class TraversalState: */
	void startTraversal(const Point& newBaseEyePos,const Rect& newViewport,const DPTransform& newProjection,const DOGTransform& newCurrentTransform,const Point& newBaseViewerPos,const Vector& newBaseUpVector);
	DOGTransform pushTransform(const DOGTransform& deltaTransform)
		{
		/* Mark OpenGL's modelview matrix as outdated and update the transformation: */
		modelviewOutdated=true;
		return TraversalState::pushTransform(deltaTransform);
		}
	DOGTransform pushTransform(const ONTransform& deltaTransform)
		{
		/* Mark OpenGL's modelview matrix as outdated and update the transformation: */
		modelviewOutdated=true;
		return TraversalState::pushTransform(deltaTransform);
		}
	DOGTransform pushTransform(const OGTransform& deltaTransform)
		{
		/* Mark OpenGL's modelview matrix as outdated and update the transformation: */
		modelviewOutdated=true;
		return TraversalState::pushTransform(deltaTransform);
		}
	void popTransform(const DOGTransform& previousTransform)
		{
		/* Mark OpenGL's modelview matrix as outdated and update the transformation: */
		modelviewOutdated=true;
		TraversalState::popTransform(previousTransform);
		}
	
	/* New methods: */
	Point getEyePos(void) const // Returns the actual eye position in current model space
		{
		return Point(currentTransform.inverseTransform(baseEyePos));
		}
	const Rect& getViewport(void) const // Returns the current window's viewport
		{
		return viewport;
		}
	const DPTransform& getProjection(void) const // Returns the OpenGL context's projection matrix
		{
		return projection;
		}
	DPlane getFrustumPlane(int planeIndex) const; // Returns one of the six frustum planes in current model space with a unit-length normal vector
	bool doesBoxIntersectFrustum(const Box& box) const; // Returns true if the given box in current model space intersects the view frustum
	Misc::UInt32 getRenderPass(void) const // Returns the mask flag of the current rendering pass
		{
		return currentRenderPass;
		}
	void setRenderPass(Misc::UInt32 newRenderPass); // Switches to the given rendering pass
	void setTextureTransform(const TextureTransform& newTextureTransform); // Sets the given transformation as the new texture transformation
	void resetTextureTransform(void); // Resets the texture transformation
	
	/* OpenGL state management methods: */
	void uploadModelview(void) // Uploads the current transformation into OpenGL's modelview matrix
		{
		/* Upload the current transformation if OpenGL's modelview matrix has not been updated since the last transformation change: */
		if(modelviewOutdated)
			loadCurrentTransform();
		}
	void resetState(void); // Resets OpenGL state to the initial state
	void setFrontFace(GLenum newFrontFace); // Selects whether counter-clockwise or clockwise polygons are front-facing
	void enableCulling(GLenum newCulledFace); // Enables OpenGL face culling
	void disableCulling(void); // Disables OpenGL face culling
	void enableMaterials(void); // Enables OpenGL material rendering
	void setTwoSidedLighting(bool enable); // Enables or disables two-sided lighting
	void setColorMaterial(bool enable); // Enables or disables color material tracking
	void disableMaterials(void); // Disables OpenGL material rendering
	void setEmissiveColor(const Color& newEmissiveColor); // Sets the current emissive color
	void enableTexture1D(void); // Enables OpenGL 1D texture mapping
	void bindTexture1D(GLuint textureObjectId) // Binds a 1D texture
		{
		/* Check if the texture to bind is not currently bound: */
		if(currentState.boundTextures[0]!=textureObjectId)
			{
			glBindTexture(GL_TEXTURE_1D,textureObjectId);
			currentState.boundTextures[0]=textureObjectId;
			}
		}
	void enableTexture2D(void); // Enables OpenGL 2D texture mapping
	void bindTexture2D(GLuint textureObjectId) // Binds a 2D texture
		{
		/* Check if the texture to bind is not currently bound: */
		if(currentState.boundTextures[1]!=textureObjectId)
			{
			glBindTexture(GL_TEXTURE_2D,textureObjectId);
			currentState.boundTextures[1]=textureObjectId;
			}
		}
	void enableTexture3D(void); // Enables OpenGL 3D texture mapping
	void bindTexture3D(GLuint textureObjectId) // Binds a 3D texture
		{
		/* Check if the texture to bind is not currently bound: */
		if(currentState.boundTextures[2]!=textureObjectId)
			{
			glBindTexture(GL_TEXTURE_3D_EXT,textureObjectId);
			currentState.boundTextures[2]=textureObjectId;
			}
		}
	void enableTextureCubeMap(void); // Enables OpenGL cube map texture mapping
	void bindTextureCubeMap(GLuint textureObjectId) // Binds a cube map texture
		{
		/* Check if the texture to bind is not currently bound: */
		if(currentState.boundTextures[3]!=textureObjectId)
			{
			glBindTexture(GL_TEXTURE_CUBE_MAP_EXT,textureObjectId);
			currentState.boundTextures[3]=textureObjectId;
			}
		}
	void disableTextures(void); // Disables OpenGL texture mapping
	void blendFunc(GLenum newBlendSrcFactor,GLenum newBlendDstFactor); // Sets the blending function during the transparent rendering pass
	void enableVertexArrays(int vertexArraysMask) // Enables the given set of vertex arrays
		{
		/* Activate/deactivate arrays as needed and update the current state: */
		changeVertexArraysMask(currentState.activeVertexArraysMask,vertexArraysMask);
		currentState.activeVertexArraysMask=vertexArraysMask;
		}
	void bindVertexBuffer(GLuint newVertexBuffer) // Binds the given vertex buffer
		{
		/* Check if the new buffer is different from the current one: */
		if(currentState.vertexBuffer!=newVertexBuffer)
			{
			/* Bind the new buffer and update the current state: */
			glBindBufferARB(GL_ARRAY_BUFFER_ARB,newVertexBuffer);
			currentState.vertexBuffer=newVertexBuffer;
			}
		}
	void bindIndexBuffer(GLuint newIndexBuffer) // Binds the given index buffer
		{
		/* Check if the new buffer is different from the current one: */
		if(currentState.indexBuffer!=newIndexBuffer)
			{
			/* Bind the new buffer and update the current state: */
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,newIndexBuffer);
			currentState.indexBuffer=newIndexBuffer;
			}
		}
	void bindShader(GLhandleARB newShaderProgram); // Binds the shader program of the current handle by calling glUseProgramObjectARB
	void disableShaders(void); // Unbinds any currently-bound shaders and returns to OpenGL fixed functionality
	};

}

#endif
