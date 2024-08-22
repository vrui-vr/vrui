/***********************************************************************
ReadLwoFile - Helper function to read a 3D polygon file in Lightwave
Object format into a list of shape nodes.
Copyright (c) 2021-2024 Oliver Kreylos

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

#include <SceneGraph/Internal/ReadLwoFile.h>

#include <string.h>
#include <stdexcept>
#include <vector>
#include <Misc/SizedTypes.h>
#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>
#include <IO/Directory.h>
#include <IO/IFFChunk.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <GL/GLColorOperations.h>
#include <SceneGraph/Geometry.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/MaterialNode.h>
#include <SceneGraph/ImageTextureNode.h>
#include <SceneGraph/AppearanceNode.h>
#include <SceneGraph/TextureCoordinateNode.h>
#include <SceneGraph/CoordinateNode.h>
#include <SceneGraph/IndexedFaceSetNode.h>
#include <SceneGraph/ShapeNode.h>
#include <SceneGraph/MeshFileNode.h>

namespace SceneGraph {

namespace {

/****************
Helper functions:
****************/

std::string readString(IO::File& file) // Reads a NUL-terminated string
	{
	/* Read a NUL-terminated string: */
	std::string result;
	int c;
	while((c=file.getChar())!='\0')
		result.push_back(c);
	
	/* Read an extra padding byte to make the total number of read bytes even: */
	if((result.length()&0x1)==0x0)
		file.getChar();
	
	return result;
	}

Point readPoint(IO::File& file) // Reads a point defined by three 32-bit floats
	{
	Point result;
	result[0]=Scalar(file.read<Misc::Float32>());
	result[2]=Scalar(file.read<Misc::Float32>()); // Swap y and z coordinates to flip
	result[1]=Scalar(file.read<Misc::Float32>()); // coordinate system's handedness
	return result;
	}

Vector readVector(IO::File& file) // Reads a vector defined by three 32-bit floats
	{
	Vector result;
	result[0]=Scalar(file.read<Misc::Float32>());
	result[2]=Scalar(file.read<Misc::Float32>()); // Swap y and z coordinates to flip
	result[1]=Scalar(file.read<Misc::Float32>()); // coordinate system's handedness
	return result;
	}

Color readColor3ub(IO::File& file) // Reads an RGB color defined by three 8-bit unsigned integers
	{
	/* Read a GLubyte color and convert to a GLfloat color: */
	GLColor<GLubyte,3> temp;
	file.read(temp.getRgba(),3);
	return Color(temp);
	}

Color readColor3f(IO::File& file) // Reads an RGB color defined by three 32-bit floats
	{
	GLColor<GLfloat,3> result;
	file.read(result.getRgba(),3);
	return result;
	}

/**************
Helper classes:
**************/

class TextureMap // Class to represent texture maps and map vertex positions to texture coordinates
	{
	/* Embedded classes: */
	public:
	enum ProjectionMode // Enumerated type for texture projection modes
		{
		UNKNOWN,
		PLANAR,
		CYLINDRICAL,
		SPHERICAL,
		CUBIC,
		UVMAP
		};
	
	enum Flags // Enumerated type for texture flags
		{
		NONE=0x0,
		X_AXIS=0x1,
		Y_AXIS=0x2,
		Z_AXIS=0x4,
		AXIS_MASK=0x7,
		WORLD_COORDS=0x8,
		NEGATIVE_IMAGE=0x10,
		PIXEL_BLENDING=0x20,
		ANTIALIASING=0x40
		};
	
	enum WrapMode // Enumerated type for texture wrapping modes
		{
		BLACK=0,
		CLAMP=1,
		REPEAT=2,
		REPEAT_MIRROR=3
		};
	
	/* Elements: */
	std::string imageName; // Name of the texture image
	ProjectionMode projectionMode; // Texture projection mode
	unsigned int flags; // Texture flags bit field
	unsigned int wrapModes[2]; // Texture wrapping modes for width and height, respectively
	Vector size; // Texture size
	Point center; // Texture center
	Vector falloff; // ??
	Vector velocity; // ??
	Color color; // Texture color for color textures
	Scalar value; // Texture value
	
	/* Constructors and destructors: */
	public:
	TextureMap(void) // Creates a default texture map
		:projectionMode(UNKNOWN),flags(0x0U),
		 size(Vector::zero),center(Point::origin),
		 falloff(Vector::zero),velocity(Vector::zero),
		 color(0,0,0),value(0)
		{
		wrapModes[1]=wrapModes[0]=REPEAT;
		}
	
	/* Methods: */
	TexCoord calcTexCoord(const Point& point) const // Returns the texture coordinate assigned to the given vertex position
		{
		/* Scale the point position: */
		Point sp;
		for(int i=0;i<3;++i)
			sp[i]=(point[i]-center[i])/size[i];
		
		TexCoord result;
		switch(projectionMode)
			{
			case PLANAR:
				switch(flags&AXIS_MASK)
					{
					case X_AXIS:
						result[0]=sp[1]+Scalar(0.5);
						result[1]=sp[2]+Scalar(0.5);
						break;
					
					case Y_AXIS:
						result[0]=sp[0]+Scalar(0.5);
						result[1]=sp[1]+Scalar(0.5);
						break;
					
					case Z_AXIS:
						result[0]=sp[0]+Scalar(0.5);
						result[1]=sp[2]+Scalar(0.5);
						break;
					}
				break;
			
			case CYLINDRICAL:
				switch(flags&AXIS_MASK)
					{
					case X_AXIS:
						result[0]=Math::atan2(sp[1],sp[2])/(Scalar(2)*Math::Constants<Scalar>::pi)+Scalar(0.5);
						result[1]=sp[0]+Scalar(0.5);
						break;
					
					case Y_AXIS:
						result[0]=Math::atan2(sp[0],sp[1])/(Scalar(2)*Math::Constants<Scalar>::pi)+Scalar(0.5);
						result[1]=sp[2]+Scalar(0.5);
						break;
					
					case Z_AXIS:
						result[0]=Math::atan2(sp[0],sp[2])/(Scalar(2)*Math::Constants<Scalar>::pi)+Scalar(0.5);
						result[1]=sp[1]+Scalar(0.5);
						break;
					}
				break;
			
			case SPHERICAL:
				switch(flags&AXIS_MASK)
					{
					case X_AXIS:
						result[0]=Math::atan2(sp[1],sp[2])/(Scalar(2)*Math::Constants<Scalar>::pi)+Scalar(0.5);
						result[1]=Math::atan2(sp[0],Math::sqrt(sp[1]*sp[1]+sp[2]*sp[2]))/Math::Constants<Scalar>::pi+Scalar(0.5);
						break;
					
					case Y_AXIS:
						result[0]=Math::atan2(sp[0],sp[1])/(Scalar(2)*Math::Constants<Scalar>::pi)+Scalar(0.5);
						result[1]=Math::atan2(sp[2],Math::sqrt(sp[0]*sp[0]+sp[1]*sp[1]))/Math::Constants<Scalar>::pi+Scalar(0.5);
						break;
					
					case Z_AXIS:
						result[0]=Math::atan2(sp[0],sp[2])/(Scalar(2)*Math::Constants<Scalar>::pi)+Scalar(0.5);
						result[1]=Math::atan2(sp[1],Math::sqrt(sp[0]*sp[0]+sp[2]*sp[2]))/Math::Constants<Scalar>::pi+Scalar(0.5);
						break;
					}
				break;
			
			case CUBIC:
				{
				if(Math::abs(sp[0])>=Math::abs(sp[1])&&Math::abs(sp[0])>=Math::abs(sp[2]))
					{
					/* sp[0] is biggest: */
					result[0]=sp[1]/(sp[0]*Scalar(2))+Scalar(0.5);
					result[1]=sp[2]/(sp[0]*Scalar(2))+Scalar(0.5);
					}
				else if(Math::abs(sp[1])>=Math::abs(sp[2]))
					{
					/* sp[1] is biggest: */
					result[0]=sp[0]/(sp[1]*Scalar(2))+Scalar(0.5);
					result[1]=sp[2]/(sp[1]*Scalar(2))+Scalar(0.5);
					}
				else
					{
					/* sp[2] is biggest: */
					result[0]=sp[0]/(sp[2]*Scalar(2))+Scalar(0.5);
					result[1]=sp[1]/(sp[2]*Scalar(2))+Scalar(0.5);
					}
				break;
				}
			}
		
		return result;
		}
	};

struct Surface // Structure associating surface names with material properties and a face set
	{
	/* Embedded classes: */
	public:
	enum SurfaceFlags // Enumerated type for surface flags
		{
		NONE=0x0,
		LUMINOUS=0x1,
		OUTLINE=0x2,
		SMOOTHING=0x4,
		COLOR_HIGHLIGHTS=0x8,
		COLOR_FILTER=0x10,
		OPAQUE_EDGE=0x20,
		TRANSPARENT_EDGE=0x40,
		SHARP_TERMINATOR=0x80,
		DOUBLE_SIDED=0x100,
		ADDITIVE=0x200,
		SHADOW_ALPHA=0x400
		};
	
	/* Elements: */
	std::string name; // Surface's name
	ShapeNodePointer shape; // Pointer to Shape node representing the surface
	TextureMap diffTex; // Diffuse texture map
	Misc::Autopointer<IndexedFaceSetNode> faceSet; // Set of faces making up the surface
	};

}

void readLwobFile(IO::Directory& directory,IO::IFFChunk& formChunk,MeshFileNode& node)
	{
	/* Create a shared coordinate node: */
	CoordinateNodePointer coord=new CoordinateNode;
	MFPoint::ValueList& vertices=coord->point.getValues();
	
	/* Create a list of surfaces: */
	std::vector<Surface> surfaces;
	
	/* Process all chunks in the LWOB file: */
	while(!formChunk.eof())
		{
		/* Read the next chunk: */
		IO::IFFChunk chunk(&formChunk);
		chunk.ref();
		
		/* Process the chunk: */
		if(chunk.isChunk("PNTS"))
			{
			/* Read all vertices from the PNTS chunk: */
			while(!chunk.eof())
				vertices.push_back(readPoint(chunk));
			}
		else if(chunk.isChunk("SRFS"))
			{
			/* Create surfaces for all names contained in the SRFS chunk: */
			while(!chunk.eof())
				{
				/* Create a new surface with an empty indexed face set: */
				Surface surface;
				surface.name=readString(chunk);
				surface.faceSet=new IndexedFaceSetNode;
				surfaces.push_back(surface);
				}
			}
		else if(chunk.isChunk("SURF"))
			{
			/* Read the surface name and find the referenced surface: */
			std::string surfaceName=readString(chunk);
			std::vector<Surface>::iterator sIt;
			for(sIt=surfaces.begin();sIt!=surfaces.end()&&sIt->name!=surfaceName;++sIt)
				;
			if(sIt==surfaces.end())
				throw Misc::makeStdErr(0,"Undefined surface name %s",surfaceName);
			Surface& surface=*sIt;
			
			/* Initialize surface parameters: */
			Color color(1,1,1);
			unsigned int flags=0x0U;
			Scalar diff(1);
			Scalar spec(0);
			Scalar glos(0);
			Scalar lumi(0);
			Scalar refl(0);
			Scalar tran(0);
			Scalar bumpMapAmplitude(0);
			TextureMap* currTex=0;
			
			/* Process all subchunks of the SURF chunk: */
			while(!chunk.eof())
				{
				/* Read the next subchunk: */
				IO::IFFChunk surfChunk(&chunk,true);
				surfChunk.ref();
				
				/* Process the subchunk: */
				if(surfChunk.isChunk("COLR")) // Basic surface color
					color=readColor3ub(surfChunk);
				else if(surfChunk.isChunk("FLAG")) // Surface flags
					flags=surfChunk.read<Misc::UInt16>();
				else if(surfChunk.isChunk("DIFF")) // Diffuse factor as 8:8 fixed point
					diff=Scalar(surfChunk.read<Misc::UInt16>())/Scalar(256);
				else if(surfChunk.isChunk("VDIF")) // Diffuse factor as 32-bit float
					diff=Scalar(surfChunk.read<Misc::Float32>());
				else if(surfChunk.isChunk("SPEC")) // Specular factor as 8:8 fixed point
					spec=Scalar(surfChunk.read<Misc::UInt16>())/Scalar(256);
				else if(surfChunk.isChunk("VSPC")) // Specular factor as 32-bit float
					spec=Scalar(surfChunk.read<Misc::Float32>());
				else if(surfChunk.isChunk("GLOS")) // Glossiness as 16-bit integer
					glos=Scalar(surfChunk.read<Misc::UInt16>());
				else if(surfChunk.isChunk("LUMI")) // Luminous factor as 8:8 fixed point
					lumi=Scalar(surfChunk.read<Misc::UInt16>())/Scalar(256);
				else if(surfChunk.isChunk("VLUM")) // Luminous factor as 32-bit float
					lumi=Scalar(surfChunk.read<Misc::Float32>());
				else if(surfChunk.isChunk("REFL")) // Reflective factor as 8:8 fixed point
					refl=Scalar(surfChunk.read<Misc::UInt16>())/Scalar(256);
				else if(surfChunk.isChunk("VRFL")) // Reflective factor as 32-bit float
					refl=Scalar(surfChunk.read<Misc::Float32>());
				else if(surfChunk.isChunk("TRAN")) // Transparency as 8:8 fixed point
					tran=Scalar(surfChunk.read<Misc::UInt16>())/Scalar(256);
				else if(surfChunk.isChunk("VTRN")) // Transparency as 32-bit float
					tran=Scalar(surfChunk.read<Misc::Float32>());
				else if(surfChunk.isChunk("SMAN")) // Crease angle as 32-bit float
					surface.faceSet->creaseAngle.setValue(Scalar(surfChunk.read<Misc::Float32>()));
				else if(surfChunk.isChunk("CTEX")) // Following sub-chunks define color texture
					currTex=0;
				else if(surfChunk.isChunk("DTEX")) // Following sub-chunks define diffuse texture
					{
					/* Activate the diffuse texture: */
					currTex=&surface.diffTex;
					
					/* Read texture mapping parameters: */
					std::string textureType=readString(surfChunk);
					if(textureType=="Planar Image Map")
						currTex->projectionMode=TextureMap::PLANAR;
					else if(textureType=="Cylindrical Image Map")
						currTex->projectionMode=TextureMap::CYLINDRICAL;
					else if(textureType=="Spherical Image Map")
						currTex->projectionMode=TextureMap::SPHERICAL;
					else if(textureType=="Cubic Image Map")
						currTex->projectionMode=TextureMap::CUBIC;
					else
						Misc::formattedUserWarning("SceneGraph::readLwoFile: Invalid texture type %s in surface %s",textureType.c_str(),surface.name.c_str());
					}
				else if(surfChunk.isChunk("STEX")) // Following sub-chunks define specular texture
					currTex=0;
				else if(surfChunk.isChunk("LTEX")) // Following sub-chunks define luminous texture
					currTex=0;
				else if(surfChunk.isChunk("BTEX")) // Following sub-chunks define bump texture
					currTex=0;
				else if(surfChunk.isChunk("RTEX")) // Following sub-chunks define reflective texture
					currTex=0;
				else if(surfChunk.isChunk("TTEX")) // Following sub-chunks define transparency texture
					currTex=0;
				else if(surfChunk.isChunk("TIMG")&&currTex!=0) // Texture image file name
					currTex->imageName=readString(surfChunk);
				else if(surfChunk.isChunk("TFLG")&&currTex!=0) // Texture flags
					currTex->flags=surfChunk.read<Misc::UInt16>();
				else if(surfChunk.isChunk("TWRP")&&currTex!=0) // Texture wrapping modes
					{
					for(int i=0;i<2;++i)
						currTex->wrapModes[i]=surfChunk.read<Misc::UInt16>();
					}
				else if(surfChunk.isChunk("TSIZ")&&currTex!=0) // Texture size
					currTex->size=readVector(surfChunk);
				else if(surfChunk.isChunk("TCTR")&&currTex!=0) // Texture center
					currTex->center=readPoint(surfChunk);
				else if(surfChunk.isChunk("TFAL")&&currTex!=0) // Texture fall-off
					currTex->falloff=readVector(surfChunk);
				else if(surfChunk.isChunk("TVEL")&&currTex!=0) // Texture velocity
					currTex->velocity=readVector(surfChunk);
				else if(surfChunk.isChunk("TCLR")&&currTex!=0) // Texture color
					currTex->color=readColor3ub(surfChunk);
				else if(surfChunk.isChunk("TVAL")&&currTex!=0) // Texture value as 8:8 fixed point
					currTex->value=Scalar(surfChunk.read<Misc::UInt16>())/Scalar(256);
				else if(surfChunk.isChunk("TAMP")) // Bump texture map amplitude
					bumpMapAmplitude=Scalar(surfChunk.read<Misc::Float32>());
				}
			
			/* Create a shape node to represent the surface: */
			surface.shape=new ShapeNode;
			
			/* Create an appearance node based on surface parameters: */
			AppearanceNodePointer appearance=new AppearanceNode;
			
			MaterialNodePointer material=new MaterialNode;
			material->ambientIntensity.setValue(1);
			material->diffuseColor.setValue(color*diff);
			material->specularColor.setValue(color*spec);
			material->shininess.setValue(Math::min(glos/Scalar(128),Scalar(1)));
			material->emissiveColor.setValue(color*lumi);
			material->transparency.setValue(tran);
			material->update();
			
			appearance->material.setValue(material);
			
			if(!surface.diffTex.imageName.empty())
				{
				/* Extract the texture image's file name: */
				std::string::iterator fnIt=surface.diffTex.imageName.end();
				for(std::string::iterator sIt=surface.diffTex.imageName.begin();sIt!=surface.diffTex.imageName.end();++sIt)
					if(*sIt=='\\')
						fnIt=sIt+1;
				
				/* Create an image texture node: */
				ImageTextureNode* imageTexture=new ImageTextureNode;
				imageTexture->setUrl(std::string(fnIt,surface.diffTex.imageName.end()),directory);
				imageTexture->repeatS.setValue(surface.diffTex.wrapModes[0]==TextureMap::REPEAT);
				imageTexture->repeatT.setValue(surface.diffTex.wrapModes[1]==TextureMap::REPEAT);
				imageTexture->filter.setValue(true);
				imageTexture->update();
				
				appearance->texture.setValue(imageTexture);
				}
			
			appearance->update();
			
			surface.shape->appearance.setValue(appearance);
			
			/* Create an indexed face set to hold the surface's polygons: */
			surface.faceSet->coord.setValue(coord);
			surface.faceSet->normalPerVertex.setValue(true);
			surface.faceSet->ccw.setValue(node.ccw.getValue());
			surface.faceSet->convex.setValue(node.convex.getValue());
			surface.faceSet->solid.setValue((flags&Surface::DOUBLE_SIDED)==0x0U);
			
			surface.shape->geometry.setValue(surface.faceSet);
			}
		else if(chunk.isChunk("POLS"))
			{
			/* Create a buffer for polygon vertex indices: */
			std::vector<unsigned int> pvis;
			
			/* Read all polygons from the chunk: */
			while(!chunk.eof())
				{
				/* Read the polygon's vertex indices: */
				unsigned int numVertices=chunk.read<Misc::UInt16>();
				pvis.reserve(numVertices);
				for(unsigned int i=0;i<numVertices;++i)
					pvis.push_back(chunk.read<Misc::UInt16>());
				
				/* Read the surface index: */
				int surfaceNumber=chunk.read<Misc::SInt16>();
				
				/* Check if the polygon has detail polygons: */
				if(surfaceNumber<0)
					{
					/* Read (and ignore) all detail polygons: */
					unsigned int numDetailPolygons=chunk.read<Misc::UInt16>();
					for(unsigned int i=0;i<numDetailPolygons;++i)
						{
						/* Read the number of vertices in the detail polygon: */
						unsigned int numDetailVertices=chunk.read<Misc::UInt16>();
						
						/* Skip the detail polygon's vertices: */
						chunk.skip<Misc::UInt16>(numDetailVertices);
						
						/* Skip the detail polygon's surface number: */
						chunk.skip<Misc::SInt16>(1);
						}
					
					surfaceNumber=-surfaceNumber;
					}
				
				/* Add the polygon to its surface's face set and flip the vertex order from clockwise to counter-clockwise: */
				Surface& surface=surfaces[surfaceNumber-1];
				MFInt::ValueList& coordIndex=surface.faceSet->coordIndex.getValues();
				if(!coordIndex.empty())
					coordIndex.push_back(-1);
				coordIndex.push_back(pvis[1]);
				coordIndex.push_back(pvis[0]);
				for(unsigned int i=2;i<numVertices;++i)
					coordIndex.push_back(pvis[numVertices+1-i]);
				
				pvis.clear();
				}
			}
		}
	
	/* Finalize all surfaces and add their respective shape nodes to the mesh file node: */
	coord->update();
	for(std::vector<Surface>::iterator sIt=surfaces.begin();sIt!=surfaces.end();++sIt)
		{
		/* Check if the surface has a diffuse texture: */
		if(!sIt->diffTex.imageName.empty())
			{
			/* Find the range of vertex indices used by the surface: */
			MFInt::ValueList& coordIndex=sIt->faceSet->coordIndex.getValues();
			int viMin=Math::Constants<int>::max;
			int viMax=-1;
			for(MFInt::ValueList::iterator ciIt=coordIndex.begin();ciIt!=coordIndex.end();++ciIt)
				if(*ciIt>=0)
					{
					if(viMin>*ciIt)
						viMin=*ciIt;
					if(viMax<*ciIt)
						viMax=*ciIt;
					}
			
			/* Create a map from vertex indices to texture coordinate indices and calculate texture coordinates: */
			int* tcis=new int[viMax+1-viMin];
			int* tciEnd=tcis+(viMax+1-viMin);
			for(int* tciPtr=tcis;tciPtr!=tciEnd;++tciPtr)
				*tciPtr=-1;
			TextureCoordinateNodePointer texCoord=new TextureCoordinateNode;
			int nextTcIndex=0;
			for(MFInt::ValueList::iterator ciIt=coordIndex.begin();ciIt!=coordIndex.end();++ciIt)
				if(*ciIt>=0&&tcis[*ciIt-viMin]==-1)
					{
					/* Calculate the vertex's texture coordinate: */
					texCoord->point.appendValue(sIt->diffTex.calcTexCoord(coord->point.getValue(*ciIt)));
					
					/* Assign a texture coordinate index: */
					tcis[*ciIt-viMin]=nextTcIndex++;
					}
			texCoord->update();
			
			sIt->faceSet->texCoord.setValue(texCoord);
			
			/* Create the texture coordinate index list: */
			MFInt::ValueList& texCoordIndex=sIt->faceSet->texCoordIndex.getValues();
			texCoordIndex.reserve(coordIndex.size());
			for(MFInt::ValueList::iterator ciIt=coordIndex.begin();ciIt!=coordIndex.end();++ciIt)
				if(*ciIt>=0)
					texCoordIndex.push_back(tcis[*ciIt-viMin]);
				else
					texCoordIndex.push_back(-1);
			}
		
		/* Finalize the surface's scene graph: */
		sIt->faceSet->update();
		sIt->shape->update();
		
		/* Add the surface's scene graph to the mesh file node: */
		node.addShape(*sIt->shape);
		}
	}

void readLwo2File(IO::Directory& directory,IO::IFFChunk& formChunk,MeshFileNode& node)
	{
	}

void readLwoFile(const IO::Directory& directory,const std::string& fileName,MeshFileNode& node)
	{
	/* Read and process the input file's root FORM chunk: */
	IO::IFFChunk formChunk(directory.openFile(fileName.c_str()));
	formChunk.ref();
	if(!formChunk.isChunk("FORM"))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"File %s is not a valid IFF file",directory.getPath(fileName.c_str()).c_str());
	
	/* Get the base directory containing the LWO file: */
	IO::DirectoryPtr lwoDir=directory.openFileDirectory(fileName.c_str());
	
	/* Check the FORM chunk type: */
	char formChunkType[4];
	formChunk.read(formChunkType,4);
	if(memcmp(formChunkType,"LWOB",4)==0)
		{
		try
			{
			/* Read a Lightwave Object file prior to version 6.0: */
			readLwobFile(*lwoDir,formChunk,node);
			}
		catch(const std::runtime_error& err)
			{
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Caught exception \"%s\" while reading file %s",err.what(),directory.getPath(fileName.c_str()).c_str());
			}
		}
	else if(memcmp(formChunkType,"LWO2",4)==0)
		{
		try
			{
			/* Read a Lightwave Object file version 6.0 and up: */
			readLwo2File(*lwoDir,formChunk,node);
			}
		catch(const std::runtime_error& err)
			{
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Caught exception \"%s\" while reading file %s",err.what(),directory.getPath(fileName.c_str()).c_str());
			}
		}
	else
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"File %s is not a valid LWO file",directory.getPath(fileName.c_str()).c_str());
	}

}
