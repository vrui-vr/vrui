/***********************************************************************
StandardDirectory - Pair of classes to access cluster-transparent
standard filesystem directories.
Copyright (c) 2011-2024 Oliver Kreylos

This file is part of the Cluster Abstraction Library (Cluster).

The Cluster Abstraction Library is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Cluster Abstraction Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Cluster Abstraction Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <Cluster/StandardDirectory.h>

#include <errno.h>
#include <Misc/StdError.h>
#include <Misc/StandardMarshallers.h>
#include <Misc/GetCurrentDirectory.h>
#include <Misc/FileTests.h>
#include <IO/StandardDirectory.h>
#include <Cluster/Opener.h>

namespace Cluster {

namespace {

/*******************************
String constants for exceptions:
*******************************/

static const char constructorFuncName[]="Cluster::StandardDirectory::StandardDirectory";
static const char readNextEntryFuncName[]="Cluster::StandardDirectory::readNextEntry";
static const char readNextEntryErrorString[]="Cannot read next directory entry";

}

/**********************************
Methods of class StandardDirectory:
**********************************/

StandardDirectory::StandardDirectory(Multiplexer* sMultiplexer,const char* sPathName)
	:pipe(sMultiplexer),
	 entryType(Misc::PATHTYPE_DOES_NOT_EXIST)
	{
	/* Prepend the current directory path to the path name if the given path name is relative: */
	if(sPathName[0]!='/')
		{
		pathName=Misc::getCurrentDirectory();
		pathName.push_back('/');
		pathName.append(sPathName);
		}
	else
		pathName=sPathName;
	
	/* Normalize the path name: */
	normalizePath(pathName,1);
	}

StandardDirectory::StandardDirectory(Multiplexer* sMultiplexer,const char* sPathNameBegin,const char* sPathNameEnd)
	:pipe(sMultiplexer),
	 entryType(Misc::PATHTYPE_DOES_NOT_EXIST)
	{
	/* Prepend the current directory path to the path name if the given path name is relative: */
	if(sPathNameBegin==sPathNameEnd||*sPathNameBegin!='/')
		{
		pathName=Misc::getCurrentDirectory();
		pathName.push_back('/');
		pathName.append(sPathNameBegin,sPathNameEnd);
		}
	else
		pathName=std::string(sPathNameBegin,sPathNameEnd);
	
	/* Normalize the path name: */
	normalizePath(pathName,1);
	}

StandardDirectory::StandardDirectory(Multiplexer* sMultiplexer,const char* sPathName,int)
	:pipe(sMultiplexer),
	 pathName(sPathName),
	 entryType(Misc::PATHTYPE_DOES_NOT_EXIST)
	{
	}

std::string StandardDirectory::getName(void) const
	{
	return std::string(getLastComponent(pathName,1),pathName.end());
	}

std::string StandardDirectory::getPath(void) const
	{
	return pathName;
	}

std::string StandardDirectory::getPath(const char* relativePath) const
	{
	/* Check if the given path is absolute: */
	if(relativePath[0]=='/')
		{
		/* Normalize the given absolute path name: */
		std::string result=relativePath;
		normalizePath(result,1);
		
		return result;
		}
	else
		{
		/* Assemble and normalize the absolute path name: */
		std::string result=pathName;
		if(result.length()>1)
			result.push_back('/');
		result.append(relativePath);
		normalizePath(result,1);
		
		return result;
		}
	}

bool StandardDirectory::hasParent(void) const
	{
	return pathName.length()>1;
	}

IO::DirectoryPtr StandardDirectory::getParent(void) const
	{
	/* Check for the special case of the root directory: */
	if(pathName.length()==1)
		return 0;
	else
		{
		/* Find the last slash in the absolute path name: */
		std::string::const_iterator slashIt;
		for(slashIt=pathName.end()-1;*slashIt!='/';--slashIt)
			;
		
		/* Find the last component in the absolute path name: */
		std::string::const_iterator lastCompIt=getLastComponent(pathName,1);
		
		/* Strip off the last slash unless it's the prefix: */
		if(lastCompIt-pathName.begin()>1)
			--lastCompIt;
		
		/* Open and return the directory corresponding to the path name prefix before the last slash: */
		if(pipe.getMultiplexer()->isMaster())
			return new StandardDirectoryMaster(pipe.getMultiplexer(),std::string(pathName.begin(),lastCompIt).c_str(),0);
		else
			return new StandardDirectorySlave(pipe.getMultiplexer(),std::string(pathName.begin(),lastCompIt).c_str(),0);
		}
	}

Misc::PathType StandardDirectory::getEntryType(void) const
	{
	return entryType;
	}

IO::FilePtr StandardDirectory::openFile(const char* fileName,IO::File::AccessMode accessMode) const
	{
	/* Check if the file name is absolute: */
	if(fileName[0]=='/')
		{
		/* Open and return the file using the absolute path: */
		return Opener::openFile(pipe.getMultiplexer(),fileName,accessMode);
		}
	else
		{
		/* Assemble the absolute path name of the given file based on this directory's path name: */
		std::string filePath=pathName;
		if(filePath.length()>1)
			filePath.push_back('/');
		filePath.append(fileName);
		
		/* Open and return the file: */
		return Opener::openFile(pipe.getMultiplexer(),filePath.c_str(),accessMode);
		}
	}

IO::DirectoryPtr StandardDirectory::openDirectory(const char* directoryName) const
	{
	/* Check if the directory name is absolute: */
	if(directoryName[0]=='/')
		{
		/* Open and return the directory using the absolute path: */
		if(pipe.getMultiplexer()->isMaster())
			return new StandardDirectoryMaster(pipe.getMultiplexer(),directoryName);
		else
			return new StandardDirectorySlave(pipe.getMultiplexer(),directoryName);
		}
	else
		{
		/* Assemble the absolute path name of the given directory based on this directory's path name: */
		std::string directoryPath=pathName;
		if(directoryPath.length()>1)
			directoryPath.push_back('/');
		directoryPath.append(directoryName);
		
		/* Open and return the directory: */
		if(pipe.getMultiplexer()->isMaster())
			return new StandardDirectoryMaster(pipe.getMultiplexer(),directoryPath.c_str());
		else
			return new StandardDirectorySlave(pipe.getMultiplexer(),directoryPath.c_str());
		}
	}

/****************************************
Methods of class StandardDirectoryMaster:
****************************************/

StandardDirectoryMaster::StandardDirectoryMaster(Multiplexer* sMultiplexer,const char* sPathName)
	:StandardDirectory(sMultiplexer,sPathName),
	 directory(opendir(pathName.c_str())),
	 entry(0)
	{
	/* Check for failure: */
	int errorCode=errno;
	pipe.write<int>(directory!=0?0:errorCode);
	pipe.flush();
	if(directory==0)
		throw OpenError(constructorFuncName,errorCode,pathName.c_str());
	}

StandardDirectoryMaster::StandardDirectoryMaster(Multiplexer* sMultiplexer,const char* sPathNameBegin,const char* sPathNameEnd)
	:StandardDirectory(sMultiplexer,sPathNameBegin,sPathNameEnd),
	 directory(opendir(pathName.c_str())),
	 entry(0)
	{
	/* Check for failure: */
	int errorCode=errno;
	pipe.write<int>(directory!=0?0:errorCode);
	pipe.flush();
	if(directory==0)
		throw OpenError(constructorFuncName,errorCode,pathName.c_str());
	}

StandardDirectoryMaster::StandardDirectoryMaster(Multiplexer* sMultiplexer,const char* sPathName,int)
	:StandardDirectory(sMultiplexer,sPathName,0),
	 directory(opendir(pathName.c_str())),
	 entry(0)
	{
	/* Check for failure: */
	int errorCode=errno;
	pipe.write<int>(directory!=0?0:errorCode);
	pipe.flush();
	if(directory==0)
		throw OpenError(constructorFuncName,errorCode,pathName.c_str());
	}

StandardDirectoryMaster::~StandardDirectoryMaster(void)
	{
	/* Close the directory: */
	if(directory!=0)
		closedir(directory);
	}

void StandardDirectoryMaster::rewind(void)
	{
	rewinddir(directory);
	entry=0;
	entryType=Misc::PATHTYPE_DOES_NOT_EXIST;
	}

bool StandardDirectoryMaster::readNextEntry(void)
	{
	/* Read the next entry: */
	errno=0;
	entry=readdir(directory);
	
	if(entry!=0)
		{
		/* Determine the entry's path type: */
		#ifdef _DIRENT_HAVE_D_TYPE
		
		/* Convert the OS-level entry type to a Misc::PathType: */
		switch(entry->d_type)
			{
			case DT_REG:
				entryType=Misc::PATHTYPE_FILE;
				break;
				
			case DT_DIR:
				entryType=Misc::PATHTYPE_DIRECTORY;
				break;
				
			case DT_CHR:
				entryType=Misc::PATHTYPE_CHARACTER_DEVICE;
				break;
				
			case DT_BLK:
				entryType=Misc::PATHTYPE_BLOCK_DEVICE;
				break;
				
			case DT_FIFO:
				entryType=Misc::PATHTYPE_NAMED_PIPE;
				break;
				
			case DT_LNK:
				entryType=Misc::PATHTYPE_SYMBOLIC_LINK;
				break;
				
			case DT_SOCK:
				entryType=Misc::PATHTYPE_SOCKET;
				break;
				
			default:
				entryType=Misc::PATHTYPE_UNKNOWN;
			}
		
		#else
		
		/* Assemble the absolute path name of the directory entry: */
		std::string entryPath=pathName;
		if(entryPath.length()>1)
			entryPath.push_back('/');
		entryPath.append(entry->d_name);
		
		/* Query the path's file type: */
		entryType=Misc::getPathType(entryPath.c_str());
		
		#endif
		
		/* Send the entry path type to the slaves: */
		pipe.write(int(entryType));
		
		/* Send the entry name to the slaves: */
		Misc::Marshaller<std::string>::write(std::string(entry->d_name),pipe);
		
		pipe.flush();
		}
	else if(errno==0)
		{
		/* Send the end-of-directory code to the slaves: */
		pipe.write(int(-1));
		
		pipe.flush();
		}
	else
		{
		/* Send an error code to the slaves and throw an exception: */
		int errorCode=errno;
		pipe.write(int(-2));
		pipe.write(errorCode);
		
		pipe.flush();
		
		throw Misc::makeLibcErr(readNextEntryFuncName,errorCode,readNextEntryErrorString);
		}
	
	return entry!=0;
	}

const char* StandardDirectoryMaster::getEntryName(void) const
	{
	return entry->d_name;
	}

Misc::PathType StandardDirectoryMaster::getPathType(const char* relativePath) const
	{
	Misc::PathType result;
	
	/* Check if the given path is absolute: */
	if(relativePath[0]=='/')
		{
		/* Use the given absolute path directly: */
		result=Misc::getPathType(relativePath);
		}
	else
		{
		/* Assemble the absolute path name: */
		std::string absolutePath=pathName;
		if(absolutePath.length()>1)
			absolutePath.push_back('/');
		absolutePath.append(relativePath);
		
		/* Use the assembled absolute path: */
		result=Misc::getPathType(absolutePath.c_str());
		}
	
	/* Send the path type to the slaves: */
	pipe.write<int>(int(result));
	pipe.flush();
	
	return result;
	}

/***************************************
Methods of class StandardDirectorySlave:
***************************************/

StandardDirectorySlave::StandardDirectorySlave(Multiplexer* sMultiplexer,const char* sPathName)
	:StandardDirectory(sMultiplexer,sPathName)
	{
	/* Check for failure: */
	int errorCode=pipe.read<int>();
	if(errorCode!=0)
		throw OpenError(constructorFuncName,errorCode,pathName.c_str());
	}

StandardDirectorySlave::StandardDirectorySlave(Multiplexer* sMultiplexer,const char* sPathNameBegin,const char* sPathNameEnd)
	:StandardDirectory(sMultiplexer,sPathNameBegin,sPathNameEnd)
	{
	/* Check for failure: */
	int errorCode=pipe.read<int>();
	if(errorCode!=0)
		throw OpenError(constructorFuncName,errorCode,pathName.c_str());
	}

StandardDirectorySlave::StandardDirectorySlave(Multiplexer* sMultiplexer,const char* sPathName,int)
	:StandardDirectory(sMultiplexer,sPathName,0)
	{
	/* Check for failure: */
	int errorCode=pipe.read<int>();
	if(errorCode!=0)
		throw OpenError(constructorFuncName,errorCode,pathName.c_str());
	}

StandardDirectorySlave::~StandardDirectorySlave(void)
	{
	}

void StandardDirectorySlave::rewind(void)
	{
	entryName="";
	entryType=Misc::PATHTYPE_DOES_NOT_EXIST;
	}

bool StandardDirectorySlave::readNextEntry(void)
	{
	/* Read the combined entry type / status code: */
	int status=pipe.read<int>();
	
	if(status>=0)
		{
		/* Set the entry type: */
		entryType=Misc::PathType(status);

		/* Read the entry name: */
		Misc::Marshaller<std::string>::read(pipe,entryName);
		}
	else if(status==-2)
		{
		/* Read the error code and throw an exception: */
		int errorCode=pipe.read<int>();
		throw Misc::makeLibcErr(readNextEntryFuncName,errorCode,readNextEntryErrorString);
		}
	
	return status>=0;
	}

const char* StandardDirectorySlave::getEntryName(void) const
	{
	return entryName.c_str();
	}

Misc::PathType StandardDirectorySlave::getPathType(const char* relativePath) const
	{
	/* Read the path type from the master: */
	return Misc::PathType(pipe.read<int>());
	}

}
