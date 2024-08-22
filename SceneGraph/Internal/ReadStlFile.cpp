/***********************************************************************
ReadStlFile - Helper function to read a 3D polygon file in STL format
into a shape node.
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

#include <SceneGraph/Internal/ReadStlFile.h>

#include <string.h>
#include <string>
#include <stdexcept>
#include <Misc/SizedTypes.h>
#include <Misc/StdError.h>
#include <Misc/HashTable.h>
#include <IO/File.h>
#include <IO/Directory.h>
#include <IO/ValueSource.h>
#include <SceneGraph/ColorNode.h>
#include <SceneGraph/NormalNode.h>
#include <SceneGraph/CoordinateNode.h>
#include <SceneGraph/IndexedFaceSetNode.h>
#include <SceneGraph/ShapeNode.h>
#include <SceneGraph/MeshFileNode.h>

namespace SceneGraph {

namespace {

class PointHasher
	{
	/* Static methods: */
	public:
	static size_t hash(const Point& source,size_t tableSize)
		{
		size_t result=*reinterpret_cast<const Misc::UInt32*>(source.getComponents()+0);
		result<<=16;
		result|=*reinterpret_cast<const Misc::UInt32*>(source.getComponents()+1);
		result<<=16;
		result|=*reinterpret_cast<const Misc::UInt32*>(source.getComponents()+2);
		return result%tableSize;
		}
	};

}

void readStlFile(const IO::Directory& directory,const std::string& fileName,MeshFileNode& node)
	{
	/* Open the STL file: */
	IO::FilePtr stlFile=directory.openFile(fileName.c_str());
	
	try
		{
		/* Create nodes to collect data read from the STL file: */
		ColorNodePointer color=new ColorNode;
		MFColor::ValueList& colors=color->color.getValues();
		bool haveColor=false;
		NormalNodePointer normal=new NormalNode;
		MFVector::ValueList& normals=normal->vector.getValues();
		CoordinateNodePointer coord=new CoordinateNode;
		MFPoint::ValueList& points=coord->point.getValues();
		Misc::Autopointer<IndexedFaceSetNode> indexedFaceSet=new IndexedFaceSetNode;
		MFInt::ValueList& coordIndices=indexedFaceSet->coordIndex.getValues();
		
		/* Create a hash table to map triangle vertices to coordinate indices: */
		typedef Misc::HashTable<Point,int,PointHasher> VertexMap;
		VertexMap vertexMap(101);
		int nextVertexIndex=0;
		
		/* Check if the file is in ASCII format: */
		char solid[7];
		stlFile->readRaw(solid,6);
		solid[6]='\0';
		if(strcmp(solid,"solid ")==0)
			{
			/*******************************************************************
			Attach a value source to the STL file and read it in ASCII format:
			*******************************************************************/
			
			IO::ValueSource stlReader(stlFile);
			
			/* Skip the rest of the ASCII STL header: */
			stlReader.skipLine();
			stlReader.skipWs();
			
			/* Read triangles from the file until the "endsolid" tag: */
			while(!stlReader.eof())
				{
				std::string tag=stlReader.readString();
				if(tag=="facet")
					{
					/* Read a triangle definition: */
					
					/* Read the triangle's normal vector: */
					if(!stlReader.isLiteral("normal"))
						throw std::runtime_error("Missing normal vector in triangle");
					Vector normal;
					for(int i=0;i<3;++i)
						normal[i]=stlReader.readNumber();
					normals.push_back(normal);
					
					/* Read the triangle's vertex loop: */
					if(!(stlReader.isLiteral("outer")&&stlReader.isLiteral("loop")))
						throw std::runtime_error("Missing vertex loop in triangle");
					for(int i=0;i<3;++i)
						{
						/* Read a vertex: */
						if(!stlReader.isLiteral("vertex"))
							throw std::runtime_error("Missing vertex in vertex loop");
						Point p;
						for(int j=0;j<3;++j)
							p[j]=stlReader.readNumber();
						
						/* Get the vertex's index: */
						VertexMap::Iterator vmIt=vertexMap.findEntry(p);
						if(!vmIt.isFinished())
							coordIndices.push_back(vmIt->getDest());
						else
							{
							points.push_back(p);
							vertexMap.setEntry(VertexMap::Entry(p,nextVertexIndex));
							coordIndices.push_back(nextVertexIndex);
							++nextVertexIndex;
							}
						}
					coordIndices.push_back(-1);
					
					if(!stlReader.isLiteral("endloop"))
						throw std::runtime_error("Missing vertex loop end marker");
					
					if(!stlReader.isLiteral("endfacet"))
						throw std::runtime_error("Missing triangle end marker");
					}
				else if(tag=="endsolid")
					{
					/* Skip the solid name and finish reading: */
					stlReader.skipLine();
					stlReader.skipWs();
					break;
					}
				else
					throw std::runtime_error("Invalid tag");
				}
			}
		else
			{
			/*******************************************************************
			Read the STL file in binary format:
			*******************************************************************/
			
			/* Binary STL files are always little endian: */
			stlFile->setEndianness(Misc::LittleEndian);
			
			/* Skip the unused 80-byte header: */
			stlFile->skip<char>(80-6);
			
			/* Read the number of triangles in the file: */
			size_t numTriangles=stlFile->read<Misc::UInt32>();
			colors.reserve(numTriangles);
			normals.reserve(numTriangles);
			points.reserve(numTriangles*3);
			coordIndices.reserve(numTriangles*4);
			
			/* Read all triangles: */
			for(size_t triangleIndex=0;triangleIndex<numTriangles;++triangleIndex)
				{
				/* Read the normal vector: */
				Vector normal;
				for(int i=0;i<3;++i)
					normal[i]=Scalar(stlFile->read<Misc::Float32>());
				normals.push_back(normal);
				
				/* Read the triangle's vertices: */
				for(int i=0;i<3;++i)
					{
					Point p;
					for(int j=0;j<3;++j)
						p[j]=Scalar(stlFile->read<Misc::Float32>());
					
					/* Get the vertex's index: */
					VertexMap::Iterator vmIt=vertexMap.findEntry(p);
					if(!vmIt.isFinished())
						coordIndices.push_back(vmIt->getDest());
					else
						{
						points.push_back(p);
						vertexMap.setEntry(VertexMap::Entry(p,nextVertexIndex));
						coordIndices.push_back(nextVertexIndex);
						++nextVertexIndex;
						}
					}
				
				/* Read the attribute byte count: */
				unsigned int attributeSize=stlFile->read<Misc::UInt16>();
				
				/* Interpret the attribute byte count as an RGB color: */
				unsigned int red=attributeSize&0x1f;
				attributeSize>>=5;
				unsigned int green=attributeSize&0x1f;
				attributeSize>>=5;
				unsigned int blue=attributeSize&0x1f;
				attributeSize>>=5;
				if(attributeSize!=0x0)
					{
					colors.push_back(Color(float(red)/31.0f,float(green)/31.0f,float(blue)/31.0f));
					haveColor=true;
					}
				else
					colors.push_back(Color(1.0f,1.0f,1.0f));
				
				/* Add the triangle to the indexed face set: */
				coordIndices.push_back(-1);
				}
			}
		
		/* Attach the property nodes to the face set node: */
		if(haveColor)
			{
			color->update();
			indexedFaceSet->color.setValue(color);
			indexedFaceSet->colorPerVertex.setValue(false);
			}
		if(node.creaseAngle.getValue()==Scalar(0))
			{
			normal->update();
			indexedFaceSet->normal.setValue(normal);
			indexedFaceSet->normalPerVertex.setValue(false);
			}
		else
			indexedFaceSet->normalPerVertex.setValue(true);
		coord->update();
		indexedFaceSet->coord.setValue(coord);
		
		/* Copy face set parameters from the mesh file node: */
		indexedFaceSet->ccw.setValue(node.ccw.getValue());
		indexedFaceSet->convex.setValue(true);
		indexedFaceSet->solid.setValue(node.solid.getValue());
		indexedFaceSet->creaseAngle.setValue(node.creaseAngle.getValue());
		
		/* Create a shape node: */
		ShapeNodePointer shape=new ShapeNode;
		
		/* Set the shape node's appearance to the mesh file node's appearance: */
		shape->appearance.setValue(node.appearance.getValue());
		
		/* Set the shape node's geometry to the indexed face set: */
		indexedFaceSet->update();
		shape->geometry.setValue(indexedFaceSet);
		
		/* Finalize the shape node and add it to the mesh file node's shape list: */
		shape->update();
		node.addShape(*shape);
		}
	catch(const std::runtime_error& err)
		{
		/* Wrap and re-throw the exception: */
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error %s while reading STL file %s",err.what(),fileName.c_str());
		}
	}

}
