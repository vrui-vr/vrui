/***********************************************************************
XMLDocument - Class representing the structure and contents of an XML
entity as a tree.
Copyright (c) 2018-2024 Oliver Kreylos

This file is part of the I/O Support Library (IO).

The I/O Support Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The I/O Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the I/O Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <IO/XMLDocument.h>

#include <stdio.h>
#include <stdexcept>
#include <Misc/SelfDestructPointer.h>
#include <Misc/StdError.h>
#include <IO/Directory.h>
#include <IO/UTF8.h>
#include <IO/XMLSource.h>

namespace IO {

/*****************************************
Methods of class XMLNode::ConversionError:
*****************************************/

std::string XMLNode::ConversionError::createErrorString(const XMLNode* sNode,const char* sRequestedType)
	{
	char stringBuffer[512];
	snprintf(stringBuffer,sizeof(stringBuffer),"IO::XMLNode: Unable to convert node of type %s to %s",sNode->getTypeName(),sRequestedType);
	return std::string(stringBuffer);
	}

XMLNode::ConversionError::ConversionError(const XMLNode* sNode,const char* sRequestedType)
	:std::runtime_error(createErrorString(sNode,sRequestedType)),
	 node(sNode),requestedType(sRequestedType)
	{
	}

/************************
Methods of class XMLNode:
************************/

XMLNode::~XMLNode(void)
	{
	/* Nothing to do, incidentally: */
	}

const XMLContainer* XMLNode::getContainer(void) const
	{
	const XMLContainer* result=dynamic_cast<const XMLContainer*>(this);
	if(result==0)
		throw ConversionError(this,XMLContainer::getClassTypeName());
	return result;
	}

XMLContainer* XMLNode::getContainer(void)
	{
	XMLContainer* result=dynamic_cast<XMLContainer*>(this);
	if(result==0)
		throw ConversionError(this,XMLContainer::getClassTypeName());
	return result;
	}

const XMLNodeList& XMLNode::getChildren(void) const
	{
	return getContainer()->getChildren();
	}

XMLNodeList& XMLNode::getChildren(void)
	{
	return getContainer()->getChildren();
	}

const XMLElement* XMLNode::findNextElement(const char* name,const XMLNode* afterChild) const
	{
	return getContainer()->findNextElement(name,afterChild);
	}

XMLElement* XMLNode::findNextElement(const char* name,XMLNode* afterChild)
	{
	return getContainer()->findNextElement(name,afterChild);
	}

const XMLCharacterData* XMLNode::getCharacterData(void) const
	{
	const XMLCharacterData* result=dynamic_cast<const XMLCharacterData*>(this);
	if(result==0)
		throw ConversionError(this,XMLCharacterData::getClassTypeName());
	return result;
	}

XMLCharacterData* XMLNode::getCharacterData(void)
	{
	XMLCharacterData* result=dynamic_cast<XMLCharacterData*>(this);
	if(result==0)
		throw ConversionError(this,XMLCharacterData::getClassTypeName());
	return result;
	}

const std::string& XMLNode::getData(void) const
	{
	return getCharacterData()->getData();
	}

const XMLComment* XMLNode::getComment(void) const
	{
	const XMLComment* result=dynamic_cast<const XMLComment*>(this);
	if(result==0)
		throw ConversionError(this,XMLComment::getClassTypeName());
	return result;
	}

XMLComment* XMLNode::getComment(void)
	{
	XMLComment* result=dynamic_cast<XMLComment*>(this);
	if(result==0)
		throw ConversionError(this,XMLComment::getClassTypeName());
	return result;
	}

const XMLProcessingInstruction* XMLNode::getProcessingInstruction(void) const
	{
	const XMLProcessingInstruction* result=dynamic_cast<const XMLProcessingInstruction*>(this);
	if(result==0)
		throw ConversionError(this,XMLProcessingInstruction::getClassTypeName());
	return result;
	}

XMLProcessingInstruction* XMLNode::getProcessingInstruction(void)
	{
	XMLProcessingInstruction* result=dynamic_cast<XMLProcessingInstruction*>(this);
	if(result==0)
		throw ConversionError(this,XMLProcessingInstruction::getClassTypeName());
	return result;
	}

const XMLElement* XMLNode::getElement(void) const
	{
	const XMLElement* result=dynamic_cast<const XMLElement*>(this);
	if(result==0)
		throw ConversionError(this,XMLElement::getClassTypeName());
	return result;
	}

XMLElement* XMLNode::getElement(void)
	{
	XMLElement* result=dynamic_cast<XMLElement*>(this);
	if(result==0)
		throw ConversionError(this,XMLElement::getClassTypeName());
	return result;
	}

const std::string& XMLNode::getName(void) const
	{
	return getElement()->getName();
	}

bool XMLNode::hasAttribute(const std::string& attributeName) const
	{
	return getElement()->hasAttribute(attributeName);
	}

const std::string& XMLNode::getAttributeValue(const std::string& attributeName) const
	{
	return getElement()->getAttributeValue(attributeName);
	}

/****************************
Methods of class XMLNodeList:
****************************/

XMLNodeList::~XMLNodeList(void)
	{
	/* Delete all nodes: */
	while(head!=0)
		{
		/* Delete the head and make its younger sibling the new head: */
		XMLNode* sibling=head->sibling;
		delete head;
		head=sibling;
		}
	}

unsigned int XMLNodeList::size(void) const
	{
	/* Count the number of nodes in the list: */
	unsigned int result=0;
	for(const XMLNode* nPtr=head;nPtr!=0;nPtr=nPtr->sibling,++result)
		;
	
	return result;
	}

const XMLNode* XMLNodeList::operator[](unsigned int index) const
	{
	/* Find the node of the given index: */
	const XMLNode* nPtr;
	for(nPtr=head;nPtr!=0&&index>0;nPtr=nPtr->sibling,--index)
		;
	if(nPtr==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Index out of bounds");
	
	return nPtr;
	}

XMLNode* XMLNodeList::operator[](unsigned int index)
	{
	/* Find the node of the given index: */
	XMLNode* nPtr;
	for(nPtr=head;nPtr!=0&&index>0;nPtr=nPtr->sibling,--index)
		;
	if(nPtr==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Index out of bounds");
	
	return nPtr;
	}

void XMLNodeList::push_back(XMLNode* node)
	{
	/* Find the last node: */
	XMLNode* pred=0;
	for(XMLNode* nPtr=head;nPtr!=0;pred=nPtr,nPtr=nPtr->sibling)
		;
	
	/* Link the new node: */
	if(pred!=0)
		{
		/* Link the node after the predecessor: */
		node->sibling=pred->sibling;
		pred->sibling=node;
		}
	else
		{
		/* Link the node as this node's first element: */
		node->sibling=head;
		head=node;
		}
	}

void XMLNodeList::insert(unsigned int index,XMLNode* node)
	{
	/* Find the node preceding the given index: */
	XMLNode* pred=0;
	for(XMLNode* nPtr=head;nPtr!=0&&index>0;pred=nPtr,nPtr=nPtr->sibling,--index)
		;
	
	/* Check if the index was valid: */
	if(index>0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Index out of bounds");
		
	/* Link the new node: */
	if(pred!=0)
		{
		/* Link the node after the predecessor: */
		node->sibling=pred->sibling;
		pred->sibling=node;
		}
	else
		{
		/* Link the node as this node's first element: */
		node->sibling=head;
		head=node;
		}
	}

XMLNode* XMLNodeList::pop_back(void)
	{
	/* Check if the list is empty: */
	if(head==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"List is empty");
	
	/* Find the node preceding the last node: */
	XMLNode* pred=0;
	XMLNode* nPtr;
	for(nPtr=head;nPtr->sibling!=0;pred=nPtr,nPtr=nPtr->sibling)
		;
	
	/* Unlink the found node: */
	if(pred!=0)
		pred->sibling=nPtr->sibling;
	else
		head=nPtr->sibling;
	
	/* Make the unlinked node a single and return it: */
	nPtr->sibling=0;
	return nPtr;
	}

XMLNode* XMLNodeList::erase(unsigned int index)
	{
	/* Find the node preceding the node of the given index: */
	XMLNode* pred=0;
	XMLNode* nPtr;
	for(nPtr=head;nPtr!=0&&index>0;pred=nPtr,nPtr=nPtr->sibling,--index)
		;
	
	/* Check if the index was valid: */
	if(index>0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Index out of bounds");
	
	/* Unlink the found node: */
	if(pred!=0)
		pred->sibling=nPtr->sibling;
	else
		head=nPtr->sibling;
	
	/* Make the unlinked node a single and return it: */
	nPtr->sibling=0;
	return nPtr;
	}

void XMLNodeList::erase(XMLNode* node)
	{
	/* Find the node preceding the given node: */
	XMLNode* pred=0;
	XMLNode* nPtr;
	for(nPtr=head;nPtr!=0&&nPtr!=node;pred=nPtr,nPtr=nPtr->sibling)
		;
	
	/* Check if the node was found: */
	if(nPtr!=0)
		{
		/* Unlink the found node: */
		if(pred!=0)
			pred->sibling=nPtr->sibling;
		else
			head=nPtr->sibling;
		}
	
	/* Make the given node a single: */
	node->sibling=0;
	}

/*****************************
Methods of class XMLContainer:
*****************************/

const char* XMLContainer::getClassTypeName(void)
	{
	return "XMLContainer";
	}

const char* XMLContainer::getTypeName(void) const
	{
	return getClassTypeName();
	}

const XMLElement* XMLContainer::findNextElement(const char* name,const XMLNode* afterChild) const
	{
	const XMLNode* child=afterChild!=0?afterChild->getSibling():children.front();
	for(;child!=0;child=child->getSibling())
		{
		/* Check whether the node is an element: */
		const XMLElement* element=dynamic_cast<const XMLElement*>(child);
		if(element!=0)
			{
			/* Return the element if its name matches: */
			if(element->getName()==name)
				return element;
			}
		}
	
	return 0;
	}

XMLElement* XMLContainer::findNextElement(const char* name,XMLNode* afterChild)
	{
	XMLNode* child=afterChild!=0?afterChild->getSibling():children.front();
	for(;child!=0;child=child->getSibling())
		{
		/* Check whether the node is an element: */
		XMLElement* element=dynamic_cast<XMLElement*>(child);
		if(element!=0)
			{
			/* Return the element if its name matches: */
			if(element->getName()==name)
				return element;
			}
		}
	
	return 0;
	}

/*********************************
Methods of class XMLCharacterData:
*********************************/

XMLCharacterData::XMLCharacterData(XMLSource& source)
	{
	/* Read the character data: */
	source.readUTF8(data);
	}

const char* XMLCharacterData::getClassTypeName(void)
	{
	return "XMLCharacterData";
	}

const char* XMLCharacterData::getTypeName(void) const
	{
	return getClassTypeName();
	}

bool XMLCharacterData::isSpace(void) const
	{
	/* Check all characters: */
	std::string::const_iterator dIt;
	for(dIt=data.begin();dIt!=data.end()&&isSpace(*dIt);++dIt)
		;
	
	return dIt==data.end();
	}

/***************************
Methods of class XMLComment:
***************************/

XMLComment::XMLComment(XMLSource& source)
	{
	/* Read the comment: */
	source.readUTF8(comment);
	}

const char* XMLComment::getClassTypeName(void)
	{
	return "XMLComment";
	}

const char* XMLComment::getTypeName(void) const
	{
	return getClassTypeName();
	}

/*****************************************
Methods of class XMLProcessingInstruction:
*****************************************/

XMLProcessingInstruction::XMLProcessingInstruction(XMLSource& source)
	{
	/* Read the processing instruction target and instruction: */
	source.readUTF8(target);
	source.readUTF8(instruction);
	}

const char* XMLProcessingInstruction::getClassTypeName(void)
	{
	return "XMLProcessingInstruction";
	}

const char* XMLProcessingInstruction::getTypeName(void) const
	{
	return getClassTypeName();
	}

/***************************
Methods of class XMLElement:
***************************/

XMLElement::XMLElement(XMLSource& source)
	:attributes(5)
	{
	/* Read the element name: */
	source.readUTF8(name);
	
	/* Read all attribute/value pairs: */
	while(source.isAttributeName())
		{
		/* Read the name/value pair and store it in the attribute map: */
		std::string name,value;
		source.readUTF8(name);
		source.readUTF8(value);
		attributes.setEntry(AttributeMap::Entry(name,value));
		}
	
	/* Check if the tag has content and a closing tag: */
	empty=source.wasSelfClosingTag();
	if(!empty)
		{
		/* Read the element's content: */
		while(true)
			{
			/* Proceed based on the type of the current syntactic element: */
			XMLNode* node=0;
			if(source.isCharacterData())
				{
				/* Read a character data node: */
				node=new XMLCharacterData(source);
				}
			else if(source.isComment())
				{
				/* Read a comment node: */
				node=new XMLComment(source);
				}
			else if(source.isPITarget())
				{
				/* Read a processing instruction node: */
				node=new XMLProcessingInstruction(source);
				}
			else if(source.isTagName())
				{
				/* Check if this is an opening tag: */
				if(source.isOpeningTag())
					{
					/* Read an element node: */
					node=new XMLElement(source);
					}
				else
					{
					/* Check that the closing tag matches this element's name: */
					std::string tagName;
					source.readUTF8(tagName);
					if(tagName!=name)
						throw XMLSource::WellFormedError(source,"Mismatching closing tag name");
					
					/* Stop reading content: */
					break;
					}
				}
			else if(source.eof())
				throw XMLSource::WellFormedError(source,"Unterminated element");
			
			/* Add the content node as a child of this node: */
			getChildren().push_back(node);
			}
		}
	}

const char* XMLElement::getClassTypeName(void)
	{
	return "XMLElement";
	}

const char* XMLElement::getTypeName(void) const
	{
	return getClassTypeName();
	}

void XMLElement::setAttributeValue(const std::string& attributeName,const std::string& attributeValue)
	{
	/* Add or replace the name/value association in the map: */
	attributes.setEntry(AttributeMap::Entry(attributeName,attributeValue));
	}

void XMLElement::removeAttribute(const std::string& attributeName)
	{
	/* Remove the name/value association from the map: */
	attributes.removeEntry(attributeName);
	}

const XMLElement* XMLElement::findNextElement(const char* name,const XMLNode* afterChild) const
	{
	/* Get the first node to check: */
	const XMLNode* cPtr=afterChild!=0?afterChild->getSibling():getChildren().front();
	while(cPtr!=0)
		{
		/* Check if the child is an element and matches the requested name: */
		const XMLElement* element=dynamic_cast<const XMLElement*>(cPtr);
		if(element!=0&&element->getName()==name)
			return element;
		
		/* Go to the next child: */
		cPtr=cPtr->getSibling();
		}
	
	/* No matching element found: */
	return 0;
	}

XMLElement* XMLElement::findNextElement(const char* name,XMLNode* afterChild)
	{
	/* Get the first node to check: */
	XMLNode* cPtr=afterChild!=0?afterChild->getSibling():getChildren().front();
	while(cPtr!=0)
		{
		/* Check if the child is an element and matches the requested name: */
		XMLElement* element=dynamic_cast<XMLElement*>(cPtr);
		if(element!=0&&element->getName()==name)
			return element;
		
		/* Go to the next child: */
		cPtr=cPtr->getSibling();
		}
	
	/* No matching element found: */
	return 0;
	}

/****************************
Methods of class XMLDocument:
****************************/

void XMLDocument::parseFile(IO::File& file)
	{
	/* Wrap the XML file in a low-level XML processor: */
	XMLSource source(&file);
	
	/* Read comments and processing instructions preceding the root element: */
	while(!source.isTagName())
		{
		/* Proceed based on the type of the current syntactic element: */
		XMLNode* node=0;
		if(source.eof())
			throw XMLSource::WellFormedError(source,"No root element in XML document");
		else if(source.isComment())
			{
			/* Read a comment node: */
			node=new XMLComment(source);
			}
		else if(source.isPITarget())
			{
			/* Read a processing instruction node: */
			node=new XMLProcessingInstruction(source);
			}
		else if(source.isCharacterData())
			{
			/* Whitespace is allowed in the XML prolog; check for anything that's not: */
			int c;
			while((c=source.readCharacterData())>=0)
				if(!XMLSource::isSpace(c))
					throw XMLSource::WellFormedError(source,"Non-whitespace character data in XML prolog");
			}
		else
			throw XMLSource::WellFormedError(source,"Illegal syntactic element in XML prolog");
		
		/* Add a read node to the prolog: */
		if(node!=0)
			prolog.push_back(node);
		}
	
	/* Check if the tag is an opening tag: */
	if(!source.isOpeningTag())
		throw XMLSource::WellFormedError(source,"Missing opening tag for root element");
	
	/* Read the root element: */
	Misc::SelfDestructPointer<XMLElement> tempRoot(new XMLElement(source));
	
	/* Read comments and processing instructions succeeding the root element: */
	while(!source.eof())
		{
		/* Proceed based on the type of the current syntactic element: */
		XMLNode* node=0;
		if(source.isComment())
			{
			/* Read a comment node: */
			node=new XMLComment(source);
			}
		else if(source.isPITarget())
			{
			/* Read a processing instruction node: */
			node=new XMLProcessingInstruction(source);
			}
		else if(source.isCharacterData())
			{
			/* Whitespace is allowed in the XML epilog; check for anything that's not: */
			int c;
			while((c=source.readCharacterData())>=0)
				if(!XMLSource::isSpace(c))
					throw XMLSource::WellFormedError(source,"Non-whitespace character data in XML epilog");
			}
		else
			throw XMLSource::WellFormedError(source,"Illegal syntactic element in XML epilog");
		
		/* Add a read node to the epilog: */
		if(node!=0)
			epilog.push_back(node);
		}
	
	/* Retain the root element: */
	root=tempRoot.releaseTarget();
	}

XMLDocument::XMLDocument(IO::File& file)
	:root(0)
	{
	/* Parse the given file: */
	parseFile(file);
	}

XMLDocument::XMLDocument(IO::Directory& directory,const char* xmlFileName)
	:root(0)
	{
	/* Open the XML file and parse it: */
	parseFile(*directory.openFile(xmlFileName));
	}

XMLDocument::~XMLDocument(void)
	{
	/* Delete the root element and its subtree: */
	delete root;
	}

}
