/***********************************************************************
MessageLogger - Base class for objects that receive and log messages.
Copyright (c) 2015-2024 Oliver Kreylos

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

#include <Misc/MessageLogger.h>

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <string>
#include <Misc/ParsePrettyFunction.h>

namespace Misc {

/**************************************
Static elements of class MessageLogger:
**************************************/

Autopointer<MessageLogger> MessageLogger::theMessageLogger(new MessageLogger);

/******************************
Methods of class MessageLogger:
******************************/

void MessageLogger::logMessageInternal(MessageLogger::Target target,int messageLevel,const char* message)
	{
	/* Check if the message should start with a time stamp: */
	std::string paddedMessage;
	if(printTimeStamps[target])
		{
		/* Get the current time and break UNIX time into local time: */
		time_t t=time(0);
		struct tm lt;
		if(localtime_r(&t,&lt)!=0)
			{
			char tsBuffer[128];
			snprintf(tsBuffer,sizeof(tsBuffer),"%04d/%02d/%02d,%02d:%02d:%02d ",lt.tm_year+1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec);
			paddedMessage.append(tsBuffer);
			}
		}
	
	/* Append the message and a newline: */
	paddedMessage.append(message);
	paddedMessage.push_back('\n');
	
	/* Write message directly to the target file, bypassing any buffers: */
	if(write(targetFds[target],paddedMessage.data(),paddedMessage.size())!=ssize_t(paddedMessage.size()))
		{
		/* Whatcha gonna do? */
		}
	}

MessageLogger::MessageLogger(void)
	:minMessageLevel(Note)
	{
	/* Initialize the target file descriptor array: */
	targetFds[Log]=STDOUT_FILENO;
	targetFds[Console]=STDOUT_FILENO;
	targetFds[User]=STDERR_FILENO;
	
	/* Disable time stamps for all targets: */
	for(int i=0;i<3;++i)
		printTimeStamps[i]=false;
	}

MessageLogger::~MessageLogger(void)
	{
	/* Close the current log target if it is an actual file: */
	if(targetFds[Log]!=STDOUT_FILENO)
		close(targetFds[Log]);
	}

void MessageLogger::setMessageLogger(Autopointer<MessageLogger> newMessageLogger)
	{
	theMessageLogger=newMessageLogger;
	}

void MessageLogger::setLogFile(const char* logFileName)
	{
	/* Close the current log target if it is an actual file: */
	if(targetFds[Log]!=STDOUT_FILENO)
		close(targetFds[Log]);
	
	if(logFileName!=0)
		{
		/* Open the log file and set it as the target for log messages: */
		targetFds[Log]=open(logFileName,O_WRONLY|O_CREAT|O_APPEND,0666); // Give full permissions to everyone; umask will take care of it
		if(targetFds[Log]<0)
			{
			/* Route log messages to stdout and print an error message: */
			targetFds[Log]=STDOUT_FILENO;
			formattedUserError("Misc::MessageLogger::setLogFile: Unable to route log messages to file %s due to error %d(%s)",logFileName,errno,strerror(errno));
			}
		}
	else
		{
		/* Route log messages to stdout: */
		targetFds[Log]=STDOUT_FILENO;
		}
	}

void MessageLogger::setMinMessageLevel(int newMinMessageLevel)
	{
	minMessageLevel=newMinMessageLevel;
	}

void MessageLogger::setPrintTimeStamps(Target target,bool newPrintTimeStamps)
	{
	printTimeStamps[target]=newPrintTimeStamps;
	}

void MessageLogger::logMessage(MessageLogger::Target target,int messageLevel,const char* message)
	{
	/* Log the message if there is a logger and the message exceeds the minimum severity level: */
	if(theMessageLogger!=0&&messageLevel>=theMessageLogger->minMessageLevel)
		theMessageLogger->logMessageInternal(target,messageLevel,message);
	}

void MessageLogger::logFormattedMessage(MessageLogger::Target target,int messageLevel,const char* formatString,...)
	{
	/* Log the message if there is a logger and the message exceeds the minimum severity level: */
	if(theMessageLogger!=0&&messageLevel>=theMessageLogger->minMessageLevel)
		{
		/* Print the message into a local buffer: */
		char message[1024]; // Buffer for error messages - hopefully long enough...
		va_list ap;
		va_start(ap,formatString);
		vsnprintf(message,sizeof(message),formatString,ap);
		va_end(ap);
		
		/* Log the message: */
		theMessageLogger->logMessageInternal(target,messageLevel,message);
		}
	}

void MessageLogger::logFormattedMessage(MessageLogger::Target target,int messageLevel,const char* formatString,va_list args)
	{
	/* Log the message if there is a logger and the message exceeds the minimum severity level: */
	if(theMessageLogger!=0&&messageLevel>=theMessageLogger->minMessageLevel)
		{
		/* Print the message into a local buffer: */
		char message[1024]; // Buffer for error messages - hopefully long enough...
		vsnprintf(message,sizeof(message),formatString,args);
		
		/* Log the message: */
		theMessageLogger->logMessageInternal(target,messageLevel,message);
		}
	}

void MessageLogger::logFormattedMessage(const char* prettyFunction,Target target,int messageLevel,const char* formatString,...)
	{
	if(theMessageLogger!=0&&messageLevel>=theMessageLogger->minMessageLevel)
		{
		/* Print the message into a local buffer: */
		char buffer[1024];
		char* bufferStart=buffer;
		char* bufferEnd=buffer+sizeof(buffer);
		
		/* Copy the source function name into the message buffer: */
		bufferStart=parsePrettyFunction(prettyFunction,bufferStart,bufferEnd-3); // Leave room for ": " separator and NUL terminator
		
		/* Append the location separator: */
		*(bufferStart++)=':';
		*(bufferStart++)=' ';
		
		/* Append the given message to the message buffer: */
		va_list ap;
		va_start(ap,formatString);
		bufferStart+=vsnprintf(bufferStart,bufferEnd-bufferStart,formatString,ap);
		va_end(ap);
		
		/* Log the message: */
		theMessageLogger->logMessageInternal(target,messageLevel,buffer);
		}
	}

void MessageLogger::logFormattedMessage(const char* prettyFunction,Target target,int messageLevel,const char* formatString,va_list args)
	{
	if(theMessageLogger!=0&&messageLevel>=theMessageLogger->minMessageLevel)
		{
		/* Print the message into a local buffer: */
		char buffer[1024];
		char* bufferStart=buffer;
		char* bufferEnd=buffer+sizeof(buffer);
		
		/* Copy the source function name into the message buffer: */
		bufferStart=parsePrettyFunction(prettyFunction,bufferStart,bufferEnd-3); // Leave room for ": " separator and NUL terminator
		
		/* Append the location separator: */
		*(bufferStart++)=':';
		*(bufferStart++)=' ';
		
		/* Append the given message to the message buffer: */
		bufferStart+=vsnprintf(bufferStart,bufferEnd-bufferStart,formatString,args);
		
		/* Log the message: */
		theMessageLogger->logMessageInternal(target,messageLevel,buffer);
		}
	}

}
