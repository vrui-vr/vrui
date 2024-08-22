/***********************************************************************
VRMLFile - Class to represent a VRML 2.0 file and state required to
parse its contents.
Copyright (c) 2009-2024 Oliver Kreylos

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

#include <SceneGraph/VRMLFile.h>

#include <stdlib.h>
#include <Misc/FileNameExtensions.h>
#include <Misc/StringPrintf.h>
#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>
#include <IO/OpenFile.h>
#include <Geometry/ComponentArray.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Rotation.h>
#include <SceneGraph/EventTypes.h>
#include <SceneGraph/NodeCreator.h>
#include <SceneGraph/GraphNode.h>

namespace SceneGraph {

namespace {

/*************************************************************************
Helper function to parse route statements (will move into VRMLFile class):
*************************************************************************/

void parseRoute(VRMLFile& vrmlFile)
	{
	/* Read the event source name: */
	const char* source=vrmlFile.getToken();
	
	/* Split the event source into node name and field name: */
	const char* periodPtr=0;
	for(const char* sPtr=source;*sPtr!='\0';++sPtr)
		if(*sPtr=='.')
			{
			if(periodPtr!=0)
				throw VRMLFile::ParseError(vrmlFile,Misc::stringPrintf("multiple periods in event source %s",source));
			periodPtr=sPtr;
			}
	if(periodPtr==0)
		throw VRMLFile::ParseError(vrmlFile,Misc::stringPrintf("missing period in event source %s",source));
	
	/* Retrieve the event source: */
	EventOut* eventOut=0;
	try
		{
		std::string sourceNode(source,periodPtr);
		eventOut=vrmlFile.useNode(sourceNode.c_str())->getEventOut(periodPtr+1);
		}
	catch(const Node::FieldError& err)
		{
		throw VRMLFile::ParseError(vrmlFile,Misc::stringPrintf("unknown field \"%s\" in event source",periodPtr+1));
		}
	vrmlFile.readNextToken();
	
	/* Check the TO keyword: */
	if(!vrmlFile.isToken("TO"))
		throw VRMLFile::ParseError(vrmlFile,"missing TO keyword in route definition");
	vrmlFile.readNextToken();
	
	/* Read the event sink name: */
	const char* sink=vrmlFile.getToken();
	
	/* Split the event sink into node name and field name: */
	periodPtr=0;
	for(const char* sPtr=sink;*sPtr!='\0';++sPtr)
		if(*sPtr=='.')
			{
			if(periodPtr!=0)
				throw VRMLFile::ParseError(vrmlFile,Misc::stringPrintf("multiple periods in event sink %s",sink));
			periodPtr=sPtr;
			}
	if(periodPtr==0)
		throw VRMLFile::ParseError(vrmlFile,Misc::stringPrintf("missing period in event sink %s",sink));
	
	/* Retrieve the event sink: */
	EventIn* eventIn=0;
	try
		{
		std::string sinkNode(sink,periodPtr);
		eventIn=vrmlFile.useNode(sinkNode.c_str())->getEventIn(periodPtr+1);
		}
	catch(const Node::FieldError& err)
		{
		throw VRMLFile::ParseError(vrmlFile,Misc::stringPrintf("unknown field \"%s\" in event sink",periodPtr+1));
		}
	vrmlFile.readNextToken();
	
	/* Create a route: */
	Route* route=0;
	try
		{
		route=eventOut->connectTo(eventIn);
		}
	catch(const Route::TypeMismatchError& err)
		{
		throw VRMLFile::ParseError(vrmlFile,"mismatching field types in route definition");
		}
	
	/* For now, just delete the route again: */
	delete route;
	}

/********************************************************************
Helper functions to parse floating-point values and component arrays:
********************************************************************/

template <class ScalarParam>
inline
ScalarParam
parseFloatingPoint(
	VRMLFile& vrmlFile)
	{
	/* Convert the current token to floating-point: */
	char* endPtr=0;
	ScalarParam result=ScalarParam(strtod(vrmlFile.getToken(),&endPtr));
	
	/* Ensure that the entire token was converted: */
	if(endPtr!=vrmlFile.getToken()+vrmlFile.getTokenSize())
		throw VRMLFile::ParseError(vrmlFile,Misc::stringPrintf("%s is not a valid floating-point value",vrmlFile.getToken()));
	
	/* Read the next token: */
	vrmlFile.readNextToken();
	
	return result;
	}

template <class ComponentArrayParam>
inline
void
parseComponentArray(
	ComponentArrayParam& value,
	VRMLFile& vrmlFile)
	{
	/* Parse the components of the given component array: */
	for(int i=0;i<ComponentArrayParam::dimension;++i)
		{
		/* Convert the current token to floating-point: */
		char* endPtr=0;
		value[i]=typename ComponentArrayParam::Scalar(strtod(vrmlFile.getToken(),&endPtr));
		
		/* Ensure that the entire token was converted: */
		if(endPtr!=vrmlFile.getToken()+vrmlFile.getTokenSize())
			throw VRMLFile::ParseError(vrmlFile,Misc::stringPrintf("%s is not a valid floating-point value",vrmlFile.getToken()));
		
		/* Read the next token: */
		vrmlFile.readNextToken();
		}
	}

/***********************************************************
Templatized helper class to parse values from token sources:
***********************************************************/

template <class ValueParam>
class ValueParser // Generic class to parse values from token sources
	{
	};

template <>
class ValueParser<bool>
	{
	/* Methods: */
	public:
	static bool parseValue(VRMLFile& vrmlFile)
		{
		/* Parse the current token's value: */
		bool result;
		if(vrmlFile.isToken("TRUE"))
			result=true;
		else if(vrmlFile.isToken("FALSE"))
			result=false;
		else
			throw VRMLFile::ParseError(vrmlFile,Misc::stringPrintf("%s is not a valid boolean value",vrmlFile.getToken()));
		
		/* Read the next token: */
		vrmlFile.readNextToken();
		
		return result;
		}
	};

template <>
class ValueParser<std::string>
	{
	/* Methods: */
	public:
	static std::string parseValue(VRMLFile& vrmlFile)
		{
		/* Return the current token: */
		std::string result(vrmlFile.getToken());
		
		/* Read the next token: */
		vrmlFile.readNextToken();
		
		return result;
		}
	};

template <>
class ValueParser<int>
	{
	/* Methods: */
	public:
	static int parseValue(VRMLFile& vrmlFile)
		{
		/* Convert the current token to integer: */
		char* endPtr=0;
		int result=int(strtol(vrmlFile.getToken(),&endPtr,10));
		
		/* Ensure that the entire token was converted: */
		if(endPtr!=vrmlFile.getToken()+vrmlFile.getTokenSize())
			throw VRMLFile::ParseError(vrmlFile,Misc::stringPrintf("%s is not a valid integer value",vrmlFile.getToken()));
		
		/* Read the next token: */
		vrmlFile.readNextToken();
		
		return result;
		}
	};

template <>
class ValueParser<Scalar>
	{
	/* Methods: */
	public:
	static Scalar parseValue(VRMLFile& vrmlFile)
		{
		return parseFloatingPoint<Scalar>(vrmlFile);
		}
	};

template <>
class ValueParser<double>
	{
	/* Methods: */
	public:
	static double parseValue(VRMLFile& vrmlFile)
		{
		return parseFloatingPoint<double>(vrmlFile);
		}
	};

template <class ScalarParam,int dimensionParam>
class ValueParser<Geometry::ComponentArray<ScalarParam,dimensionParam> >
	{
	/* Methods: */
	public:
	static Geometry::ComponentArray<ScalarParam,dimensionParam> parseValue(VRMLFile& vrmlFile)
		{
		Geometry::ComponentArray<ScalarParam,dimensionParam> result;
		parseComponentArray(result,vrmlFile);
		return result;
		}
	};

template <class ScalarParam,int dimensionParam>
class ValueParser<Geometry::Point<ScalarParam,dimensionParam> >
	{
	/* Methods: */
	public:
	static Geometry::Point<ScalarParam,dimensionParam> parseValue(VRMLFile& vrmlFile)
		{
		Geometry::Point<ScalarParam,dimensionParam> result;
		parseComponentArray(result,vrmlFile);
		return result;
		}
	};

template <class ScalarParam,int dimensionParam>
class ValueParser<Geometry::Vector<ScalarParam,dimensionParam> >
	{
	/* Methods: */
	public:
	static Geometry::Vector<ScalarParam,dimensionParam> parseValue(VRMLFile& vrmlFile)
		{
		Geometry::Vector<ScalarParam,dimensionParam> result;
		parseComponentArray(result,vrmlFile);
		return result;
		}
	};

template <>
class ValueParser<Rotation>
	{
	/* Methods: */
	public:
	static Rotation parseValue(VRMLFile& vrmlFile)
		{
		/* Parse the rotation axis: */
		Rotation::Vector axis;
		parseComponentArray(axis,vrmlFile);
		
		/* Parse the rotation angle: */
		Rotation::Scalar angle=parseFloatingPoint<Rotation::Scalar>(vrmlFile);
		
		/* Return the rotation: */
		return Rotation::rotateAxis(axis,angle);
		}
	};

template <class ScalarParam,int numComponentsParam>
class ValueParser<GLColor<ScalarParam,numComponentsParam> >
	{
	/* Methods: */
	public:
	static GLColor<ScalarParam,numComponentsParam> parseValue(VRMLFile& vrmlFile)
		{
		/* Parse the color's components: */
		GLColor<ScalarParam,numComponentsParam> result;
		for(int i=0;i<numComponentsParam;++i)
			result[i]=parseFloatingPoint<ScalarParam>(vrmlFile);
		
		return result;
		}
	};

template <>
class ValueParser<TexCoord>
	{
	/* Methods: */
	public:
	static TexCoord parseValue(VRMLFile& vrmlFile)
		{
		TexCoord result;
		parseComponentArray(result,vrmlFile);
		return result;
		}
	};

template <>
class ValueParser<NodePointer>
	{
	/* Methods: */
	public:
	static NodePointer parseValue(VRMLFile& vrmlFile)
		{
		NodePointer result;
		
		/* Read the node type name: */
		if(vrmlFile.isToken("ROUTE"))
			{
			/* Parse a route statement: */
			parseRoute(vrmlFile);
			}
		else if(vrmlFile.isToken("USE"))
			{
			/* Retrieve a named node from the VRML file: */
			result=vrmlFile.useNode(vrmlFile.readNextToken());
			vrmlFile.readNextToken();
			}
		else
			{
			/* Check for the optional DEF keyword: */
			std::string defName;
			if(vrmlFile.isToken("DEF"))
				{
				/* Read the new node name: */
				defName=vrmlFile.readNextToken();
				
				/* Read the node type name: */
				vrmlFile.readNextToken();
				}
			
			if(!vrmlFile.isToken("NULL"))
				{
				/* Create the result node: */
				if((result=vrmlFile.createNode(vrmlFile.getToken()))!=0)
					{
					/* Check for and skip the opening brace: */
					vrmlFile.readNextToken();
					if(!vrmlFile.isToken('{'))
						throw VRMLFile::ParseError(vrmlFile,"Missing opening brace in node definition");
					
					/* Parse fields until the matching closing brace or end of file: */
					vrmlFile.readNextToken();
					while(!vrmlFile.eof()&&!vrmlFile.isToken('}'))
						{
						if(vrmlFile.isToken("ROUTE"))
							{
							/* Parse a route statement: */
							vrmlFile.readNextToken();
							parseRoute(vrmlFile);
							}
						else
							{
							/* Parse a field value: */
							std::string fieldName(vrmlFile.getToken());
							vrmlFile.readNextToken();
							result->parseField(fieldName.c_str(),vrmlFile);
							}
						}
					
					/* Check for and skip the closing brace: */
					if(!vrmlFile.isToken('}'))
						throw VRMLFile::ParseError(vrmlFile,"Missing closing brace in node definition");
					
					/* Finalize the node: */
					result->update();
					}
				else
					{
					/* Don't throw an exception; instead, try to cleanly skip the unknown node: */
					// throw VRMLFile::ParseError(vrmlFile,Misc::stringPrintf("Unknown node type %s",vrmlFile.getToken()));
					VRMLFile::ParseError error(vrmlFile,Misc::stringPrintf("Unknown node type %s",vrmlFile.getToken()));
					Misc::formattedUserWarning("SceneGraph::VRMLFile: %s",error.what());
					
					/* Check for and skip the opening brace: */
					vrmlFile.readNextToken();
					if(!vrmlFile.isToken('{'))
						throw VRMLFile::ParseError(vrmlFile,"Missing opening brace in node definition");
					
					/* Skip from the file until the matching closing brace is found or end of file occurs: */
					unsigned int braceDepth=1;
					vrmlFile.readNextToken();
					while(!vrmlFile.eof()&&(braceDepth>1||!vrmlFile.isToken('}')))
						{
						/* Process the next token: */
						if(vrmlFile.isToken('{')||vrmlFile.isToken('['))
							++braceDepth;
						else if(vrmlFile.isToken('}')||vrmlFile.isToken(']'))
							--braceDepth;
						}
					
					/* Check for and skip the closing brace: */
					if(!vrmlFile.isToken('}'))
						throw VRMLFile::ParseError(vrmlFile,"Missing closing brace in node definition");
					}
				}
			vrmlFile.readNextToken();
			
			if(!defName.empty())
				{
				/* Store the named node in the VRML file: */
				vrmlFile.defineNode(defName.c_str(),result);
				}
			}
		
		return result;
		}
	};

/***********************************************************
Templatized helper class to parse fields from token sources:
***********************************************************/

template <class FieldParam>
class FieldParser
	{
	};

/**************************************
Specialization for single-value fields:
**************************************/

template <class ValueParam>
class FieldParser<SF<ValueParam> >
	{
	/* Methods: */
	public:
	static void parseField(SF<ValueParam>& field,VRMLFile& vrmlFile)
		{
		/* Just read the field's value: */
		field.setValue(ValueParser<ValueParam>::parseValue(vrmlFile));
		}
	};

/*************************************
Specialization for multi-value fields:
*************************************/

template <class ValueParam>
class FieldParser<MF<ValueParam> >
	{
	/* Methods: */
	public:
	static void parseField(MF<ValueParam>& field,VRMLFile& vrmlFile)
		{
		/* Clear the field: */
		field.clearValues();
		
		/* Check for opening bracket: */
		if(vrmlFile.isToken('['))
			{
			/* Skip the opening bracket: */
			vrmlFile.readNextToken();
			
			/* Read a list of values: */
			while(!vrmlFile.eof()&&!vrmlFile.isToken(']'))
				{
				/* Read a single value: */
				field.appendValue(ValueParser<ValueParam>::parseValue(vrmlFile));
				}
			
			/* Skip the closing bracket: */
			if(!vrmlFile.isToken(']'))
				throw VRMLFile::ParseError(vrmlFile,"Missing closing bracket in multi-valued field");
			vrmlFile.readNextToken();
			}
		else
			{
			/* Read a single value: */
			field.appendValue(ValueParser<ValueParam>::parseValue(vrmlFile));
			}
		}
	};

}

/*************************************
Methods of class VRMLFile::ParseError:
*************************************/

VRMLFile::ParseError::ParseError(const VRMLFile& vrmlFile,const std::string& error)
	:std::runtime_error(Misc::stringPrintf("%s, line %u: %s",vrmlFile.sourceUrl.c_str(),(unsigned int)vrmlFile.currentLine,error.c_str()))
	{
	}

VRMLFile::ParseError::ParseError(const VRMLFile& vrmlFile,const char* error)
	:std::runtime_error(Misc::stringPrintf("%s, line %u: %s",vrmlFile.sourceUrl.c_str(),(unsigned int)vrmlFile.currentLine,error))
	{
	}

/*************************
Methods of class VRMLFile:
*************************/

void VRMLFile::init(void)
	{
	/* Initialize the token source: */
	setWhitespace(',',true); // Comma is treated as whitespace
	setPunctuation("#[]{}\n"); // Newline is treated as punctuation to count lines
	setQuotes("\""); // VRML specification only allows double quotes
	
	/* Initialize error tracking: */
	currentLine=1;
	
	/* Check the VRML file header: */
	bool valid=true;
	if(valid)
		IO::TokenSource::readNextToken();
	valid=valid&&isToken('#');
	if(valid)
		IO::TokenSource::readNextToken();
	valid=valid&&isToken("VRML");
	if(valid)
		IO::TokenSource::readNextToken();
	valid=valid&&isToken("V2.0");
	if(valid)
		IO::TokenSource::readNextToken();
	valid=valid&&isToken("utf8");
	if(!valid)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"%s is not a valid VRML 2.0 file",sourceUrl.c_str());
	
	/* Skip the rest of the header line, which is a comment after all: */
	skipLine();
	skipWs();
	
	/* Create the root node name map: */
	nodeMapStack.push_back(NodeMap(5));
	
	/* Create a new prototype scope: */
	nodeCreator.startPrototypeScope(false);
	}

void VRMLFile::parseProto(void)
	{
	/* Read the prototype name: */
	std::string protoName(getToken());
	readNextToken();
	
	/* Parse the interface declaration: */
	if(!isToken('['))
		throw VRMLFile::ParseError(*this,"missing interface declaration in PROTO definition");
	readNextToken();
	if(!isToken(']'))
		throw VRMLFile::ParseError(*this,"non-empty interface declaration in PROTO definition");
	readNextToken();
	
	/* Parse the prototype body: */
	if(!isToken('{'))
		throw VRMLFile::ParseError(*this,"missing body in PROTO definition");
	readNextToken();
	
	/* Start new scopes for local prototype definitions and DEF/USE pairs: */
	nodeCreator.startPrototypeScope(true);
	nodeMapStack.push_back(NodeMap(5));
	
	MFNode bodyNodes;
	while(!eof()&&!isToken('}'))
		{
		if(isToken("PROTO"))
			{
			/* Parse a prototype definition: */
			readNextToken();
			parseProto();
			}
		else if(isToken("EXTERNPROTO"))
			{
			/* Parse an external prototype definition: */
			readNextToken();
			parseExternProto();
			}
		else
			{
			/* Parse a node and add it to the list of body nodes: */
			bodyNodes.appendValue(parseValue<NodePointer>());
			}
		}
	if(!isToken('}'))
		throw VRMLFile::ParseError(*this,"unterminated body in PROTO definition");
	readNextToken();
	
	/* Close the local prototype and node name scopes: */
	nodeCreator.closePrototypeScope();
	nodeMapStack.pop_back();
	
	/* Define the new prototype: */
	if(bodyNodes.getValues().empty())
		throw VRMLFile::ParseError(*this,"empty body in PROTO definition");
	nodeCreator.definePrototype(protoName.c_str(),*bodyNodes.getValue(0));
	}

void VRMLFile::parseExternProto(void)
	{
	/* Read the prototype name: */
	std::string protoName(getToken());
	readNextToken();
	
	/* Parse the interface declaration: */
	if(!isToken('['))
		throw VRMLFile::ParseError(*this,"missing interface declaration in EXTERNPROTO definition");
	readNextToken();
	if(!isToken(']'))
		throw VRMLFile::ParseError(*this,"non-empty interface declaration in EXTERNPROTO definition");
	
	/* Read the prototype definition URL(s): */
	skipExtendedWhitespace();
	if(peekc()!='"')
		throw VRMLFile::ParseError(*this,"no URL in EXTERNPROTO definition");
	std::string url(readNextToken());
	skipExtendedWhitespace();
	while(peekc()=='"')
		{
		readNextToken();
		skipExtendedWhitespace();
		}
	readNextToken();
	
	/* Load the external prototype: */
	nodeCreator.defineExternalPrototype(*baseDirectory,protoName.c_str(),url.c_str());
	}

VRMLFile::VRMLFile(IO::Directory& sBaseDirectory,const std::string& sSourceUrl,NodeCreator& sNodeCreator)
	:IO::TokenSource(sBaseDirectory.openFile(sSourceUrl.c_str())),
	 baseDirectory(sBaseDirectory.openFileDirectory(sSourceUrl.c_str())),sourceUrl(Misc::getFileName(sSourceUrl.c_str())),
	 nodeCreator(sNodeCreator)
	{
	init();
	}

VRMLFile::VRMLFile(const std::string& sSourceUrl,NodeCreator& sNodeCreator)
	:IO::TokenSource(IO::openFile(sSourceUrl.c_str())),
	 baseDirectory(IO::openFileDirectory(sSourceUrl.c_str())),sourceUrl(Misc::getFileName(sSourceUrl.c_str())),
	 nodeCreator(sNodeCreator)
	{
	init();
	}

VRMLFile::~VRMLFile(void)
	{
	/* Close the prototype scope: */
	nodeCreator.closePrototypeScope();
	}

void VRMLFile::parse(GroupNode& root)
	{
	/* Read nodes until end of file: */
	readNextToken();
	while(!eof())
		{
		if(isToken("PROTO"))
			{
			/* Parse a prototype definition: */
			readNextToken();
			parseProto();
			}
		else if(isToken("EXTERNPROTO"))
			{
			/* Parse an external prototype definition: */
			readNextToken();
			parseExternProto();
			}
		else
			{
			/* Parse a node derived from GraphNode: */
			SF<GraphNodePointer> node;
			parseSFNode(node);
			if(node.getValue()!=0)
				root.addChild(*node.getValue());
			}
		}
	}

NodePointer VRMLFile::getNode(const std::string& nodeName)
	{
	/* Find the named node in the node map: */
	NodeMap::Iterator nIt=nodeMapStack.back().findEntry(nodeName);
	if(!nIt.isFinished())
		return nIt->getDest();
	else
		return 0;
	}

template <class ValueParam>
ValueParam
VRMLFile::parseValue(
	void)
	{
	/* Call on the templatized value parser helper class: */
	return ValueParser<ValueParam>::parseValue(*this);
	}

template <class FieldParam>
void
VRMLFile::parseField(
	FieldParam& field)
	{
	/* Call on the templatized field parser helper class: */
	FieldParser<FieldParam>::parseField(field,*this);
	}

NodePointer VRMLFile::createNode(const char* nodeType)
	{
	return nodeCreator.createNode(nodeType);
	}

void VRMLFile::defineNode(const char* nodeName,NodePointer node)
	{
	nodeMapStack.back().setEntry(NodeMap::Entry(nodeName,node));
	}

NodePointer VRMLFile::useNode(const char* nodeName)
	{
	NodeMap::Iterator nIt=nodeMapStack.back().findEntry(nodeName);
	if(nIt.isFinished())
		throw ParseError(*this,Misc::stringPrintf("Undefined node name %s",nodeName));
	
	return nIt->getDest();
	}

GroupNodePointer readVRMLFile(IO::Directory& baseDirectory,const std::string& sourceUrl)
	{
	/* Create a new node creator: */
	NodeCreator nodeCreator;
	
	/* Open the given VRML file: */
	VRMLFile file(baseDirectory,sourceUrl,nodeCreator);
	
	/* Create a new root node: */
	GroupNodePointer root=new GroupNode;
	
	/* Read the contents of the VRML file into the root node and return it: */
	file.parse(*root);
	return root;
	}

GroupNodePointer readVRMLFile(const std::string& sourceUrl)
	{
	/* Create a new node creator: */
	NodeCreator nodeCreator;
	
	/* Open the given VRML file: */
	VRMLFile file(sourceUrl,nodeCreator);
	
	/* Create a new root node: */
	GroupNodePointer root=new GroupNode;
	
	/* Read the contents of the VRML file into the root node and return it: */
	file.parse(*root);
	return root;
	}

/********************************************************************
Force instantiation of value parser methods for standard value types:
********************************************************************/

template NodePointer VRMLFile::parseValue<NodePointer>();

/********************************************************************
Force instantiation of field parser methods for standard field types:
********************************************************************/

template void VRMLFile::parseField(SFBool&);
template void VRMLFile::parseField(SFString&);
template void VRMLFile::parseField(SFInt&);
template void VRMLFile::parseField(SFFloat&);
template void VRMLFile::parseField(SFSize&);
template void VRMLFile::parseField(SFPoint&);
template void VRMLFile::parseField(SFVector&);
template void VRMLFile::parseField(SFRotation&);
template void VRMLFile::parseField(SFColor&);
template void VRMLFile::parseField(SFTexCoord&);
template void VRMLFile::parseField(SFNode&);
template void VRMLFile::parseField(MFBool&);
template void VRMLFile::parseField(MFString&);
template void VRMLFile::parseField(MFInt&);
template void VRMLFile::parseField(MFFloat&);
template void VRMLFile::parseField(MFSize&);
template void VRMLFile::parseField(MFPoint&);
template void VRMLFile::parseField(MFVector&);
template void VRMLFile::parseField(MFRotation&);
template void VRMLFile::parseField(MFColor&);
template void VRMLFile::parseField(MFTexCoord&);
template void VRMLFile::parseField(MFNode&);

template void VRMLFile::parseField(SF<double>&);
template void VRMLFile::parseField(MF<double>&);
template void VRMLFile::parseField(SF<Geometry::Point<double,3> >&);
template void VRMLFile::parseField(MF<Geometry::Point<double,3> >&);
template void VRMLFile::parseField(SF<Geometry::Vector<double,3> >&);
template void VRMLFile::parseField(MF<Geometry::Vector<double,3> >&);

template void VRMLFile::parseField(SF<Geometry::ComponentArray<Scalar,2> >&);
template void VRMLFile::parseField(SF<Geometry::Vector<Scalar,2> >&);

}
