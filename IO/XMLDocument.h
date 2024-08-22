/***********************************************************************
XMLDocument - Class representing the structure and contents of an XML
document as a tree of nodes.
Copyright (c) 2018-2022 Oliver Kreylos

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

#ifndef IO_XMLDOCUMENT_INCLUDED
#define IO_XMLDOCUMENT_INCLUDED

#include <string>
#include <Misc/StandardHashFunction.h>
#include <Misc/StringHashFunctions.h>
#include <Misc/HashTable.h>

/* Forward declarations: */
namespace IO {
class File;
class Directory;
class XMLSource;
}

namespace IO {

/* Forward declarations: */
class XMLNodeList;
class XMLContainer;
class XMLCharacterData;
class XMLComment;
class XMLProcessingInstruction;
class XMLElement;

class XMLNode // Base class for XML document tree nodes
	{
	friend class XMLNodeList;
	
	/* Embedded classes: */
	public:
	class ConversionError:public std::runtime_error // Class to signal node type conversion errors
		{
		/* Elements: */
		public:
		const XMLNode* node; // Node that could not be converted
		std::string requestedType; // Requested conversion target type
		
		/* Private methods: */
		private:
		static std::string createErrorString(const XMLNode* sNode,const char* sRequestedType); // Creates an error message
		
		/* Constructors and destructors: */
		public:
		ConversionError(const XMLNode* sNode,const char* sRequestedType);
		};
	
	/* Elements: */
	private:
	XMLNode* sibling; // Pointer to the next-younger sibling of this node
	
	/* Constructors and destructors: */
	public:
	XMLNode(void) // Creates a single node
		:sibling(0)
		{
		}
	virtual ~XMLNode(void); // Destroys this node
	
	/* Methods: */
	virtual const char* getTypeName(void) const =0; // Returns the node's type name
	const XMLNode* getSibling(void) const // Returns this node's sibling
		{
		return sibling;
		}
	XMLNode* getSibling(void) // Ditto
		{
		return sibling;
		}
	
	/* Convenience methods to downcast an XML node to well-known node types; throw exception on invalid conversion: */
	const XMLContainer* getContainer(void) const; // Converts to a container
	XMLContainer* getContainer(void); // Ditto
	const XMLNodeList& getChildren(void) const; // Returns the list of a container's children
	XMLNodeList& getChildren(void); // Ditto
	const XMLElement* findNextElement(const char* name,const XMLNode* afterChild) const; // Returns the next XML element of the given name after the given node, or from the first child if the node is null
	XMLElement* findNextElement(const char* name,XMLNode* afterChild); // Ditto
	
	const XMLCharacterData* getCharacterData(void) const; // Converts to character data
	XMLCharacterData* getCharacterData(void); // Ditto
	const std::string& getData(void) const; // Returns character data's character data
	
	const XMLComment* getComment(void) const; // Converts to a comment
	XMLComment* getComment(void); // Ditto
	
	const XMLProcessingInstruction* getProcessingInstruction(void) const; // Converts to a processing instruction
	XMLProcessingInstruction* getProcessingInstruction(void); // Ditto
	
	const XMLElement* getElement(void) const; // Converts to an element
	XMLElement* getElement(void); // Ditto
	const std::string& getName(void) const; // Returns element's name
	bool hasAttribute(const std::string& attributeName) const; // Returns true if an attribute with the given name is associated with an element
	const std::string& getAttributeValue(const std::string& attributeName) const; // Returns the value of the given attribute of an element; throws exception if attribute name does not exist in the element
	};

class XMLNodeList // Wrapper class to impose list semantics on node pointers
	{
	/* Elements: */
	private:
	XMLNode* head; // Pointer to first node in list, or null for empty list
	
	/* Constructors and destructors: */
	public:
	XMLNodeList(void) // Creates an empty list
		:head(0)
		{
		}
	~XMLNodeList(void); // Deletes all nodes in the list
	
	/* Methods: */
	bool empty(void) const // Returns true if the list is empty
		{
		return head==0;
		}
	const XMLNode* front(void) const // Returns the first node in the list (or 0 for empty list)
		{
		return head;
		}
	XMLNode* front(void) // Ditto
		{
		return head;
		}
	unsigned int size(void) const; // Returns the number of nodes in this list
	const XMLNode* operator[](unsigned int index) const; // Returns the node of the given index; throws exception if index>=size
	XMLNode* operator[](unsigned int index); // Ditto
	void push_back(XMLNode* node); // Appends the given node to the end of the list
	void insert(unsigned int index,XMLNode* node); // Inserts the given node at the given position in the list
	XMLNode* pop_back(void); // Unlinks the last node in the list and returns it
	XMLNode* erase(unsigned int index); // Unlinks the node of the given index and returns it; throws exception if index>=size
	void erase(XMLNode* node); // Unlinks (but does not delete) the given node from the list; does nothing if node wasn't in list in the first place
	};

class XMLContainer:public XMLNode // Base class representing nodes that have children
	{
	/* Elements: */
	private:
	XMLNodeList children; // List of child nodes
	
	/* Methods from class XMLNode: */
	public:
	static const char* getClassTypeName(void);
	virtual const char* getTypeName(void) const;
	
	/* New methods: */
	const XMLNodeList& getChildren(void) const // Returns the list of this container's children
		{
		return children;
		}
	XMLNodeList& getChildren(void) // Ditto
		{
		return children;
		}
	const XMLElement* findNextElement(const char* name,const XMLNode* afterChild) const; // Returns the next XML element of the given name after the given node, or from the first child if the node is null
	XMLElement* findNextElement(const char* name,XMLNode* afterChild); // Ditto
	};

class XMLCharacterData:public XMLNode // Class representing an uninterrupted sequence of character data
	{
	/* Elements: */
	private:
	std::string data; // The character data encoded as UTF-8
	
	/* Constructors and destructors: */
	public:
	XMLCharacterData(const char* sData) // Copies the given UTF-8 encoded string
		:data(sData)
		{
		}
	XMLCharacterData(XMLSource& source); // Reads character data from the given XML source
	
	/* Methods from class XMLNode: */
	static const char* getClassTypeName(void);
	virtual const char* getTypeName(void) const;
	
	/* Methods: */
	const std::string& getData(void) const // Returns the character data
		{
		return data;
		}
	static bool isSpace(int c) // Returns true if the given character is XML whitespace
		{
		return c==0x09||c==0x0a||c==0x20;
		}
	static std::string::const_iterator skipSpace(std::string::const_iterator begin,std::string::const_iterator end) // Skips whitespace characters starting from the given iterator and returns iterator to first non-whitespace character or end-of-string
		{
		/* Skip until non-whitespace or end of string: */
		while(begin!=end&&isSpace(*begin))
			++begin;
		return begin;
		}
	bool isSpace(void) const; // Returns true if the character data is empty or entirely whitespace
	void setData(const char* newData) // Replaces the character data
		{
		data=newData;
		}
	};

class XMLComment:public XMLNode // Class representing an XML comment
	{
	/* Elements: */
	private:
	std::string comment; // The comment string encoded as UTF-8
	
	/* Constructors and destructors: */
	public:
	XMLComment(const char* sXMLComment) // Copies the given UTF-8 encoded string
		:comment(sXMLComment)
		{
		}
	XMLComment(XMLSource& source); // Reads a comment from the given XML source
	
	/* Methods from class XMLNode: */
	public:
	static const char* getClassTypeName(void);
	virtual const char* getTypeName(void) const;
	
	/* Methods: */
	const std::string& getComment(void) const // Returns the comment
		{
		return comment;
		}
	};

class XMLProcessingInstruction:public XMLNode // Class representing an XML processing instruction
	{
	/* Elements: */
	private:
	std::string target; // The processing instruction's target encoded as UTF-8
	std::string instruction; // The processing instruction encoded as UTF-8
	
	/* Constructors and destructors: */
	public:
	XMLProcessingInstruction(const char* sTarget,const char* sInstruction) // Copies the given UTF-8 encoded strings
		:target(sTarget),instruction(sInstruction)
		{
		}
	XMLProcessingInstruction(XMLSource& source); // Reads a processing instruction from the given XML source
	
	/* Methods from class XMLNode: */
	public:
	static const char* getClassTypeName(void);
	virtual const char* getTypeName(void) const;
	
	/* Methods: */
	const std::string& getTarget(void) const // Returns the target
		{
		return target;
		}
	const std::string& getInstruction(void) const // Returns the instruction
		{
		return instruction;
		}
	};

class XMLElement:public XMLContainer // Class representing an XML element, i.e., an opening and closing tag and everything inbetween
	{
	/* Embedded classes: */
	public:
	typedef Misc::HashTable<std::string,std::string> AttributeMap; // Type for hash tables mapping attribute names to attribute values
	
	/* Elements: */
	private:
	std::string name; // The element's name encoded as UTF-8
	AttributeMap attributes; // Map representing the element's attributes
	bool empty; // Flag if this element used a self-closing opening tag. Even if false, the element may still have no content
	
	/* Constructors and destructors: */
	public:
	XMLElement(const char* sName,bool sEmpty =false) // Copies the given UTF-8 encoded string and sets the empty flag
		:name(sName),
		 attributes(5),empty(sEmpty)
		{
		}
	XMLElement(XMLSource& source); // Reads an element from the given XML source
	
	/* Methods from class XMLNode: */
	public:
	static const char* getClassTypeName(void);
	virtual const char* getTypeName(void) const;
	
	/* Methods: */
	const std::string& getName(void) const // Returns the element's name
		{
		return name;
		}
	const AttributeMap& getAttributes(void) const // Returns the element's attribute map
		{
		return attributes;
		}
	AttributeMap& getAttributes(void) // Ditto
		{
		return attributes;
		}
	bool hasAttribute(const std::string& attributeName) const // Returns true if an attribute with the given name is associated with this element
		{
		/* Check if there is an entry with the given name in the attribute map: */
		return attributes.isEntry(attributeName);
		}
	const std::string& getAttributeValue(const std::string& attributeName) const // Returns the value of the given attribute; throws exception if attribute name does not exist
		{
		/* Look up the given attribute name in the map and return its associated value: */
		return attributes.getEntry(attributeName).getDest();
		}
	void setAttributeValue(const std::string& attributeName,const std::string& attributeValue); // Sets the value of the given attribute, replacing any previous association
	void removeAttribute(const std::string& attributeName); // Removes an association of the given attribute name from the element
	bool isEmpty(void) const // Returns true if this element used a self-closing opening tag. Even if false, the element may still have no content
		{
		return empty;
		}
	const XMLElement* findNextElement(const char* name,const XMLNode* afterChild) const; // Returns the next XML element of the given name after the given child (or from the first child if afterChild==0); returns 0 if there are no further matching elements
	XMLElement* findNextElement(const char* name,XMLNode* afterChild); // Ditto
	};

class XMLDocument // Class representing markup and character data of an XML document as a tree of nodes
	{
	/* Elements: */
	private:
	XMLNodeList prolog; // A list of processing instructions or comments that appeared before the root element
	XMLElement* root; // The root element of the XML document
	XMLNodeList epilog; // A list of processing instructions or comments that appeared after the root element
	
	/* Private methods: */
	void parseFile(IO::File& file); // Parses the given file into an XML document
	
	/* Constructors and destructors: */
	public:
	XMLDocument(IO::File& file); // Reads an XML document from the given file
	XMLDocument(IO::Directory& directory,const char* xmlFileName); // Reads an XML document from a file of the given name relative the given directory
	~XMLDocument(void); // Destroys the XML document
	
	/* Methods: */
	const XMLNodeList& getProlog(void) const // Returns the list of prolog nodes
		{
		return prolog;
		}
	XMLNodeList& getProlog(void) // Ditto
		{
		return prolog;
		}
	const XMLNode* getRoot(void) const // Returns the document's root node
		{
		return root;
		}
	XMLNode* getRoot(void) // Ditto
		{
		return root;
		}
	const XMLNodeList& getEpilog(void) const // Returns the list of epilog nodes
		{
		return epilog;
		}
	XMLNodeList& getEpilog(void) // Ditto
		{
		return epilog;
		}
	};

}

#endif
