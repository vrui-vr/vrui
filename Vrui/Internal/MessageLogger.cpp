/***********************************************************************
MessageLogger - Class derived from Misc::MessageLogger to log and
present messages inside a Vrui application.
Copyright (c) 2015-2026 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <Vrui/Internal/MessageLogger.h>

#include <ctype.h>
#include <unistd.h>
#include <utility>
#include <string>
#include <Threads/RefCounted.h>
#include <Threads/FunctionCalls.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Margin.h>
#include <GLMotif/Label.h>
#include <GLMotif/Button.h>
#include <GLMotif/ScrolledListBox.h>
#include <Vrui/Vrui.h>

namespace Vrui {

/**************************************************
Declaration of class MessageLogger::PendingMessage:
**************************************************/

class MessageLogger::PendingMessage:public Threads::RefCounted
	{
	/* Elements: */
	public:
	Target target; // Message's target (User or Console)
	int messageLevel; // Severity level of the message
	std::string message; // The message string
	
	/* Constructors and destructors: */
	PendingMessage(Target sTarget,int sMessageLevel,const char* sMessage)
		:target(sTarget),messageLevel(sMessageLevel),message(sMessage)
		{
		}
	};

/******************************
Methods of class MessageLogger:
******************************/

void MessageLogger::logMessageInternal(Target target,int messageLevel,const char* message)
	{
	/* Reroute user messages to the console if requested: */
	if(target==User&&userToConsole)
		target=Console;
	
	/* Handle user and console messages graphically; log messages go to stderr: */
	if(target==User||target==Console)
		{
		/* Send a user signal with the message to the main thread: */
		logMessageSignal->signal(*new PendingMessage(target,messageLevel,message));
		}
	else if(isHeadNode())
		{
		/* Let the base class handle the message: */
		Misc::MessageLogger::logMessageInternal(target,messageLevel,message);
		}
	}

void MessageLogger::clearButtonCallback(Misc::CallbackData* cbData)
	{
	/* Clear the console's message list: */
	consoleMessageList->clear();
	}

void MessageLogger::logMessageCallback(Threads::UserSignalEvent& event)
	{
	/* Retrieve the pending message: */
	PendingMessage& message=event.getSignalData<PendingMessage>();
	
	/* Check if the message starts with a source identifier: */
	const char* messageString=message.message.c_str();
	const char* colonPtr=0;
	for(const char* mPtr=messageString;*mPtr!='\0'&&!isspace(*mPtr);++mPtr)
		if(*mPtr==':')
			colonPtr=mPtr;
	const char* source=0;
	const char* sourceEnd=0;
	if(colonPtr!=0&&colonPtr[1]!='\0'&&isspace(colonPtr[1]))
		{
		/* Split the message into source and body: */
		source=messageString;
		sourceEnd=colonPtr;
		messageString=colonPtr+1;
		while(*messageString!='\0'&&isspace(*messageString))
			++messageString;
		}
	
	/* Check if it's a user or console message: */
	if(message.target==User)
		{
		/* Assemble the dialog title: */
		std::string title=message.messageLevel<Warning?"Note":message.messageLevel<Error?"Warning":"Error";
		
		/* If the message has a source, append it to the title: */
		if(source!=0)
			{
			title.append(" from ");
			title.append(source,messageString-1);
			}
		
		/* Create an appropriate acknowledgment button label: */
		const char* buttonLabel=message.messageLevel<Warning?"Gee, thanks":message.messageLevel<Error?"Alright then":"Darn it!";
		
		/* Show a message dialog: */
		showErrorMessage(title.c_str(),messageString,buttonLabel);
		}
	else
		{
		/* Create the console dialog if it doesn't exist yet: */
		if(consoleDialog==0)
			{
			/* Create the console dialog shell: */
			consoleDialog=new GLMotif::PopupWindow("VruiConsole",getWidgetManager(),"Vrui Console");
			consoleDialog->setHideButton(true);
			consoleDialog->setCloseButton(true);
			consoleDialog->setResizableFlags(true,true);
			consoleDialog->popDownOnClose();
			
			/* Create the dialog body: */
			GLMotif::RowColumn* body=new GLMotif::RowColumn("Body",consoleDialog,false);
			body->setOrientation(GLMotif::RowColumn::VERTICAL);
			body->setPacking(GLMotif::RowColumn::PACK_TIGHT);
			
			/* Create the list box: */
			GLMotif::ScrolledListBox* scrolledList=new GLMotif::ScrolledListBox("ScrolledList",body,GLMotif::ListBox::ATMOST_ONE,40,5);
			consoleMessageList=scrolledList->getListBox();
			
			body->setRowWeight(0,1.0f);
			
			/* Create a button to clear the message list: */
			GLMotif::Margin* buttonMargin=new GLMotif::Margin("ButtonMargin",body,false);
			buttonMargin->setAlignment(GLMotif::Alignment::RIGHT);
			
			GLMotif::Button* clearButton=new GLMotif::Button("ClearButton",buttonMargin,"Clear");
			clearButton->getSelectCallbacks().add(this,&MessageLogger::clearButtonCallback);
			
			buttonMargin->manageChild();
			
			body->manageChild();
			}
		
		/* Assemble the full message string: */
		std::string fullMessageString=message.messageLevel<Warning?"Note":message.messageLevel<Error?"Warning":"Error";
		if(source!=0)
			{
			fullMessageString.append(" from ");
			fullMessageString.append(source,messageString-1);
			}
		fullMessageString.append(": ");
		fullMessageString.append(messageString);
		
		/* Add the full message string to the list of console messages: */
		consoleMessageList->addItem(fullMessageString.c_str(),true);
		
		/* Show the console dialog: */
		popupPrimaryWidget(consoleDialog);
		}
	}

MessageLogger::MessageLogger(void)
	:logMessageSignal(new Threads::UserSignal(getRunLoop(),true,*Threads::createFunctionCall(this,&MessageLogger::logMessageCallback))),
	 consoleDialog(0),consoleMessageList(0),
	 userToConsole(true)
	{
	/* Route console and log messages to stderr: */
	targetFds[Console]=STDERR_FILENO;
	targetFds[Log]=STDERR_FILENO;
	}

MessageLogger::~MessageLogger(void)
	{
	delete consoleDialog;
	}

void MessageLogger::setUserToConsole(bool newUserToConsole)
	{
	userToConsole=newUserToConsole;
	}

}
