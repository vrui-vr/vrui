/***********************************************************************
HttpServer - Class implementing a simple server listening for incoming
HTTP connections and serving HTTP requests.
Copyright (c) 2026 Oliver Kreylos

This file is part of the Portable Communications Library (Comm).

The Portable Communications Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Portable Communications Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Portable Communications Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <Comm/HttpServer.h>

#include <stdexcept>
#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>
#include <Threads/FunctionCalls.h>
#include <IO/VariableMemoryFile.h>
#include <IO/OStream.h>
#include <IO/JsonEntity.h>
#include <Comm/ListeningTCPSocket.h>
#include <Comm/HttpRequestHeader.h>

// DEBUGGING
#include <iostream>

namespace Comm {

/*****************************************
Methods of struct HttpServer::PostRequest:
*****************************************/

void HttpServer::PostRequest::setJsonResult(const IO::JsonEntity& json)
	{
	/* Mark the result as valid and set the appropriate content type: */
	resultValid=true;
	resultContentType="application/json";
	
	/* Write the JSON entity to the result file using an IO::OStream layer: */
	IO::OStream result(&resultFile);
	result<<json;
	}

/***************************************
Methods of class HttpServer::Connection:
***************************************/

void HttpServer::Connection::close(void)
	{
	// DEBUGGING
	std::cout<<"Comm::HttpServer: Closing HTTP connection"<<std::endl;
	
	/* Remove this connection from the server's set of active connections: */
	server.connections.remove(this);
	
	/* Destroy this connection, which will clean up everything: */
	delete this;
	}

bool HttpServer::Connection::processRequest(bool valid)
	{
	/* Check if the current request is a valid POST request: */
	if(valid)
		{
		/* Create a temporary in-memory file to receive the request results: */
		IO::VariableMemoryFile resultFile;
		resultFile.ref();
		
		/* Create the POST request structure and call the POST request handler: */
		PostRequest postRequest(requestHeader->getRequestUri(),parameters,resultFile);
		if(server.postRequestHandler!=0)
			(*server.postRequestHandler)(postRequest);
		
		/* Check if the request generated a valid result: */
		if(postRequest.resultValid)
			{
			/* Write the HTTP reply header: */
			{
			IO::OStream reply(pipe);
			reply<<"HTTP/1.1 200 OK\r\n";
			reply<<"Content-Type: "<<postRequest.resultContentType<<"\r\n";
			reply<<"Content-Length: "<<resultFile.getDataSize()<<"\r\n";
			reply<<"Connection: "<<(requestHeader->getKeepAlive()?"keep-alive":"close")<<"\r\n";
			
			/* Take care of CORS: */
			if(requestHeader->hasHeaderField("origin"))
				reply<<"Access-Control-Allow-Origin: "<<requestHeader->getHeaderFieldValue("origin")<<"\r\n";
			
			reply<<"\r\n";
			}
			
			/* Append the result file to the HTTP reply: */
			resultFile.flush();
			resultFile.rewind();
			resultFile.writeToSink(*pipe);
			}
		else
			{
			/* Mark the request as invalid to send an error reply: */
			valid=false;
			}
		}
		
	if(!valid)
		{
		/* Send an HTTP error: */
		IO::OStream reply(pipe);
		reply<<"HTTP/1.1 400 Bad Request\r\n";
		reply<<"\r\n";
		}
	
	/* Send the reply: */
	pipe->flush();
	
	/* Check if the client wants to keep the underlying connection alive: */
	if(requestHeader->getKeepAlive())
		{
		/* Delete the request header and parameter list and go back to Start state: */
		delete requestHeader;
		requestHeader=0;
		parameters.clear();
		state=Start;
		
		return false;
		}
	else
		{
		/* Close the connection and tell the caller to bail out: */
		close();
		
		return true;
		}
	}

bool HttpServer::Connection::ignoreRequest(void)
	{
	/* Check if the current request has a body: */
	if(requestHeader->hasContentLength())
		{
		/* Skip the request's body: */
		state=SkippingRequestContents;
		contentLength=requestHeader->getContentLength();
		
		return false;
		}
	else
		{
		/* Process the request immediately: */
		return processRequest(false);
		}
	}

void HttpServer::Connection::pipeCallback(Threads::RunLoop::IOWatcher::Event& event)
	{
	try
		{
		/* Read more data from the pipe: */
		pipe->readSomeData();
		
		/* Check if the client hung up and all sent data has been read: */
		if(pipe->eof())
			{
			/* Check if there is unfinished business: */
			if(state!=Start)
				Misc::sourcedConsoleWarning(__PRETTY_FUNCTION__,"HTTP connection closed unexpectedly");
			
			/* Close the connection and bail out: */
			close();
			return;
			}
		
		/* Handle the HTTP connection state machine while there is unread data in the pipe's read buffer: */
		while(state!=Closed&&pipe->canReadImmediately())
			{
			switch(state)
				{
				case Start:
					/* Create a new HTTP request header: */
					requestHeader=new HttpRequestHeader;
					
					/* Start reading the HTTP request header: */
					state=ReadingHttpRequestHeader;
					
					break;
				
				case ReadingHttpRequestHeader:
					/* Continue reading the current HTTP request header and check if it's done: */
					if(requestHeader->read(*pipe))
						{
						switch(requestHeader->getRequestType())
							{
							case HttpRequestHeader::Options:
								{
								/* Print the OPTIONS request for posterity: */
								std::cout<<"Comm::HttpServer: Received OPTIONS request with header fields:"<<std::endl;
								const HttpRequestHeader::NameValueList& headerFields=requestHeader->getHeaderFields();
								for(HttpRequestHeader::NameValueList::const_iterator hfIt=headerFields.begin();hfIt!=headerFields.end();++hfIt)
									std::cout<<'\t'<<hfIt->name<<": "<<hfIt->value<<std::endl;
								
								/* Ignore the request otherwise: */
								if(ignoreRequest())
									return;
								
								break;
								}
							
							case HttpRequestHeader::Post:
								/* Check that the POST request has the correct encoding: */
								if(requestHeader->getHeaderFieldValue("content-type")!="application/x-www-form-urlencoded")
									throw Misc::makeStdErr(0,"Wrong content type %s in POST request",requestHeader->getHeaderFieldValue("content-type").c_str());
								
								/* Check if the POST request has parameters: */
								if(!requestHeader->hasContentLength())
									throw Misc::makeStdErr(0,"No Content-Length header field in POST request");
								contentLength=requestHeader->getContentLength();
								if(contentLength>0)
									{
									/* Start reading the first parameter name: */
									state=ReadingParameterName;
									}
								else
									{
									/* Process the request: */
									if(processRequest(true))
										return;
									}
								
								break;
							
							default:
								/* Ignore the unknown request: */
								if(ignoreRequest())
									return;
							}
						}
					
					break;
				
				case ReadingParameterName:
					{
					/* Access the pipe's read buffer: */
					void* buffer;
					size_t bufferSize=pipe->readInBuffer(buffer,contentLength);
					char* readBuffer=static_cast<char*>(buffer);
					char* rPtr=readBuffer;
					char* rEnd=rPtr+bufferSize;
					
					/* Tentatively reduce the request's remaining content length by the buffer's size, which is at most contentLength: */
					contentLength-=bufferSize;
					
					/* Add to the current parameter name: */
					while(rPtr!=rEnd&&*rPtr!='='&&*rPtr!='&')
						parameter.name.push_back(*(rPtr++));
					
					/* Check if the parameter name is complete: */
					if(rPtr!=rEnd)
						{
						/* Skip the separating character and return the unread part of the read buffer to the pipe and fix the remaining content length: */
						++rPtr;
						pipe->putBackInBuffer(rEnd-rPtr);
						contentLength+=rEnd-rPtr;
						
						/* Check if the request is now fully read: */
						if(contentLength==0)
							{
							/* Add a parameter with an empty value to the request's parameter list and process the request: */
							parameters.push_back(parameter);
							parameter.name.clear();
							if(processRequest(true))
								return;
							}
						else if(rPtr[-1]=='=')
							{
							/* Start reading a parameter value: */
							state=ReadingParameterValue;
							}
						else
							{
							/* Add a parameter with an empty value to the request's parameter list and start reading the next parameter name: */
							parameters.push_back(parameter);
							parameter.name.clear();
							}
						}
					else if(contentLength==0)
						{
						/* Add a parameter with an empty value to the request's parameter list and process the request: */
						parameters.push_back(parameter);
						parameter.name.clear();
						if(processRequest(true))
							return;
						}
					
					break;
					}
				
				case ReadingParameterValue:
					{
					/* Access the pipe's read buffer: */
					void* buffer;
					size_t bufferSize=pipe->readInBuffer(buffer,contentLength);
					char* readBuffer=static_cast<char*>(buffer);
					char* rPtr=readBuffer;
					char* rEnd=rPtr+bufferSize;
					
					/* Tentatively reduce the request's remaining content length by the buffer's size, which is at most contentLength: */
					contentLength-=bufferSize;
					
					/* Add to the current parameter value: */
					while(rPtr!=rEnd&&*rPtr!='&')
						parameter.value.push_back(*(rPtr++));
					
					/* Check if the parameter value is complete: */
					if(rPtr!=rEnd)
						{
						/* Skip the separating ampersand and return the unread part of the read buffer to the pipe and fix the remaining content length: */
						++rPtr;
						pipe->putBackInBuffer(rEnd-rPtr);
						contentLength+=rEnd-rPtr;
						
						/* Add a parameter to the request's parameter list: */
						parameters.push_back(parameter);
						parameter.name.clear();
						parameter.value.clear();
						
						/* Check if the request is now fully read: */
						if(contentLength==0)
							{
							/* Process the request: */
							if(processRequest(true))
								return;
							}
						else
							{
							/* Start reading another parameter name: */
							state=ReadingParameterName;
							}
						}
					else if(contentLength==0)
						{
						/* Add a parameter to the request's parameter list and process the request: */
						parameters.push_back(parameter);
						parameter.name.clear();
						parameter.value.clear();
						if(processRequest(true))
							return;
						}
					
					break;
					}
				
				case SkippingRequestContents:
					{
					/* Skip content inside the pipe's read buffer: */
					void* buffer;
					contentLength-=pipe->readInBuffer(buffer,contentLength);
					
					/* Process the request if all content has been skipped: */
					if(contentLength==0&&processRequest(false))
						return;
					}
				}
			}
		}
	catch(const std::runtime_error& err)
		{
		/* Print an error message, then close this connection: */
		Misc::sourcedConsoleWarning(__PRETTY_FUNCTION__,"Closing HTTP connection due to exception %s",err.what());
		close();
		}
	}

HttpServer::Connection::Connection(HttpServer& sServer)
	:server(sServer),
	 pipe(server.listenSocket->accept()),
	 pipeWatcher(server.runLoop.createIOWatcher(pipe->getFd(),Threads::RunLoop::IOWatcher::Read,true,*Threads::createFunctionCall(this,&HttpServer::Connection::pipeCallback))),
	 state(Start),requestHeader(0),contentLength(0)
	{
	// DEBUGGING
	std::cout<<"Comm::HttpServer: Opening HTTP connection"<<std::endl;
	}

HttpServer::Connection::~Connection(void)
	{
	/* Delete a potential lingering HTTP request header: */
	delete requestHeader;
	}

/***************************
Methods of class HttpServer:
***************************/

void HttpServer::listenSocketCallback(Threads::RunLoop::IOWatcher::Event& event)
	{
	try
		{
		/* Create a new HTTP connection and add it to the active connection set: */
		connections.add(new Connection(*this));
		}
	catch(const std::runtime_error& err)
		{
		/* Print an error message and carry on: */
		Misc::sourcedConsoleWarning(__PRETTY_FUNCTION__,"Cannot accept new HTTP connection due to exception %s",err.what());
		}
	}

HttpServer::HttpServer(Threads::RunLoop& sRunLoop,int listenPort)
	:runLoop(sRunLoop),
	 listenSocket(new ListeningTCPSocket(listenPort,5)),
	 listenSocketWatcher(runLoop.createIOWatcher(listenSocket->getFd(),Threads::RunLoop::IOWatcher::Read,true,*Threads::createFunctionCall(this,&HttpServer::listenSocketCallback)))
	{
	/* Set the listening socket to non-blocking mode: */
	listenSocket->setBlocking(false);
	}

HttpServer::~HttpServer(void)
	{
	}

int HttpServer::getPort(void) const
	{
	return static_cast<Comm::ListeningTCPSocket*>(listenSocket.getPointer())->getPortId();
	}

void HttpServer::setPostRequestHandler(PostRequestHandler& newPostRequestHandler)
	{
	/* Replace the current HTTP POST request handler: */
	postRequestHandler=&newPostRequestHandler;
	}

}
