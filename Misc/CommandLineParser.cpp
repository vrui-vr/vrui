/***********************************************************************
CommandLineParser - Helper class to simplify parsing and processing
application command lines.
Copyright (c) 2025 Oliver Kreylos

This file is part of the Miscellaneous Support Library (Misc).

The Miscellaneous Support Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Miscellaneous Support Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Miscellaneous Support Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <Misc/CommandLineParser.icpp>

#include <Misc/StdError.h>
#include <Misc/StringHashFunctions.h>
#include <Misc/ValueCoder.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/FunctionCalls.h>

namespace Misc {

/*********************************************************
Declaration of class CommandLineParser::AddToListArgument:
*********************************************************/

class CommandLineParser::AddToListArgument:public CommandLineParser::Argument
	{
	/* Elements: */
	private:
	std::vector<std::string>& arguments; // Reference to the list collecting arguments
	
	/* Constructors and destructors: */
	public:
	AddToListArgument(std::vector<std::string>& sArguments)
		:arguments(sArguments)
		{
		}
	
	/* Methods from class Argument: */
	virtual void handle(const char* arg)
		{
		/* Append the argument to the list: */
		arguments.push_back(arg);
		}
	};

/********************************************************
Declaration of class CommandLineParser::CallbackArgument:
********************************************************/

class CommandLineParser::CallbackArgument:public CommandLineParser::Argument
	{
	/* Elements: */
	private:
	ArgumentCallback* argumentCallback; // Callback to be called for each argument
	
	/* Constructors and destructors: */
	public:
	CallbackArgument(ArgumentCallback* sArgumentCallback)
		:argumentCallback(sArgumentCallback)
		{
		}
	
	/* Methods from class Argument: */
	virtual void handle(const char* arg)
		{
		/* Call the argument callback: */
		(*argumentCallback)(arg);
		}
	};

/**************************************************
Declaration of class CommandLineParser::HelpOption:
**************************************************/

class CommandLineParser::HelpOption:public CommandLineParser::Option
	{
	/* Elements: */
	private:
	CommandLineParser& parser; // Reference to the parser object
	
	/* Constructors and destructors: */
	public:
	HelpOption(const char* sDescription,CommandLineParser& sParser)
		:Option(sDescription),
		 parser(sParser)
		{
		}
	
	/* Methods from class Option: */
	virtual char** parse(char* arg,char** argPtr,char** argEnd)
		{
		/* Call the parser's help method: */
		parser.printHelp();
		
		return argPtr;
		}
	};

/**************************************************
Declaration of class CommandLineParser::BoolOption:
**************************************************/

class CommandLineParser::BoolOption:public CommandLineParser::Option
	{
	/* Elements: */
	private:
	bool inverted; // Flag if the presence of the option sets the boolean value to false
	bool& value; // Reference to the flag indicating whether the option was present
	
	/* Constructors and destructors: */
	public:
	BoolOption(const char* sDescription,bool sInverted,bool& sValue)
		:Option(sDescription),
		 inverted(sInverted),value(sValue)
		{
		}
	
	/* Methods from class Option: */
	virtual char** parse(char* arg,char** argPtr,char** argEnd)
		{
		/* Set the boolean value according to the inversion flag: */
		value=!inverted;
		
		return argPtr;
		}
	};

/****************************************************
Declaration of class CommandLineParser::StringOption:
****************************************************/

class CommandLineParser::StringOption:public CommandLineParser::Option
	{
	/* Elements: */
	private:
	std::string argument; // String defining the value
	std::string& value; // Reference to the option value
	
	/* Constructors and destructors: */
	public:
	StringOption(const char* sDescription,const char* sArgument,std::string& sValue)
		:Option(sDescription),
		 argument(sArgument),
		 value(sValue)
		{
		}
	
	/* Methods from class Option: */
	virtual void printArguments(std::ostream& os) const
		{
		os<<' '<<argument;
		}
	virtual char** parse(char* arg,char** argPtr,char** argEnd)
		{
		/* Check if the string value is present: */
		if(argPtr==argEnd)
			throw Misc::makeStdErr(0,"%s: Missing value",arg);
		
		/* Set the option value: */
		value=*argPtr;
		
		return argPtr+1;
		}
	};

/******************************************************
Declaration of class CommandLineParser::CategoryOption:
******************************************************/

class CommandLineParser::CategoryOption:public CommandLineParser::Option
	{
	/* Elements: */
	private:
	std::vector<std::string> categories; // List of category names
	unsigned int& value; // Reference to the option value, which is the index of the selected category
	
	/* Constructors and destructors: */
	public:
	CategoryOption(const char* sDescription,unsigned int sNumCategories,const char* sCategories[],unsigned int& sValue)
		:Option(sDescription),
		 value(sValue)
		{
		/* Copy the category name array: */
		categories.reserve(sNumCategories);
		for(unsigned int i=0;i<sNumCategories;++i)
			categories.push_back(sCategories[i]);
		}
	CategoryOption(const char* sDescription,unsigned int sNumCategories,const std::string sCategories[],unsigned int& sValue)
		:Option(sDescription),
		 value(sValue)
		{
		/* Copy the category name array: */
		categories.reserve(sNumCategories);
		for(unsigned int i=0;i<sNumCategories;++i)
			categories.push_back(sCategories[i]);
		}
	CategoryOption(const char* sDescription,const std::vector<std::string>& sCategories,unsigned int& sValue)
		:Option(sDescription),
		 categories(sCategories),
		 value(sValue)
		{
		}
	
	/* Methods from class Option: */
	virtual void printArguments(std::ostream& os) const
		{
		/* Print the list of categories: */
		unsigned int numCategories=categories.size();
		if(numCategories==1U)
			os<<' '<<categories[0];
		else if(numCategories>1U)
			{
			os<<" ( "<<categories[0];
			for(unsigned int i=1;i<numCategories;++i)
				os<<" | "<<categories[i];
			os<<" )";
			}
		}
	virtual char** parse(char* arg,char** argPtr,char** argEnd)
		{
		/* Check if a category name is present: */
		if(argPtr==argEnd)
			throw Misc::makeStdErr(0,"%s: Missing category name",arg);
		
		/* Find the category name in the list: */
		unsigned int numCategories=categories.size();
		unsigned int i;
		for(i=0;i<numCategories&&categories[i]!=*argPtr;++i)
			;
		if(i>=numCategories)
			throw Misc::makeStdErr(0,"%s: Invalid category name %s",arg,*argPtr);
			
		/* Set the option value: */
		value=i;
		
		return argPtr+1;
		}
	};

/**********************************
Methods of class CommandLineParser:
**********************************/

void CommandLineParser::addOption(const char* longOption,const char* shortOption,Option* option)
	{
	/* Add the option to the set of options: */
	options.add(option);
	
	/* Add the new option to the map of long and/or short options: */
	if(longOption!=0)
		longOptions.setEntry(OptionMap::Entry(longOption,option));
	if(shortOption!=0)
		shortOptions.setEntry(OptionMap::Entry(shortOption,option));
	}

void CommandLineParser::printHelp(void)
	{
	/* Print a description for the application: */
	if(appName!=0)
		std::cout<<appName;
	if(appName!=0&&!description.empty())
		std::cout<<": ";
	if(!description.empty())
		std::cout<<": "<<description;
	if(appName!=0||!description.empty())
		std::cout<<std::endl<<std::endl;
	
	/* Print all defined options in the order in which they were defined: */
	std::cout<<"Command line options:"<<std::endl;
	for(OptionSet::iterator oIt=options.begin();oIt!=options.end();++oIt)
		{
		/* Get a pointer to the option object: */
		Option* option=&*oIt;
		
		std::cout<<"  ";
		
		/* Find the option object in the long and short option maps: */
		OptionMap::Iterator loIt;
		for(loIt=longOptions.begin();!loIt.isFinished()&&loIt->getDest()!=option;++loIt)
			;
		OptionMap::Iterator soIt;
		for(soIt=shortOptions.begin();!soIt.isFinished()&&soIt->getDest()!=option;++soIt)
			;
		
		if(!loIt.isFinished()&&!soIt.isFinished())
			std::cout<<"( --"<<loIt->getSource()<<" | -"<<soIt->getSource()<<" )";
		else if(!loIt.isFinished())
			std::cout<<"--"<<loIt->getSource();
		else if(!soIt.isFinished())
			std::cout<<"-"<<soIt->getSource();
		
		/* Print the option's arguments: */
		option->printArguments(std::cout);
		std::cout<<std::endl;
		
		/* Print the option's description: */
		std::cout<<"    "<<option->getDescription()<<std::endl;
		}
	
	helpPrinted=true;
	}

CommandLineParser::CommandLineParser(void)
	:longOptions(17),shortOptions(17),
	 argument(0),
	 appName(0),helpPrinted(false)
	{
	/* Create a help option object and add it to the set: */
	addOption("help","h",new HelpOption("Displays this help screen",*this));
	}

CommandLineParser::~CommandLineParser(void)
	{
	/* Delete the argument handler: */
	delete argument;
	}

void CommandLineParser::setDescription(const char* newDescription)
	{
	/* Save the description string: */
	description=newDescription;
	}

void CommandLineParser::stopOnArguments(void)
	{
	/* Remove any argument handlers: */
	delete argument;
	argument=0;
	}

void CommandLineParser::addArgumentsToList(std::vector<std::string>& arguments)
	{
	/* Replace the current argument handler: */
	delete argument;
	argument=new AddToListArgument(arguments);
	}

void CommandLineParser::callArgumentCallback(CommandLineParser::ArgumentCallback* newArgumentCallback)
	{
	/* Replace the current argument handler: */
	delete argument;
	argument=new CallbackArgument(newArgumentCallback);
	}

void CommandLineParser::addEnableOption(const char* longOption,const char* shortOption,bool& value,const char* description)
	{
	/* Create a new option object and add it to the set: */
	addOption(longOption,shortOption,new BoolOption(description,false,value));
	}

void CommandLineParser::addDisableOption(const char* longOption,const char* shortOption,bool& value,const char* description)
	{
	/* Create a new option object and add it to the set: */
	addOption(longOption,shortOption,new BoolOption(description,true,value));
	}

void CommandLineParser::addStringOption(const char* longOption,const char* shortOption,std::string& value,const char* argument,const char* description)
	{
	/* Create a new option object and add it to the set: */
	addOption(longOption,shortOption,new StringOption(description,argument,value));
	}

void CommandLineParser::addCategoryOption(const char* longOption,const char* shortOption,unsigned int numCategories,const char* categories[],unsigned int& value,const char* description)
	{
	/* Create a new option object and add it to the set: */
	addOption(longOption,shortOption,new CategoryOption(description,numCategories,categories,value));
	}

void CommandLineParser::addCategoryOption(const char* longOption,const char* shortOption,unsigned int numCategories,const std::string categories[],unsigned int& value,const char* description)
	{
	/* Create a new option object and add it to the set: */
	addOption(longOption,shortOption,new CategoryOption(description,numCategories,categories,value));
	}

void CommandLineParser::addCategoryOption(const char* longOption,const char* shortOption,const std::vector<std::string>& categories,unsigned int& value,const char* description)
	{
	/* Create a new option object and add it to the set: */
	addOption(longOption,shortOption,new CategoryOption(description,categories,value));
	}

char** CommandLineParser::parse(char** argPtr,char** argEnd)
	{
	/* If this is the first time parse() is called, save the application name: */
	if(appName==0)
		{
		appName=*argPtr;
		++argPtr;
		}
	
	/* Process all arguments in order: */
	while(argPtr!=argEnd)
		{
		/* Check whether this argument is a long or short option: */
		char* arg=*argPtr;
		if(arg[0]=='-')
			{
			/* Parse a long or short option: */
			OptionMap::Iterator oIt;
			if(arg[1]=='-')
				oIt=longOptions.findEntry(arg+2);
			else
				oIt=shortOptions.findEntry(arg+1);
			
			/* Check if the option was defined: */
			if(oIt.isFinished())
				throw Misc::makeStdErr(0,"Invalid option %s",arg);
			
			/* Parse the option: */
			argPtr=oIt->getDest()->parse(arg,argPtr+1,argEnd);
			
			/* Finish parsing if help was printed: */
			if(helpPrinted)
				argPtr=argEnd;
			}
		else
			{
			/* Handle a non-option argument: */
			if(argument!=0)
				argument->handle(*argPtr);
			else
				{
				/* Bail out to let the caller deal with the non-option argument: */
				break;
				}
			
			++argPtr;
			}
		}
	
	return argPtr;
	}

/***************************************************************
Force instantiation of all standard Point classes and functions:
***************************************************************/

template void CommandLineParser::addArrayOption<bool>(const char*,const char*,unsigned int,bool*,const char*,const char*);
template void CommandLineParser::addArrayOption<signed short>(const char*,const char*,unsigned int,signed short*,const char*,const char*);
template void CommandLineParser::addArrayOption<unsigned short>(const char*,const char*,unsigned int,unsigned short*,const char*,const char*);
template void CommandLineParser::addArrayOption<signed int>(const char*,const char*,unsigned int,signed int*,const char*,const char*);
template void CommandLineParser::addArrayOption<unsigned int>(const char*,const char*,unsigned int,unsigned int*,const char*,const char*);
template void CommandLineParser::addArrayOption<signed long>(const char*,const char*,unsigned int,signed long*,const char*,const char*);
template void CommandLineParser::addArrayOption<unsigned long>(const char*,const char*,unsigned int,unsigned long*,const char*,const char*);
template void CommandLineParser::addArrayOption<float>(const char*,const char*,unsigned int,float*,const char*,const char*);
template void CommandLineParser::addArrayOption<double>(const char*,const char*,unsigned int,double*,const char*,const char*);

}
