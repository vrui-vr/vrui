/***********************************************************************
FactoryManager - Generic base class for managers of factory classes
derived from a common base class. Intended to manage loading of dynamic
shared objects.
Copyright (c) 2003-2025 Oliver Kreylos

This file is part of the Plugin Handling Library (Plugins).

The Plugin Handling Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Plugin Handling Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Plugin Handling Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef PLUGINS_FACTORYMANAGER_INCLUDED
#define PLUGINS_FACTORYMANAGER_INCLUDED

#include <utility>
#include <string>
#include <vector>
#include <stdexcept>
#include <Misc/FileLocator.h>

namespace Plugins {

class FactoryManagerBase // Non-templatized base class for type-specific factory managers
	{
	/* Embedded classes: */
	public:
	typedef unsigned short int ClassIdType; // Type to store class IDs
	static const unsigned int maxClassId=65536; // One-past-maximum value for class IDs
	
	class Error:public std::runtime_error // Base class for factory manager errors
		{
		/* Constructors and destructors: */
		public:
		Error(const std::string& cause)
			:std::runtime_error(cause)
			{
			}
		Error(const char* cause)
			:std::runtime_error(cause)
			{
			}
		};
	
	class DsoError:public Error // Error type thrown if something goes wrong while processing a DSO
		{
		/* Constructors and destructors: */
		public:
		DsoError(const char* source); // Creates DsoError object from error string returned by dl_* calls
		};
	
	protected:
	struct LoadDsoResults // Structure holding results from loading and resolving functions from a managed factory DSO
		{
		/* Embedded classes: */
		public:
		typedef void (*FunctionPointer)(void); // Type for generic untyped C-style function pointers
		
		/* Elements: */
		void* dsoHandle; // Handle to the DSO
		FunctionPointer resolveDependencies; // The factory dependency resolution function
		FunctionPointer createFactory; // The factory creation function
		FunctionPointer destroyFactory; // The factory destruction function
		
		/* Methods: */
		FunctionPointer resolveFunction(const char* functionNameTemplate,const std::string& shortClassName); // Function to resolve one of the class management functions from a DSO
		};
	
	/* Elements: */
	private:
	std::string dsoNameTemplate; // printf-style format string to create DSO names from class names; must contain exactly one %s conversion
	Misc::FileLocator dsoLocator; // File locator to find DSO files
	
	/* Protected methods: */
	protected:
	std::pair<std::string,bool> extractClassName(const char* className) const; // Returns the class name contained in the given string, which might be a full DSO name with an optional path; second part of return value is true if the DSO template needs to be applied; throws exception if class name is malformed
	LoadDsoResults loadDso(const char* className,bool applyTemplate,const std::string& shortClassName) const; // Loads the DSO corresponding to the given managed factory class name; apply the DSO name template if flag is true
	
	/* Constructors and destructors: */
	public:
	FactoryManagerBase(const std::string& sDsoNameTemplate); // Creates "empty" manager; initializes DSO locator search path to template's base directory
	
	/* Methods: */
	const std::string& getDsoNameTemplate(void) const // Returns the DSO name template
		{
		return dsoNameTemplate;
		}
	const Misc::FileLocator& getDsoLocator(void) const // Returns reference to the DSO file locator
		{
		return dsoLocator;
		}
	Misc::FileLocator& getDsoLocator(void) // Ditto
		{
		return dsoLocator;
		}
	};

template <class ManagedFactoryParam>
class FactoryManager:public FactoryManagerBase
	{
	/* Embedded classes: */
	public:
	typedef ManagedFactoryParam ManagedFactory; // Base class of factories managed by this manager
	typedef void (*DestroyFactoryFunction)(ManagedFactory*); // Type of class unloader function stored in DSOs
	
	private:
	struct FactoryData // Structure containing information about a factory
		{
		/* Elements: */
		public:
		ClassIdType classId; // Unique number identifying the factory class
		void* dsoHandle; // Handle for dynamic shared object containing class
		ManagedFactory* factory; // Pointer to factory
		DestroyFactoryFunction destroyFactoryFunction; // Pointer to factory destruction function stored in DSO
		
		/* Constructors and destructors: */
		FactoryData(void* sDsoHandle,ManagedFactory* sFactory,DestroyFactoryFunction sDestroyFactoryFunction)
			:dsoHandle(sDsoHandle),factory(sFactory),destroyFactoryFunction(sDestroyFactoryFunction)
			{
			}
		};
	
	typedef std::vector<FactoryData> FactoryList;
	
	public:
	class FactoryIterator
		{
		friend class FactoryManager<ManagedFactoryParam>;
		
		/* Elements: */
		typename FactoryList::iterator it; // Iterator to element of FactoryList
		
		/* Constructors and destructors: */
		private:
		FactoryIterator(const typename FactoryList::iterator& sIt) // Private constructor to convert from standard iterator
			:it(sIt)
			{
			}
		public:
		FactoryIterator(void) // Creates invalid constructor
			{
			}
		
		/* Methods: */
		ManagedFactory& operator*(void) const // Dereference operator
			{
			return *(it->factory);
			}
		ManagedFactory* operator->(void) const // Arrow operator
			{
			return it->factory;
			}
		bool operator==(const FactoryIterator& other) // Equality operator
			{
			return it==other.it;
			}
		bool operator!=(const FactoryIterator& other) // Inequality operator
			{
			return it!=other.it;
			}
		FactoryIterator& operator++(void) // Pre-increment
			{
			++it;
			return *this;
			}
		FactoryIterator operator++(int) // Post-increment
			{
			return FactoryIterator(it++);
			}
		};
	
	class ConstFactoryIterator
		{
		friend class FactoryManager<ManagedFactoryParam>;
		
		/* Elements: */
		typename FactoryList::const_iterator it; // Iterator to element of FactoryList
		
		/* Constructors and destructors: */
		private:
		ConstFactoryIterator(const typename FactoryList::const_iterator& sIt) // Private constructor to convert from standard iterator
			:it(sIt)
			{
			}
		public:
		ConstFactoryIterator(void) // Creates invalid constructor
			{
			}
		
		/* Methods: */
		const ManagedFactory& operator*(void) const // Dereference operator
			{
			return *(it->factory);
			}
		const ManagedFactory* operator->(void) const // Arrow operator
			{
			return it->factory;
			}
		bool operator==(const ConstFactoryIterator& other) // Equality operator
			{
			return it==other.it;
			}
		bool operator!=(const ConstFactoryIterator& other) // Inequality operator
			{
			return it!=other.it;
			}
		ConstFactoryIterator& operator++(void) // Pre-increment
			{
			++it;
			return *this;
			}
		ConstFactoryIterator operator++(int) // Post-increment
			{
			return ConstFactoryIterator(it++);
			}
		};
	
	/* Elements: */
	private:
	FactoryList factories; // List of loaded factories
	
	/* Private methods: */
	ClassIdType getNewClassId(void) const; // Returns a unique class ID
	typename FactoryList::const_iterator findFactory(const char* className) const; // Returns an iterator to the factory of the given class name
	typename FactoryList::iterator findFactory(const char* className); // Ditto
	void destroyFactory(typename FactoryList::iterator factoryIt); // Destroys the given object class at runtime; throws exception if class cannot be removed due to dependencies
	
	/* Constructors and destructors: */
	public:
	FactoryManager(std::string sDsoNameTemplate) // Creates "empty" manager; initializes DSO locator search path to template's base directory
		:FactoryManagerBase(sDsoNameTemplate)
		{
		}
	~FactoryManager(void); // Releases all loaded object classes and DSOs
	
	/* Methods: */
	std::pair<ManagedFactory*,bool> loadClassAndCheck(const char* className); // Loads an object class at runtime and returns class object pointer and flag indicating if class was newly-loaded from a DSO
	ManagedFactory* loadClass(const char* className) // Ditto, ignoring the second part of the return value, mimicking legacy API
		{
		/* Delegate to the other method and ignore the "newly added" flag: */
		return loadClassAndCheck(className).first;
		}
	void addClass(ManagedFactory* newFactory,DestroyFactoryFunction newDestroyFactoryFunction =0); // Adds an existing factory to the manager
	void releaseClass(ManagedFactory* factory); // Destroys an object class at runtime; throws exception if class cannot be removed due to dependencies
	void releaseClass(const char* className) // Ditto, identifying the class by name
		{
		/* Find and destroy the factory for the class of the given name: */
		destroyFactory(findFactory(className));
		}
	void releaseClasses(void); // Releases all loaded classes
	ClassIdType getClassId(const ManagedFactory* factory) const; // Returns class ID based on factory object
	ManagedFactory* getFactory(ClassIdType classId) const; // Returns factory object based on class ID
	ManagedFactory* getFactory(const char* className) const // Returns factory object based on class name
		{
		/* Find the factory in the list and return it: */
		typename FactoryList::const_iterator fIt=findFactory(className);
		return fIt!=factories.end()?fIt->factory:0;
		}
	ConstFactoryIterator begin(void) const // Returns iterator to the beginning of the managed class list
		{
		return ConstFactoryIterator(factories.begin());
		}
	FactoryIterator begin(void) // Returns iterator to the beginning of the managed class list
		{
		return FactoryIterator(factories.begin());
		}
	ConstFactoryIterator end(void) const // Returns iterator past the end of the managed class list
		{
		return ConstFactoryIterator(factories.end());
		}
	FactoryIterator end(void) // Returns iterator past the end of the managed class list
		{
		return FactoryIterator(factories.end());
		}
	};

}

#ifndef PLUGINS_FACTORYMANAGER_IMPLEMENTATION
#include <Plugins/FactoryManager.icpp>
#endif

#endif
