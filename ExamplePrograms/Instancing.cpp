/***********************************************************************
Instancing - Render large numbers of similar objects using instancing.
Copyright (c) 2020-2022 Oliver Kreylos
***********************************************************************/

#include <stdlib.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Math/Random.h>
#include <Geometry/Vector.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthonormalTransformation.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GL/GLMaterialTemplates.h>
#include <GL/GLObject.h>
#include <GL/GLContextData.h>
#include <GL/Extensions/GLARBDrawInstanced.h>
#include <GL/Extensions/GLARBFragmentShader.h>
#include <GL/Extensions/GLARBInstancedArrays.h>
#include <GL/Extensions/GLARBShaderObjects.h>
#include <GL/Extensions/GLARBVertexArrayObject.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/Extensions/GLARBVertexProgram.h>
#include <GL/Extensions/GLARBVertexShader.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>

class Instancing:public Vrui::Application,public GLObject
	{
	/* Embedded classes: */
	private:
	typedef Geometry::Vector<GLfloat,3> Vector; // Type for vectors
	typedef Geometry::Rotation<GLfloat,3> Rotation; // Type for rotations
	typedef Geometry::OrthonormalTransformation<GLfloat,3> Transform; // Type for transformation matrices
	typedef GLColor<GLfloat,4> Color; // Type for colors
	
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLhandleARB objectShader; // Shader program to render instanced objects
		GLuint objectVertexBuffer; // ID of buffer holding object vertices
		GLuint instanceAttribBuffer; // ID of buffer holding instance attributes (color and matrix)
		GLuint vertexArray; // ID of vertex array object combining per-object and per-instance vertex attributes
		unsigned int instanceAttribVersion; // Version number of instance attributes in buffer
		unsigned int numInstances; // Number of isntances in instance attribute buffer
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	unsigned int numInstances; // Number of object instances to draw
	Color* instanceColors; // Array of object instance colors
	Transform* instanceTransforms; // Array of object instance transformations
	Vector* instanceAngularVelocities; // Array of instance angular velocities
	unsigned int instanceAttribVersion; // Version number of instance attributes
	
	/* Constructors and destructors: */
	public:
	Instancing(int& argc,char**& argv);
	virtual ~Instancing(void);
	
	/* Methods from class Vrui::Application: */
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	virtual void resetNavigation(void);
	
	/* Methods from class GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	};

/*************************************
Methods of class Instancing::DataItem:
*************************************/

Instancing::DataItem::DataItem(void)
	:objectShader(0),
	 objectVertexBuffer(0),instanceAttribBuffer(0),vertexArray(0),
	 instanceAttribVersion(0),numInstances(0)
	{
	/* Initialize required OpenGL extensions: */
	GLARBDrawInstanced::initExtension();
	GLARBFragmentShader::initExtension();
	GLARBInstancedArrays::initExtension();
	GLARBShaderObjects::initExtension();
	GLARBVertexArrayObject::initExtension();
	GLARBVertexBufferObject::initExtension();
	GLARBVertexProgram::initExtension();
	GLARBVertexShader::initExtension();
	
	/* Create the object shader: */
	objectShader=glCreateProgramObjectARB();
	
	/* Create vertex buffer objects: */
	glGenBuffersARB(1,&objectVertexBuffer);
	glGenBuffersARB(1,&instanceAttribBuffer);
	glGenVertexArrays(1,&vertexArray);
	}

Instancing::DataItem::~DataItem(void)
	{
	/* Destroy the object shader: */
	glDeleteObjectARB(objectShader);
	
	/* Destroy vertex buffer objects: */
	glDeleteBuffersARB(1,&objectVertexBuffer);
	glDeleteBuffersARB(1,&instanceAttribBuffer);
	glDeleteVertexArrays(1,&vertexArray);
	}

/***************************
Methods of class Instancing:
***************************/

Instancing::Instancing(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 instanceColors(0),instanceTransforms(0),instanceAngularVelocities(0),
	 instanceAttribVersion(1)
	{
	/* Calculate the number of instances: */
	unsigned int w=10;
	if(argc>=2)
		w=atoi(argv[1]);
	numInstances=w*w*w;
	instanceColors=new Color[numInstances];
	instanceTransforms=new Transform[numInstances];
	instanceAngularVelocities=new Vector[numInstances];
	
	/* Create random instance colors: */
	for(unsigned int i=0;i<numInstances;++i)
		{
		for(int j=0;j<3;++j)
			instanceColors[i][j]=GLfloat(Math::randUniformCC(0.0,1.0));
		instanceColors[i][3]=1.0f;
		}
	
	/* Create instance transformations: */
	const GLfloat pi=Math::Constants<GLfloat>::pi;
	GLfloat offset=GLfloat(w-1)*2.0f;
	Transform* itPtr=instanceTransforms;
	for(unsigned int z=0;z<w;++z)
		for(unsigned int y=0;y<w;++y)
			for(unsigned int x=0;x<w;++x,++itPtr)
				{
				/* Create an orderly translation: */
				Vector t(GLfloat(x)*4.0f-offset,GLfloat(y)*4.0f-offset,GLfloat(z)*4.0f-offset);
				for(int i=0;i<3;++i)
					t[i]+=GLfloat(Math::randUniformCC(-1.5,1.5));
				*itPtr=Transform::translate(t);
				
				/* Create a random rotation: */
				Vector rotAxis;
				do
					{
					for(int i=0;i<3;++i)
						rotAxis[i]=GLfloat(Math::randUniformCC(double(-pi),double(pi)));
					}
				while(rotAxis.sqr()>Math::sqr(pi));
				*itPtr*=Transform::rotate(Rotation(rotAxis));
				}
	
	/* Create random angular velocities: */
	Vector* iavPtr=instanceAngularVelocities;
	for(unsigned int i=0;i<numInstances;++i,++iavPtr)
		{
		Vector rotAxis;
		do
			{
			for(int i=0;i<3;++i)
				rotAxis[i]=GLfloat(Math::randUniformCC(0.5*double(-pi),0.5*double(pi)));
			}
		while(rotAxis.sqr()>Math::sqr(0.5*pi));
		*iavPtr=rotAxis;
		}
	}

Instancing::~Instancing(void)
	{
	/* Clean up: */
	delete[] instanceColors;
	delete[] instanceTransforms;
	delete[] instanceAngularVelocities;
	}

void Instancing::frame(void)
	{
	#if 1
	
	/* Update all instance orientations: */
	GLfloat timeStep(Vrui::getFrameTime());
	Transform* itPtr=instanceTransforms;
	Vector* iavPtr=instanceAngularVelocities;
	for(unsigned int i=0;i<numInstances;++i,++itPtr,++iavPtr)
		itPtr->getRotation()*=Rotation(*iavPtr*timeStep);
	
	++instanceAttribVersion;
	#endif
	
	Vrui::scheduleUpdate(Vrui::getNextAnimationTime());
	}

void Instancing::display(GLContextData& contextData) const
	{
	/* Access the context data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Check if the instance attribute array needs to be updated: */
	if(dataItem->instanceAttribVersion!=instanceAttribVersion)
		{
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->instanceAttribBuffer);
		
		/* Resize the instance attribute buffer if necessary: */
		if(dataItem->numInstances!=numInstances)
			{
			glBufferDataARB(GL_ARRAY_BUFFER_ARB,numInstances*11*sizeof(GLfloat),0,GL_DYNAMIC_DRAW_ARB);
			dataItem->numInstances!=numInstances;
			}
		
		/* Upload the new instance attributes: */
		GLfloat* vPtr=static_cast<GLfloat*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
		const Color* icPtr=instanceColors;
		const Transform* itPtr=instanceTransforms;
		for(unsigned int instance=0;instance<numInstances;++instance,++icPtr,++itPtr)
			{
			/* Upload the instance color: */
			for(int i=0;i<4;++i)
				*(vPtr++)=(*icPtr)[i];
			
			/* Upload the instance transform as a translation vector and a unit quaternion: */
			for(int i=0;i<3;++i)
				*(vPtr++)=itPtr->getTranslation()[i];
			for(int i=0;i<4;++i)
				*(vPtr++)=itPtr->getRotation().getQuaternion()[i];
			}
		glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
		
		/* Protect the instance attribute buffer: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
		
		dataItem->instanceAttribVersion=instanceAttribVersion;
		}
	
	/* Set up material properties: */
	glMaterialAmbientAndDiffuse(GLMaterialEnums::FRONT,GLColor<GLfloat,4>(1.0f,1.0f,1.0f));
	glMaterialSpecular(GLMaterialEnums::FRONT,GLColor<GLfloat,4>(1.0f,1.0f,1.0f));
	glMaterialShininess(GLMaterialEnums::FRONT,32.0f);
	glMaterialEmission(GLMaterialEnums::FRONT,GLColor<GLfloat,4>(0.0f,0.0f,0.0f));
	
	/* Draw the instances: */
	glUseProgramObjectARB(dataItem->objectShader);
	glBindVertexArray(dataItem->vertexArray);
	glDrawArraysInstancedARB(GL_QUADS,0,24,numInstances);
	glBindVertexArray(0);
	glUseProgramObjectARB(0);
	}

void Instancing::resetNavigation(void)
	{
	Vrui::setNavigationTransformation(Vrui::Point::origin,Vrui::Scalar(100));
	}

void Instancing::initContext(GLContextData& contextData) const
	{
	/* Create a context data item and associate it with this application object: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Compile the vertex and fragment shaders: */
	GLhandleARB vertexShader=glCompileVertexShaderFromFile(SHADERDIR "/InstancedObject.vs");
	glAttachObjectARB(dataItem->objectShader,vertexShader);
	GLhandleARB fragmentShader=glCompileFragmentShaderFromFile(SHADERDIR "/InstancedObject.fs");
	glAttachObjectARB(dataItem->objectShader,fragmentShader);
	
	/* Set vertex attribute names: */
	glBindAttribLocationARB(dataItem->objectShader,0,"v_normal");
	glBindAttribLocationARB(dataItem->objectShader,1,"v_position");
	glBindAttribLocationARB(dataItem->objectShader,2,"i_color");
	glBindAttribLocationARB(dataItem->objectShader,3,"i_translation");
	glBindAttribLocationARB(dataItem->objectShader,4,"i_rotation");
	
	/* Link the instanced object shader program: */
	glLinkAndTestShader(dataItem->objectShader);
	
	/* Release the vertex and fragment shaders: */
	glDeleteObjectARB(vertexShader);
	glDeleteObjectARB(fragmentShader);
	
	/* Create the vertex array object: */
	glBindVertexArray(dataItem->vertexArray);
	
	/* Set the vertex array's object attributes: */
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->objectVertexBuffer);
	glEnableVertexAttribArrayARB(0); // Normal vector
	glVertexAttribPointerARB(0,3,GL_FLOAT,GL_TRUE,7*sizeof(GLfloat),static_cast<const GLfloat*>(0)+0);
	glEnableVertexAttribArrayARB(1); // Position
	glVertexAttribPointerARB(1,4,GL_FLOAT,GL_FALSE,7*sizeof(GLfloat),static_cast<const GLfloat*>(0)+3);
	
	/* Create a cube: */
	static const GLfloat vertexData[24*7]=
		{
		// Bottom face
		 0.0f, 0.0f,-1.0f,-0.5f,-0.5f,-0.5f, 1.0f,
		 0.0f, 0.0f,-1.0f,-0.5f, 0.5f,-0.5f, 1.0f,
		 0.0f, 0.0f,-1.0f, 0.5f, 0.5f,-0.5f, 1.0f,
		 0.0f, 0.0f,-1.0f, 0.5f,-0.5f,-0.5f, 1.0f,
		
		// Top face
		 0.0f, 0.0f, 1.0f,-0.5f,-0.5f, 0.5f, 1.0f,
		 0.0f, 0.0f, 1.0f, 0.5f,-0.5f, 0.5f, 1.0f,
		 0.0f, 0.0f, 1.0f, 0.5f, 0.5f, 0.5f, 1.0f,
		 0.0f, 0.0f, 1.0f,-0.5f, 0.5f, 0.5f, 1.0f,
		
		// Front face
		 0.0f,-1.0f, 0.0f,-0.5f,-0.5f,-0.5f, 1.0f,
		 0.0f,-1.0f, 0.0f, 0.5f,-0.5f,-0.5f, 1.0f,
		 0.0f,-1.0f, 0.0f, 0.5f,-0.5f, 0.5f, 1.0f,
		 0.0f,-1.0f, 0.0f,-0.5f,-0.5f, 0.5f, 1.0f,
		
		// Back face
		 0.0f, 1.0f, 0.0f,-0.5f, 0.5f,-0.5f, 1.0f,
		 0.0f, 1.0f, 0.0f,-0.5f, 0.5f, 0.5f, 1.0f,
		 0.0f, 1.0f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f,
		 0.0f, 1.0f, 0.0f, 0.5f, 0.5f,-0.5f, 1.0f,
		
		// Left face
		-1.0f, 0.0f, 0.0f,-0.5f,-0.5f,-0.5f, 1.0f,
		-1.0f, 0.0f, 0.0f,-0.5f,-0.5f, 0.5f, 1.0f,
		-1.0f, 0.0f, 0.0f,-0.5f, 0.5f, 0.5f, 1.0f,
		-1.0f, 0.0f, 0.0f,-0.5f, 0.5f,-0.5f, 1.0f,
		
		// Right face
		 1.0f, 0.0f, 0.0f, 0.5f,-0.5f,-0.5f, 1.0f,
		 1.0f, 0.0f, 0.0f, 0.5f, 0.5f,-0.5f, 1.0f,
		 1.0f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f,
		 1.0f, 0.0f, 0.0f, 0.5f,-0.5f, 0.5f, 1.0f
		};
	
	/* Upload the object vertex buffer: */
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,24*7*sizeof(GLfloat),vertexData,GL_STATIC_DRAW_ARB);
	
	/* Set the vertex array's instance attributes: */
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->instanceAttribBuffer);
	glEnableVertexAttribArrayARB(2); // Color
	glVertexAttribPointerARB(2,4,GL_FLOAT,GL_TRUE,11*sizeof(GLfloat),static_cast<const GLfloat*>(0)+0);
	glVertexAttribDivisorARB(2,1);
	glEnableVertexAttribArrayARB(3); // Translation
	glVertexAttribPointerARB(3,3,GL_FLOAT,GL_TRUE,11*sizeof(GLfloat),static_cast<const GLfloat*>(0)+4);
	glVertexAttribDivisorARB(3,1);
	glEnableVertexAttribArrayARB(4); // Rotation
	glVertexAttribPointerARB(4,4,GL_FLOAT,GL_TRUE,11*sizeof(GLfloat),static_cast<const GLfloat*>(0)+7);
	glVertexAttribDivisorARB(4,1);
	
	/* Protect the buffers and vertex array: */
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
	glBindVertexArray(0);
	}

VRUI_APPLICATION_RUN(Instancing)
