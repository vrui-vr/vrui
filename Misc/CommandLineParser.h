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

#ifndef MISC_COMMANDLINEPARSER_INCLUDED
#define MISC_COMMANDLINEPARSER_INCLUDED

#include <string>
#include <vector>
#include <iostream>
#include <Misc/SimpleObjectSet.h>
#include <Misc/HashTable.h>

/* Forward declarations: */
namespace Misc {
template <class ParameterParam>
class FunctionCall;
}

namespace Misc {

class CommandLineParser
	{
	/* Embedded classes: */
	private:
	class Argument // Base class for argument handlers
		{
		/* Constructors and destructors: */
		public:
		virtual ~Argument(void)
			{
			}
		
		/* Methods: */
		virtual void handle(const char* arg) =0;
		};
	
	/* Concrete argument handler classes: */
	class AddToListArgument;
	class CallbackArgument;
	
	public:
	typedef Misc::FunctionCall<const char*> ArgumentCallback; // Type for argument handling callbacks
	
	private:
	class Option // Base class for option handlers
		{
		/* Elements: */
		protected:
		std::string description; // Option description displayed on help pages
		
		/* Constructors and destructors: */
		public:
		Option(const char* sDescription)
			{
			if(sDescription!=0)
				description=sDescription;
			}
		virtual ~Option(void)
			{
			}
		
		/* Methods: */
		const std::string& getDescription(void) const // Returns the option's description
			{
			return description;
			}
		virtual void printArguments(std::ostream& os) const // Prints the definition of the option's arguments to the given stream
			{
			/* Do nothing: */
			}
		virtual char** parse(char* arg,char** argPtr,char** argEnd) =0; // Parses an option starting from the given argument; returns pointer to next argument
		};
	
	typedef Misc::SimpleObjectSet<Option> OptionSet; // Type for sets of option objects
	typedef Misc::HashTable<std::string,Option*> OptionMap; // Type for hash tables mapping long or short option codes to option objects
	
	/* Concrete option handler classes: */
	class HelpOption;
	friend class HelpOption;
	class BoolOption;
	class StringOption;
	class CategoryOption;
	template <class ValueParam>
	class ArrayOption;
	
	/* Elements: */
	std::string description; // Description of the application
	OptionSet options; // List of options in the order in which they were added to the parser
	OptionMap longOptions; // Map of defined long options
	OptionMap shortOptions; // Map of defined short options
	Argument* argument; // Pointer to handler for non-option arguments
	const char* appName; // Name of the application; copied from first command line argument
	bool helpPrinted; // Flag whether help was requested during parsing
	
	/* Private methods: */
	void addOption(const char* longOption,const char* shortOption,Option* option); // Adds the given option object to the parser
	void printHelp(void); // Prints a help screen
	
	/* Constructors and destructors: */
	public:
	CommandLineParser(void);
	~CommandLineParser(void);
	
	/* Methods: */
	void setDescription(const char* newDescription); // Sets the application description
	void stopOnArguments(void); // parse() method returns when a non-option argument is encountered
	void addArgumentsToList(std::vector<std::string>& arguments); // Adds encountered arguments to the given list
	void callArgumentCallback(ArgumentCallback* newArgumentCallback); // Calls the given callback when an argument is encountered
	void addEnableOption(const char* longOption,const char* shortOption,bool& value,const char* description);
	void addDisableOption(const char* longOption,const char* shortOption,bool& value,const char* description);
	void addStringOption(const char* longOption,const char* shortOption,std::string& value,const char* argument,const char* description);
	void addCategoryOption(const char* longOption,const char* shortOption,unsigned int numCategories,const char* categories[],unsigned int& value,const char* description);
	void addCategoryOption(const char* longOption,const char* shortOption,unsigned int numCategories,const std::string categories[],unsigned int& value,const char* description);
	void addCategoryOption(const char* longOption,const char* shortOption,const std::vector<std::string>& categories,unsigned int& value,const char* description);
	template <class ValueParam>
	void addArrayOption(const char* longOption,const char* shortOption,unsigned int numValues,ValueParam* values,const char* arguments,const char* description);
	char** parse(char** argPtr,char** argEnd);
	bool hadHelp(void) const // Returns true if the parsed command line contained the -h or --help options
		{
		return helpPrinted;
		}
	};

}

#endif
