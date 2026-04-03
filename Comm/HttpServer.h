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

#ifndef COMM_HTTPSERVER_INCLUDED
#define COMM_HTTPSERVER_INCLUDED

#include <string>
#include <vector>
#include <Misc/Autopointer.h>
#include <Misc/SimpleObjectSet.h>
#include <Threads/RunLoop.h>
#include <Comm/Pipe.h>
#include <Comm/ListeningSocket.h>

/* Forward declarations: */
namespace Threads {
template <class ParameterParam>
class FunctionCall;
}
namespace IO {
class JsonEntity;
}
namespace Comm {
class HttpRequestHeader;
}

namespace Comm {

class HttpServer
	{
	/* Embedded classes: */
	public:
	struct RequestParameter // Structure representing a request parameter, i.e., a name=value pair in a POST request's body
		{
		/* Elements: */
		public:
		std::string name,value; // The parameter's name and value
		};
	
	typedef std::vector<RequestParameter> RequestParameterList; // Type for lists of request parameters read from a POST request's body
	
	struct PostRequest // Structure representing an HTTP POST request
		{
		/* Elements: */
		public:
		const std::string& requestUri; // The request URI string
		const RequestParameterList& parameters; // The list of request parameters
		bool resultValid; // Flag if the POST request resulted in a valid result
		std::string resultContentType; // The request result's content type
		IO::File& resultFile; // A file into which to write the results of the request
		
		/* Constructors and destructors: */
		PostRequest(const std::string& sRequestUri,const RequestParameterList& sParameters,IO::File& sResultFile) // Elementwise constructor
			:requestUri(sRequestUri),parameters(sParameters),resultValid(false),resultFile(sResultFile)
			{
			}
		
		/* Methods: */
		void setJsonResult(const IO::JsonEntity& json); // Sends the given JSON entity as the request's result
		};
	
	typedef Threads::FunctionCall<PostRequest&> PostRequestHandler; // Type for functions handling complete HTTP POST requests
	
	private:
	class Connection // Class representing an active HTTP connection
		{
		friend class HttpServer;
		
		/* Embedded classes: */
		private:
		enum State // Enumerated type for states of an active HTTP connection
			{
			Start,
			ReadingHttpRequestHeader,
			ReadingParameterName,ReadingParameterValue,
			SkippingRequestContents,
			Closed
			};
		
		/* Elements: */
		HttpServer& server; // Reference to the HTTP server owning this connection
		PipePtr pipe; // Pipe connected to the client on the other end of this connection
		Threads::RunLoop::IOWatcherOwner pipeWatcher; // I/O watcher for the pipe
		bool eventSink; // Flag if this connection can be used to send server-sent events to a client
		State state; // Current state of this connection
		HttpRequestHeader* requestHeader; // Pointer to an HTTP request header currently being read from the connection
		RequestParameter parameter; // The currently parsed parameter in an HTTP POST request's body
		RequestParameterList parameters; // The list of parameters parsed from the current HTTP POST request's body
		size_t contentLength; // Remaining length of the current HTTP request's content
		
		/* Private methods: */
		void close(void); // Closes the connection immediately
		bool processRequest(bool valid); // Processes a complete valid or invalid HTTP request; returns true if the I/O callback needs to bail out immediately
		bool ignoreRequest(void); // Ignores the current HTTP request; returns true if the I/O callback needs to bail out immediately
		void pipeCallback(Threads::RunLoop::IOWatcher::Event& event); // Callback called when data can be read from and/or written to the pipe
		
		/* Constructors and destructors: */
		public:
		Connection(HttpServer& sServer); // Creates a new connection by accepting a pending connection from the given server's listening socket
		~Connection(void); // Destroys this connection
		};
	
	friend class Connection;
	
	/* Elements: */
	Threads::RunLoop& runLoop; // Reference to a run loop handling I/O on the listening socket and all active connections
	ListeningSocketPtr listenSocket; // Socket listening for incoming HTTP connections
	Threads::RunLoop::IOWatcherOwner listenSocketWatcher; // I/O watcher for the HTTP listening socket
	Misc::SimpleObjectSet<Connection> connections; // List of currently active HTTP connections
	Misc::Autopointer<PostRequestHandler> postRequestHandler; // Pointer to a handler for complete HTTP POST requests
	
	/* Private methods: */
	void listenSocketCallback(Threads::RunLoop::IOWatcher::Event& event); // Callback called when a new connection is available on the HTTP listening socket
	
	/* Constructors and destructors: */
	public:
	HttpServer(Threads::RunLoop& sRunLoop,int listenPort); // Creates an HTTP server for the given run loop listening on the given port
	~HttpServer(void); // Destroys the HTTP server
	
	/* Methods: */
	int getPort(void) const; // Returns the TCP port on which the HTTP server is listening for incoming connections
	void setPostRequestHandler(PostRequestHandler& newPostRequestHandler); // Sets the function to be called to handle complete HTTP POST requests
	void sendEvent(const char* eventName,const IO::JsonEntity& eventData); // Sends an event of the given name with the given JSON payload to all clients that registered to receive events
	};

}

#endif
