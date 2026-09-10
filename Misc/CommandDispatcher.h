/***********************************************************************
CommandDispatcher - Class to dispatch text commands read from a file.
Copyright (c) 2020-2026 Oliver Kreylos

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

#ifndef MISC_COMMANDDISPATCHER_INCLUDED
#define MISC_COMMANDDISPATCHER_INCLUDED

#include <stddef.h>
#include <string>
#include <Misc/StringHashFunctions.h>
#include <Misc/HashTable.h>

namespace Misc {

class CommandDispatcher
	{
	/* Embedded classes: */
	public:
	typedef void (*CommandCallback)(const char* argumentBegin,const char* argumentEnd,void* userData); // Function type for callbacks handling commands
	template <class ClassParam,void (ClassParam::*methodParam)(const char* argumentBegin,const char* argumentEnd)>
	static void wrapMethod(const char* argumentBegin,const char* argumentEnd,void* userData) // Helper function to register class methods as command callback functions
		{
		(static_cast<ClassParam*>(userData)->*methodParam)(argumentBegin,argumentEnd);
		}
	
	struct CommandCallbackSlot // Structure holding a pipe command callback
		{
		friend class CommandDispatcher;
		
		/* Elements: */
		private:
		CommandCallback callback; // The callback function
		void* userData; // User-specified argument
		std::string arguments; // Human-readable description of command's arguments
		std::string description; // Command description
		
		/* Methods: */
		public:
		const std::string& getArguments(void) const // Returns human-readable description of the command's arguments
			{
			return arguments;
			}
		const std::string& getDescription(void) const // Returns human-readable description of the command
			{
			return description;
			}
		};
	
	typedef Misc::HashTable<std::string,CommandCallbackSlot> CommandMap; // Hash table mapping command tokens to command callbacks
	
	/* Elements: */
	private:
	CommandMap commandMap; // Map from command tokens to command callbacks
	size_t bufferSize; // Size of the command buffer
	char* buffer; // Pointer to the beginning of the command buffer
	char* writePtr; // Pointer to the first character that can be written into the command buffer
	
	/* Private methods: */
	void dispatchCommand(const char* commandBegin,const char* commandEnd,const char* argumentsBegin,const char* argumentsEnd); // Dispatches a command with an optional argument list
	static void listCommandsCallback(const char* argumentBegin,const char* argumentEnd,void* userData); // Lists all defined commands and their descriptions
	
	/* Constructors and destructors: */
	public:
	CommandDispatcher(void); // Creates a command dispatcher
	~CommandDispatcher(void); // Destroys the command dispatcher
	
	/* Methods: */
	bool addCommandCallback(const char* command,CommandCallback callback,void* userData,const char* arguments,const char* description); // Adds a command callback for the given command token with optional argument list and command description; returns false if command token was already claimed
	void removeCommandCallback(const char* command); // Removes a command callback for the given command token
	void dispatchCommand(const char* begin,const char* end); // Dispatches one complete command
	bool dispatchCommands(int commandFd); // Dispatches one or more complete commands read from the given file; returns true if there was an error
	const CommandMap& getCommandMap(void) const // Returns the map from command tokens to commands
		{
		return commandMap;
		}
	};

}

#endif
