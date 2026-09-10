/***********************************************************************
Ogg - Wrapper classes for structures in the Ogg multimedia container
format library.
Copyright (c) 2010-2026 Oliver Kreylos

This file is part of the Basic Sound Library (Sound).

The Basic Sound Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The Basic Sound Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Basic Sound Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef SOUND_OGG_INCLUDED
#define SOUND_OGG_INCLUDED

#include <stddef.h>
#include <ogg/ogg.h>

namespace Sound {

namespace Ogg {

class Packet:public ogg_packet // Class representing an Ogg packet
	{
	/* Constructors and destructors: */
	public:
	Packet(void) // Creates a packet to be retrieved from an Ogg stream during decoding
		{
		/* Let's not initialize anything here... */
		}
	Packet(const void* sPacket,long sBytes,long sB_o_s,long sE_o_s,ogg_int64_t sGranulepos,ogg_int64_t sPacketno) // Creates a packet to be submitted to an Ogg stream during encoding
		{
		/* Initialize the ogg_packet structure: */
		packet=static_cast<unsigned char*>(const_cast<void*>(sPacket)); // The packet data will not be modified during encoding
		bytes=sBytes;
		b_o_s=sB_o_s;
		e_o_s=sE_o_s;
		granulepos=sGranulepos;
		packetno=sPacketno;
		}
	};

class Page:public ogg_page // Class representing an Ogg page
	{
	/* Constructors and destructors: */
	public:
	Page(void) // Creates a page to be retrieved from an Ogg stream during encoding
		{
		/* Let's not initialize anything here... */
		}
	Page(const void* sHeader,long sHeaderLen,const void* sBody,long sBodyLen) // Creates a page to be submitted to an Ogg stream during decoding
		{
		/* Initialize the ogg_page structure: */
		header=static_cast<unsigned char*>(const_cast<void*>(sHeader)); // The header data will not be modified during decoding
		header_len=sHeaderLen;
		body=static_cast<unsigned char*>(const_cast<void*>(sBody)); // The body data will not be modified during decoding
		body_len=sBodyLen;
		}
	
	/* Methods: */
	int getPageVersion(void) const // Returns the API version number of the page
		{
		return ogg_page_version(const_cast<Page*>(this));
		}
	bool isContinued(void) const // Returns true if the page continues an Ogg packet from a previous page
		{
		return ogg_page_continued(const_cast<Page*>(this));
		}
	int getNumPackets(void) const // Returns the number of Ogg packets begun in the page
		{
		return ogg_page_packets(const_cast<Page*>(this));
		}
	bool isBos(void) const // Returns true if the page starts a new Ogg stream
		{
		return ogg_page_bos(const_cast<Page*>(this));
		}
	bool isEos(void) const // Returns true if the page completes an Ogg stream
		{
		return ogg_page_eos(const_cast<Page*>(this));
		}
	ogg_int64_t getGranulePos(void) const // Returns the exact granular position of the packet data at the end of the page
		{
		return ogg_page_granulepos(const_cast<Page*>(this));
		}
	int getSerialNumber(void) const // Returns the serial number of the logical stream to which this page belongs
		{
		return ogg_page_serialno(const_cast<Page*>(this));
		}
	long getPageNumber(void) const // Returns the sequential page number of the page
		{
		return ogg_page_pageno(const_cast<Page*>(this));
		}
	void calcChecksum(void) // Calculates the page's checksum and writes it into the page header
		{
		ogg_page_checksum_set(this);
		}
	
	/* Output methods: */
	size_t getWriteSize(void) const // Returns the size of the page when written to a data sink in bytes
		{
		return header_len+body_len;
		}
	template <class SinkParam>
	void write(SinkParam& sink) const // Writes the page to the given data sink
		{
		/* Write the page header: */
		sink.template write(header,header_len);
		
		/* Write the page body: */
		sink.template write(body,body_len);
		}
	
	};

class Stream:public ogg_stream_state // Class representing an Ogg data stream
	{
	/* Constructors and destructors: */
	public:
	Stream(int serialNumber); // Creates a stream object with the given stream serial number
	~Stream(void);
	
	/* Methods: */
	bool hadError(void) const // Returns true if the stream had an internal error and is in a failure state. It is safe to ignore errors returned by encoding/decoding functions and check for the error state at some later point
		{
		/* Check the stream state: */
		return ogg_stream_check(const_cast<Stream*>(this))!=0; // The Ogg API takes a non-const stream state pointer, but it does not modify the stream
		}
	void reset(void); // Resets the stream to its initial state
	void reset(int newSerialNumber); // Ditto, additionaly sets the stream's serial number to the given value
	
	/* Encoding methods: */
	bool packetIn(const ogg_packet& packet) // Submits an Ogg packet to the Ogg stream during encoding; returns false if there was an internal error
		{
		/* Append the Ogg packet to the Ogg stream: */
		return ogg_stream_packetin(this,const_cast<ogg_packet*>(&packet))==0;
		}
	bool pageOut(ogg_page& page) // Asks the Ogg stream if it has filled an Ogg page; returns true if page object was filled. Call this repeatedly after packetIn until it returns false
		{
		/* Retrieve a page from the Ogg stream: */
		return ogg_stream_pageout(this,&page)!=0;
		}
	bool pageOut(ogg_page& page,int minPageSize) // Ditto, but instructs the Ogg stream to emit a page if there are at least minPageSize accumulated bytes
		{
		/* Retrieve a page from the Ogg stream: */
		return ogg_stream_pageout_fill(this,&page,minPageSize)!=0;
		}
	bool flush(ogg_page& page) // Forces the Ogg stream to write any remaining data into an Ogg page; returs true if page object was filled. Call this repeatedly until it returns false to flush an Ogg stream
		{
		/* Forcefully retrieve a page from the Ogg stream: */
		return ogg_stream_flush(this,&page)!=0;
		}
	bool flush(ogg_page& page,int minPageSize) // Ditto, but instructs the Ogg stream to emit pages of approximately the given size in bytes
		{
		/* Forcefully retrieve a page from the Ogg stream: */
		return ogg_stream_flush_fill(this,&page,minPageSize)!=0;
		}
	
	/* Decoding methods: */
	bool pageIn(const ogg_page& page) // Submits a page of data to the Ogg stream during decoding; returns false if there was an internal error
		{
		/* Append the Ogg page to the Ogg stream: */
		return ogg_stream_pagein(this,const_cast<ogg_page*>(&page))==0; // The Ogg API takes a non-const page pointer, but it does not modify the page
		}
	bool packetOut(ogg_packet& packet) // Retrieves an Ogg packet from the Ogg stream; returns true if packet structure was filled. Call this repeatedly after pageIn until it returns false
		{
		/* Retrieve a packet from the ogg_stream_state: */
		return ogg_stream_packetout(this,&packet)==1; // Ignore the case of out-of-sync packets; the next call to packetOut should re-sync
		}
	bool packetPeek(ogg_packet& packet) // Retrieves an Ogg packet from the Ogg stream but does not remove it from the stream; returns true if packet structure was filled
		{
		/* Retrieve a packet from the ogg_stream_state: */
		return ogg_stream_packetpeek(this,&packet)==1; // Ignore the case of out-of-sync packets; the next call to packetOut should re-sync
		}
	bool isEos(void) const // Returns true if the end of the stream has been reached
		{
		return ogg_stream_eos(const_cast<Stream*>(this))!=0;
		}
	};

class Sync:public ogg_sync_state // Class representing an Ogg synchronization state, to read pages from an input file or stream
	{
	/* Constructors and destructors: */
	public:
	Sync(void); // Creates a synchronization object
	~Sync(void); // Destroys the synchronization object
	
	/* Methods: */
	bool hadError(void) const // Returns true if the synchronization object had an internal error and is in a failure state. It is safe to ignore errors returned by decoding functions and check for the error state at some later point
		{
		return ogg_sync_check(const_cast<Sync*>(this))!=0; // The Ogg API takes a non-const sync state pointer, but it does not modify the synchronization object
		}
	void reset(void); // Resets the synchronization object object to its initial state. It is a good idea to call this before seeking within the input file/stream
	
	/* Stream reading methods: */
	void* getBuffer(long bufferSize); // Returns a pointer to a buffer of at least the given size in bytes into which to read data from the input file/stream; throws an exception if there is an internal error
	void setDataSize(long dataSize); // Sets the amount of data in bytes that was just written into a buffer returned by getBuffer by the caller; throws exception if there is an internal error
	
	/* Page retrieval methods: */
	int seekPage(ogg_page& page) // Seeks for the beginning of the next page in the input file/stream; returns a negative result to indicate that -result bytes were skipped, a positive result to indicate that a page of size result was found at the current position, or 0 to indicate that more data needs to be read from the input file/stream
		{
		return ogg_sync_pageseek(this,&page);
		}
	bool pageOut(ogg_page& page) // Retrieves an Ogg page from the synchronization object; returns true if page structure was filled. Must be called before getBuffer to clear the internal buffer
		{
		return ogg_sync_pageout(this,&page)==1; // Ignore the case of missing sync; subsequent call to getBuffer/setDataSize should fix it
		}
	};

}

}

#endif
