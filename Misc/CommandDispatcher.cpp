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

#include <Misc/CommandDispatcher.h>

#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <Misc/MessageLogger.h>

namespace Misc {

/**********************************
Methods of class CommandDispatcher:
**********************************/

void CommandDispatcher::dispatchCommand(const char* commandBegin,const char* commandEnd,const char* argumentsBegin,const char* argumentsEnd)
	{
	/* Check if there really is a command: */
	if(commandEnd!=commandBegin)
		{
		/* Find a handler for the command: */
		std::string command(commandBegin,commandEnd);
		CommandMap::Iterator cmIt=commandMap.findEntry(command);
		if(!cmIt.isFinished())
			{
			try
				{
				/* Call the callback: */
				cmIt->getDest().callback(argumentsBegin,argumentsEnd,cmIt->getDest().userData);
				}
			catch(const std::runtime_error& err)
				{
				Misc::formattedLogError("CommandDispatcher: Caught exception %s while handling command %s %s",err.what(),command.c_str(),std::string(argumentsBegin,argumentsEnd).c_str());
				}
			}
		else
			Misc::formattedLogError("CommandDispatcher: Unrecognized command %s",command.c_str());
		}
	}

void CommandDispatcher::listCommandsCallback(const char* argumentBegin,const char* argumentEnd,void* userData)
	{
	/* Embedded classes: */
	class Compare
		{
		/* Methods: */
		public:
		bool operator()(const CommandMap::Iterator& it1,const CommandMap::Iterator& it2)
			{
			return it1->getSource()<it2->getSource();
			}
		};
	
	CommandDispatcher* thisPtr=static_cast<CommandDispatcher*>(userData);
	
	/* Put all command callback slots into a vector and sort them alphabetically: */
	std::vector<CommandMap::Iterator> ccs;
	ccs.reserve(thisPtr->commandMap.getNumEntries());
	for(CommandMap::Iterator cmIt=thisPtr->commandMap.begin();!cmIt.isFinished();++cmIt)
		ccs.push_back(cmIt);
	std::sort(ccs.begin(),ccs.end(),Compare());
	
	/* List all defined commands with their optional argument lists and descriptions: */
	bool first=true;
	for(std::vector<CommandMap::Iterator>::iterator ccIt=ccs.begin();ccIt!=ccs.end();++ccIt)
		{
		if(!first)
			Misc::consoleNote("");
		first=false;
		
		/* Print the command token and its optional argument list: */
		CommandMap::Iterator cmIt=*ccIt;
		CommandCallbackSlot& ccs=cmIt->getDest();
		if(!ccs.arguments.empty())
			Misc::formattedConsoleNote("%s %s",cmIt->getSource().c_str(),ccs.arguments.c_str());
		else
			Misc::consoleNote(cmIt->getSource().c_str());
		
		/* Print the optional description: */
		if(!ccs.description.empty())
			Misc::formattedConsoleNote("    %s",ccs.description.c_str());
		}
	}

CommandDispatcher::CommandDispatcher(void)
	:commandMap(17),
	 bufferSize(1024),buffer(new char[bufferSize]),writePtr(buffer)
	{
	/* Define a command to list all defined commands: */
	addCommandCallback("listCommands",listCommandsCallback,this,0,"Prints all defined commands and their descriptions");
	addCommandCallback("help",listCommandsCallback,this,0,"Prints all defined commands and their descriptions");
	}

CommandDispatcher::~CommandDispatcher(void)
	{
	/* Destroy the command reading buffer: */
	delete[] buffer;
	}

bool CommandDispatcher::addCommandCallback(const char* command,CommandCallback callback,void* userData,const char* arguments,const char* description)
	{
	/* Check if the command is already claimed: */
	std::string cmd(command);
	if(commandMap.isEntry(cmd))
		return false;
	
	/* Add the callback: */
	CommandCallbackSlot ccs;
	ccs.callback=callback;
	ccs.userData=userData;
	
	/* Copy the optional argument list and command description: */
	if(arguments!=0)
		ccs.arguments=arguments;
	if(description!=0)
		ccs.description=description;
	
	/* Store the command callback: */
	commandMap.setEntry(CommandMap::Entry(cmd,ccs));
	
	return true;
	}

void CommandDispatcher::removeCommandCallback(const char* command)
	{
	/* Remove the callback: */
	commandMap.removeEntry(command);
	}

void CommandDispatcher::dispatchCommand(const char* begin,const char* end)
	{
	/* Find the beginning of the command token: */
	while(begin!=end&&isspace(*begin))
		++begin;
	const char* commandBegin=begin;
	
	/* Find the end of the command token: */
	while(begin!=end&&!isspace(*begin))
		++begin;
	const char* commandEnd=begin;
	
	/* Find the beginning of an optional argument list: */
	while(begin!=end&&isspace(*begin))
		++begin;
	const char* argumentsBegin=begin;
	
	/* Dispatch the command: */
	dispatchCommand(commandBegin,commandEnd,argumentsBegin,end);
	}

bool CommandDispatcher::dispatchCommands(int commandFd)
	{
	/* Read from the command file into the command buffer and check for errors: */
	ssize_t readSize=read(commandFd,writePtr,(buffer+bufferSize)-writePtr);
	if(readSize<=0)
		{
		/* Figure out the error condition: */
		bool result=true;
		if(readSize==0)
			Misc::formattedLogWarning("CommandDispatcher: Command file %d was closed; not accepting further commands",commandFd);
		else if(errno!=EAGAIN&&errno!=EWOULDBLOCK)
			Misc::formattedLogError("CommandDispatcher: Read error %d (%s) from command file %d; not accepting further commands",commandFd,errno,strerror(errno));
		else
			{
			/* Wasn't actually an error, just no data to read: */
			result=false;
			}
		
		return result;
		}
	
	/* Process all full command lines in the command buffer: */
	writePtr+=readSize;
	char* readPtr=buffer;
	while(readPtr!=writePtr)
		{
		/* Find the beginning of the command token: */
		while(readPtr!=writePtr&&*readPtr!='\n'&&isspace(*readPtr))
			++readPtr;
		char* commandBegin=readPtr;
		
		/* Find the end of the command token: */
		while(readPtr!=writePtr&&!isspace(*readPtr))
			++readPtr;
		char* commandEnd=readPtr;
		
		/* Find the beginning of an optional argument list: */
		while(readPtr!=writePtr&&*readPtr!='\n'&&isspace(*readPtr))
			++readPtr;
		char* argumentsBegin=readPtr;
		
		/* Find the end of the current command line: */
		while(readPtr!=writePtr&&*readPtr!='\n')
			++readPtr;
		
		/* Check if the current command line is not finished: */
		if(readPtr==writePtr)
			{
			/*****************************************************************
			We need to stop processing here and read more data from the
			command file to complete the current command line. We can't call
			read() again because that might block. So we'll move what we have
			of the current command line to the beginning of the buffer,
			increase the buffer size if the buffer is full, and bail out.
			*****************************************************************/
			
			/* Calculate the length of the unfinished line: */
			size_t lineLength=readPtr-commandBegin;
			if(lineLength<bufferSize)
				{
				/* Move the unfinished line to the beginning of the buffer: */
				memmove(buffer,commandBegin,lineLength);
				}
			else
				{
				/* Allocate a new, larger buffer and copy the unfinished line into it: */
				size_t newBufferSize=bufferSize*2;
				char* newBuffer=new char[newBufferSize];
				memcpy(newBuffer,commandBegin,lineLength);
				
				/* Replace the current buffer: */
				delete[] buffer;
				bufferSize=newBufferSize;
				buffer=newBuffer;
				}
			
			/* Remember the end of the unfinished line in the command buffer and bail out: */
			writePtr=buffer+lineLength;
			
			return false;
			}
		
		/* Strip a potential CR from the end of the command line, but don't strip any other whitespace: */
		char* argumentsEnd=readPtr;
		if(argumentsEnd!=argumentsBegin&&argumentsEnd[-1]=='\r')
			--argumentsEnd;
		
		/* Dispatch the command: */
		dispatchCommand(commandBegin,commandEnd,argumentsBegin,argumentsEnd);
		
		/* Go to the next command line by skipping the LF: */
		++readPtr;
		}
	
	/* Mark the command buffer as empty: */
	writePtr=buffer;
	
	return false;
	}

}
