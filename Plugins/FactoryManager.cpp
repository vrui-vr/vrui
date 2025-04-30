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

#include <Plugins/FactoryManager.h>

#include <stddef.h>
#include <stdio.h>
#include <dlfcn.h>
#include <Misc/StdError.h>

namespace Plugins {

/*********************************************
Methods of class FactoryManagerBase::DsoError:
*********************************************/

FactoryManagerBase::DsoError::DsoError(const char* source)
	:Error(Misc::makeStdErrMsg(source,"DSO error %s",dlerror()))
	{
	}

/***************************************************
Methods of class FactoryManagerBase::LoadDsoResults:
***************************************************/

FactoryManagerBase::LoadDsoResults::FunctionPointer FactoryManagerBase::LoadDsoResults::resolveFunction(const char* functionNameTemplate,const std::string& shortClassName)
	{
	/*********************************************************************
	For all DSO function names, try a generic name first, and then a
	class-specific name if the generic one doesn't exist. I want to move
	to always using generic names (less editing), but need to respect
	legacy factory classes.
	*********************************************************************/
	
	/*********************************************************************
	Convert the plain pointer returned by dlsym into a ptrdiff_t first to
	avoid warnings in g++'s pedantic mode. This is an evil hack, but g++
	breaks the entire idea of using DSOs, so there.
	*********************************************************************/
	
	ptrdiff_t result=0;
	
	/* Try the function name template without the class name first: */
	char functionName[256];
	snprintf(functionName,sizeof(functionName),functionNameTemplate,"");
	result=reinterpret_cast<ptrdiff_t>(dlsym(dsoHandle,functionName));
	if(result==0)
		{
		/* Try the function name template with the class name second: */
		snprintf(functionName,sizeof(functionName),functionNameTemplate,shortClassName.c_str());
		result=reinterpret_cast<ptrdiff_t>(dlsym(dsoHandle,functionName));
		}
	
	/* Cast the resolved function name to a generic function pointer and return it: */
	return reinterpret_cast<FunctionPointer>(result);
	}

/***********************************
Methods of class FactoryManagerBase:
***********************************/

std::pair<std::string,bool> FactoryManagerBase::extractClassName(const char* className) const
	{
	/* Check if the class name has a path prefix and find the start and end of the class name: */
	const char* cnBegin=className;
	const char* cnEnd;
	for(cnEnd=className;*cnEnd!='\0';++cnEnd)
		if(*cnEnd=='/')
			cnBegin=cnEnd+1;
	
	/* Match the beginning of the class name against the DSO name template's prefix before the %s placeholder: */
	const char* cnPtr1=cnBegin;
	std::string::const_iterator dntIt1=dsoNameTemplate.begin();
	while(cnPtr1!=cnEnd&&*dntIt1!='%'&&*cnPtr1==*dntIt1)
		{
		++cnPtr1;
		++dntIt1;
		}
	
	if(*dntIt1=='%')
		{
		/* Skip the "%s" in the DSO name template: */
		dntIt1+=2;
		
		/* Match the end of the class name against the template's suffix after the %s placeholder: */
		const char* cnPtr2=cnEnd;
		std::string::const_iterator dntIt2=dsoNameTemplate.end();
		while(cnPtr2!=cnPtr1&&dntIt2!=dntIt1&&cnPtr2[-1]==dntIt2[-1])
			{
			--cnPtr2;
			--dntIt2;
			}
		
		if(dntIt2==dntIt1)
			{
			/* The template has been matched, return the contained class name: */
			return std::make_pair(std::string(cnPtr1,cnPtr2),false);
			}
		}
	
	/* Throw an error if there is a path prefix on a class name that does not match the DSO name template: */
	if(cnBegin!=className)
		throw Error(Misc::makeStdErrMsg(__PRETTY_FUNCTION__,"Invalid class name %s",className));
	
	/* Return the class name as given: */
	return std::make_pair(className,true);
	}

FactoryManagerBase::LoadDsoResults FactoryManagerBase::loadDso(const char* className,bool applyTemplate,const std::string& shortClassName) const
	{
	LoadDsoResults result;
	
	/* Locate the DSO containing the class implementation: */
	std::string fullDsoName;
	try
		{
		/* Locate the DSO defining the requested class: */
		if(applyTemplate)
			{
			/* Construct the DSO name from the given class name: */
			char dsoName[256];
			snprintf(dsoName,sizeof(dsoName),dsoNameTemplate.c_str(),className);
			fullDsoName=dsoLocator.locateFile(dsoName);
			}
		else
			{
			/* Use the given class name as-is: */
			fullDsoName=dsoLocator.locateFile(className);
			}
		}
	catch(const std::runtime_error& err)
		{
		/* Re-throw the error as a factory manager error: */
		throw Error(err.what());
		}
	
	/* Open the located DSO and check for errors: */
	result.dsoHandle=dlopen(fullDsoName.c_str(),RTLD_LAZY|RTLD_GLOBAL);
	if(result.dsoHandle==0)
		throw DsoError(__PRETTY_FUNCTION__);
	
	/* Get the address of the optional dependency resolution function (if it exists): */
	result.resolveDependencies=result.resolveFunction("resolve%sDependencies",shortClassName);
	
	/* Get the address of the factory creation function: */
	result.createFactory=result.resolveFunction("create%sFactory",shortClassName);
	if(result.createFactory==0)
		throw DsoError(__PRETTY_FUNCTION__);
	
	/* Get the address of the factory destruction function: */
	result.destroyFactory=result.resolveFunction("destroy%sFactory",shortClassName);
	if(result.destroyFactory==0)
		throw DsoError(__PRETTY_FUNCTION__);
	
	return result;
	}

FactoryManagerBase::FactoryManagerBase(const std::string& sDsoNameTemplate)
	{
	/* Split the DSO name template into base directory and file name and check it for validity: */
	std::string::const_iterator templateStart=sDsoNameTemplate.begin();
	bool haveClassname=false;
	for(std::string::const_iterator sIt=sDsoNameTemplate.begin();sIt!=sDsoNameTemplate.end();++sIt)
		{
		if(*sIt=='/'&&!haveClassname) // Find the last slash before the %s placeholder
			templateStart=sIt+1;
		else if(*sIt=='%') // Check for invalid placeholders
			{
			++sIt;
			if(sIt==sDsoNameTemplate.end()||*sIt!='s'||haveClassname)
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid DSO name template %s",sDsoNameTemplate.c_str());
			else
				haveClassname=true;
			}
		}
	if(!haveClassname)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid DSO name template %s",sDsoNameTemplate.c_str());
	dsoNameTemplate=std::string(templateStart,sDsoNameTemplate.end());
	
	/* If the DSO name template has a path prefix, use that path as the first search path in the DSO file locator: */
	if(templateStart!=sDsoNameTemplate.begin())
		dsoLocator.addPath(std::string(sDsoNameTemplate.begin(),templateStart));
	}

}
