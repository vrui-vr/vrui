/***********************************************************************
ConfigurationFile - Class to handle permanent storage of configuration
data in human-readable text files.
Copyright (c) 2002-2024 Oliver Kreylos

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

#include <Misc/ConfigurationFile.h>

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include <Misc/StdError.h>
#include <Misc/File.h>
#include <Misc/StandardValueCoders.h>

// DEBUGGING
#include <iostream>

namespace Misc {

/****************************************************************
Methods of class ConfigurationFileBase::MalFormedConfigFileError:
****************************************************************/

ConfigurationFileBase::MalformedConfigFileError::MalformedConfigFileError(const char* source,const char* error,int lineNumber,const char* configFileName)
	:std::runtime_error(makeStdErrMsg(source,"%s in line %d of configuration file %s",error,lineNumber,configFileName))
	{
	}

/************************************************************
Methods of class ConfigurationFileBase::SectionNotFoundError:
************************************************************/

ConfigurationFileBase::SectionNotFoundError::SectionNotFoundError(const char* source,const char* sectionPath,const char* subsectionName)
	:std::runtime_error(makeStdErrMsg(source,"Cannot find subsection %s in section %s",subsectionName,sectionPath).c_str())
	{
	}

/********************************************************
Methods of class ConfigurationFileBase::TagNotFoundError:
********************************************************/

ConfigurationFileBase::TagNotFoundError::TagNotFoundError(const char* source,const char* sectionPath,const char* tagName)
	:std::runtime_error(makeStdErrMsg(source,"Cannot find tag %s in section %s",tagName,sectionPath).c_str())
	{
	}

/***********************************************
Methods of class ConfigurationFileBase::Section:
***********************************************/

ConfigurationFileBase::Section::Section(ConfigurationFileBase::Section* sParent,const std::string& sName)
	:parent(sParent),name(sName),
	 sibling(0),
	 firstSubsection(0),lastSubsection(0),
	 edited(false)
	{
	}

ConfigurationFileBase::Section::~Section(void)
	{
	/* Delete all subsections: */
	while(firstSubsection!=0)
		{
		Section* next=firstSubsection->sibling;
		delete firstSubsection;
		firstSubsection=next;
		}
	}

void ConfigurationFileBase::Section::clear(void)
	{
	/* Remove all subsections: */
	while(firstSubsection!=0)
		{
		Section* succ=firstSubsection->sibling;
		delete firstSubsection;
		firstSubsection=succ;
		}
	lastSubsection=0;
	
	/* Remove all tag/value pairs: */
	values.clear();
	
	/* Mark the section as edited: */
	edited=true;
	}

ConfigurationFileBase::Section* ConfigurationFileBase::Section::addSubsection(const std::string& subsectionName)
	{
	/* Check if the subsection already exists: */
	Section* sPtr;
	for(sPtr=firstSubsection;sPtr!=0;sPtr=sPtr->sibling)
		if(sPtr->name==subsectionName)
			break;
	
	if(sPtr==0)
		{
		/* Add new subsection: */
		Section* newSubsection=new Section(this,subsectionName);
		if(lastSubsection!=0)
			lastSubsection->sibling=newSubsection;
		else
			firstSubsection=newSubsection;
		lastSubsection=newSubsection;
		
		/* Mark the section as edited: */
		edited=true;
		
		return newSubsection;
		}
	else
		return sPtr;
	}

void ConfigurationFileBase::Section::removeSubsection(const std::string& subsectionName)
	{
	/* Find a subsection of the given name: */
	Section* sPred=0;
	Section* sPtr;
	for(sPtr=firstSubsection;sPtr!=0&&sPtr->name!=subsectionName;sPred=sPtr,sPtr=sPtr->sibling)
		;
	if(sPtr!=0)
		{
		/* Remove the subsection: */
		if(sPred!=0)
			sPred->sibling=sPtr->sibling;
		else
			firstSubsection=sPtr->sibling;
		if(sPtr->sibling==0)
			lastSubsection=sPred;
		delete sPtr;
		
		/* Mark the section as edited: */
		edited=true;
		}
	}

void ConfigurationFileBase::Section::addTagValue(const std::string& newTag,const std::string& newValue)
	{
	/* Find the tag name in the section's tag list: */
	std::list<TagValue>::iterator tvIt;
	for(tvIt=values.begin();tvIt!=values.end()&&tvIt->tag!=newTag;++tvIt)
		;
	
	/* Set tag value: */
	if(tvIt==values.end())
		{
		/* Add a new tag/value pair: */
		values.push_back(TagValue(newTag,newValue));
		}
	else
		{
		/* Set new value for existing tag/value pair: */
		tvIt->value=newValue;
		}
	
	/* Mark the section as edited: */
	edited=true;
	}

void ConfigurationFileBase::Section::removeTag(const std::string& tag)
	{
	/* Find the tag name in the section's tag list: */
	std::list<TagValue>::iterator tvIt;
	for(tvIt=values.begin();tvIt!=values.end()&&tvIt->tag!=tag;++tvIt)
		;
	
	/* Check if the tag was found: */
	if(tvIt!=values.end())
		{
		/* Remove tag/value pair: */
		values.erase(tvIt);
		}
	
	/* Mark the section as edited: */
	edited=true;
	}

bool ConfigurationFileBase::Section::isEdited(void) const
	{
	/* Return true if this section has been edited: */
	if(edited)
		return true;
	
	/* Check if any subsections have been edited: */
	for(const Section* sPtr=firstSubsection;sPtr!=0;sPtr=sPtr->sibling)
		if(sPtr->isEdited())
			return true;
	
	/* Nothing has been edited: */
	return false;
	}

void ConfigurationFileBase::Section::clearEditFlag(void)
	{
	/* Clear this section's edit flag: */
	edited=false;
	
	/* Clear edit flags of all subsections: */
	for(Section* sPtr=firstSubsection;sPtr!=0;sPtr=sPtr->sibling)
		sPtr->clearEditFlag();
	}

void ConfigurationFileBase::Section::save(File& file,int sectionLevel)
	{
	/* Generate indentation: */
	char prefix[80];
	for(int i=0;i<sectionLevel;++i)
		prefix[i]='\t';
	prefix[sectionLevel]='\0';
	
	/* Write subsections followed by tag/value pairs: */
	bool didWriteSomething=false;
	
	/* Write all subsections: */
	for(Section* ssPtr=firstSubsection;ssPtr!=0;ssPtr=ssPtr->sibling)
		{
		/* Write separator line: */
		if(didWriteSomething)
			fprintf(file.getFilePtr(),"%s\n",prefix);
		
		/* Write section header: */
		fprintf(file.getFilePtr(),"%ssection %s\n",prefix,ssPtr->name.c_str());
		
		/* Write section contents: */
		ssPtr->save(file,sectionLevel+1);
		
		/* Write section footer: */
		fprintf(file.getFilePtr(),"%sendsection\n",prefix);
		
		didWriteSomething=true;
		}
	
	/* Write tag/value pairs: */
	for(std::list<TagValue>::const_iterator tvIt=values.begin();tvIt!=values.end();++tvIt)
		{
		/* Write separator line: */
		if(didWriteSomething)
			{
			fprintf(file.getFilePtr(),"%s\n",prefix);
			didWriteSomething=false;
			}
		
		/* Write tag/value pair: */
		fprintf(file.getFilePtr(),"%s%s %s\n",prefix,tvIt->tag.c_str(),tvIt->value.c_str());
		}
	
	/* Mark the section as saved: */
	edited=false;
	}

std::string ConfigurationFileBase::Section::getPath(void) const
	{
	if(parent==0)
		return "/";
	else
		{
		/* Accumulate path names while going up the section hierarchy: */
		std::string result="";
		for(const Section* sPtr=this;sPtr->parent!=0;sPtr=sPtr->parent)
			result=std::string("/")+sPtr->name+result;
		return result;
		}
	}

const ConfigurationFileBase::Section* ConfigurationFileBase::Section::getSection(const char* relativePath,const char** pathSuffix) const
	{
	const char* pathSuffixPtr=relativePath;
	const Section* sPtr=this;
	
	/* If the first character in the path is a slash, start searching from the root section: */
	if(pathSuffixPtr[0]=='/')
		{
		while(sPtr->parent!=0)
			sPtr=sPtr->parent;
		++pathSuffixPtr;
		}
	
	/* Process section prefixes of relative path: */
	while(true)
		{
		/* Find next slash in suffix: */
		const char* nextSlashPtr;
		for(nextSlashPtr=pathSuffixPtr;*nextSlashPtr!='\0'&&*nextSlashPtr!='/';++nextSlashPtr)
			;
		if(*nextSlashPtr=='\0'&&pathSuffix!=0)
			{
			/* The slash-free suffix is a tag name; ignore and return it: */
			*pathSuffix=pathSuffixPtr;
			break;
			}
		
		/* Navigate section hierarchy: */
		if(nextSlashPtr-pathSuffixPtr==0)
			{
			/* Ignore double slashes */
			}
		else if(nextSlashPtr-pathSuffixPtr==1&&pathSuffixPtr[0]=='.')
			{
			/* Ignore self-reference */
			}
		else if(nextSlashPtr-pathSuffixPtr==2&&pathSuffixPtr[0]=='.'&&pathSuffixPtr[1]=='.')
			{
			/* Go up in the section hierarchy, if possible: */
			if(sPtr->parent!=0)
				sPtr=sPtr->parent;
			}
		else
			{
			/* Find subsection name in current section: */
			std::string subsectionName(pathSuffixPtr,nextSlashPtr-pathSuffixPtr);
			Section* ssPtr;
			for(ssPtr=sPtr->firstSubsection;ssPtr!=0&&ssPtr->name!=subsectionName;ssPtr=ssPtr->sibling)
				;
			
			/* Go down in the section hierarchy: */
			if(ssPtr==0)
				{
				/* Can't add new section; must throw exception: */
				throw SectionNotFoundError(__PRETTY_FUNCTION__,sPtr->getPath().c_str(),subsectionName.c_str());
				}
			else
				sPtr=ssPtr;
			}
		
		if(*nextSlashPtr=='\0')
			break;
		pathSuffixPtr=nextSlashPtr+1;
		}
	
	/* At this point, sPtr points to the correct section, and pathSuffixPtr is empty or slash-free: */
	return sPtr;
	}

ConfigurationFileBase::Section* ConfigurationFileBase::Section::getSection(const char* relativePath,const char** pathSuffix)
	{
	const char* pathSuffixPtr=relativePath;
	Section* sPtr=this;
	
	/* If the first character in the path is a slash, start searching from the root section: */
	if(pathSuffixPtr[0]=='/')
		{
		while(sPtr->parent!=0)
			sPtr=sPtr->parent;
		++pathSuffixPtr;
		}
	
	/* Process section prefixes of relative path: */
	while(true)
		{
		/* Find next slash in suffix: */
		const char* nextSlashPtr;
		for(nextSlashPtr=pathSuffixPtr;*nextSlashPtr!='\0'&&*nextSlashPtr!='/';++nextSlashPtr)
			;
		if(*nextSlashPtr=='\0'&&pathSuffix!=0)
			{
			/* The slash-free suffix is a tag name; ignore and return it: */
			*pathSuffix=pathSuffixPtr;
			break;
			}
		
		/* Navigate section hierarchy: */
		if(nextSlashPtr-pathSuffixPtr==0)
			{
			/* Ignore double slashes */
			}
		else if(nextSlashPtr-pathSuffixPtr==1&&pathSuffixPtr[0]=='.')
			{
			/* Ignore self-reference */
			}
		else if(nextSlashPtr-pathSuffixPtr==2&&pathSuffixPtr[0]=='.'&&pathSuffixPtr[1]=='.')
			{
			/* Go up in the section hierarchy, if possible: */
			if(sPtr->parent!=0)
				sPtr=sPtr->parent;
			}
		else
			{
			/* Go to subsection of given name (create if not already there): */
			sPtr=sPtr->addSubsection(std::string(pathSuffixPtr,nextSlashPtr-pathSuffixPtr));
			}
		
		if(*nextSlashPtr=='\0')
			break;
		pathSuffixPtr=nextSlashPtr+1;
		}
	
	/* At this point, sPtr points to the correct section, and pathSuffixPtr is empty or slash-free: */
	return sPtr;
	}

bool ConfigurationFileBase::Section::hasTag(const char* relativeTagPath) const
	{
	/* Go to the section containing the given tag: */
	const char* tagName=0;
	const Section* sPtr=getSection(relativeTagPath,&tagName);
	
	/* Find the tag name in the section's tag list: */
	std::list<TagValue>::const_iterator tvIt;
	for(tvIt=sPtr->values.begin();tvIt!=sPtr->values.end()&&tvIt->tag!=tagName;++tvIt)
		;
	
	return tvIt!=sPtr->values.end();
	}

const std::string* ConfigurationFileBase::Section::findTagValue(const char* relativeTagPath) const
	{
	/* Go to the section containing the given tag: */
	const char* tagName=0;
	const Section* sPtr=getSection(relativeTagPath,&tagName);
	
	/* Find the tag name in the section's tag list: */
	std::list<TagValue>::const_iterator tvIt;
	for(tvIt=sPtr->values.begin();tvIt!=sPtr->values.end()&&tvIt->tag!=tagName;++tvIt)
		;
	
	/* Return tag value or null pointer: */
	return tvIt!=sPtr->values.end()?&(tvIt->value):0;
	}

const std::string& ConfigurationFileBase::Section::retrieveTagValue(const char* relativeTagPath) const
	{
	/* Go to the section containing the given tag: */
	const char* tagName=0;
	const Section* sPtr=getSection(relativeTagPath,&tagName);
	
	/* Find the tag name in the section's tag list: */
	std::list<TagValue>::const_iterator tvIt;
	for(tvIt=sPtr->values.begin();tvIt!=sPtr->values.end()&&tvIt->tag!=tagName;++tvIt)
		;
	
	/* Return tag value: */
	if(tvIt==sPtr->values.end())
		throw TagNotFoundError(__PRETTY_FUNCTION__,sPtr->getPath().c_str(),tagName);
	return tvIt->value;
	}

std::string ConfigurationFileBase::Section::retrieveTagValue(const char* relativeTagPath,const std::string& defaultValue) const
	{
	const char* tagName=0;
	const Section* sPtr;
	try
		{
		/* Go to the section containing the given tag: */
		sPtr=getSection(relativeTagPath,&tagName);
		}
	catch(const SectionNotFoundError& error)
		{
		/* If the section does not exist, bail out and return the default value: */
		return defaultValue;
		}
	
	/* Find the tag name in the section's tag list: */
	std::list<TagValue>::const_iterator tvIt;
	for(tvIt=sPtr->values.begin();tvIt!=sPtr->values.end()&&tvIt->tag!=tagName;++tvIt)
		;

	/* Return tag value: */
	if(tvIt==sPtr->values.end())
		throw TagNotFoundError(__PRETTY_FUNCTION__,sPtr->getPath().c_str(),tagName);
	return tvIt->value;
	}

const std::string& ConfigurationFileBase::Section::retrieveTagValue(const char* relativeTagPath,const std::string& defaultValue)
	{
	/* Go to the section containing the given tag: */
	const char* tagName=0;
	Section* sPtr=getSection(relativeTagPath,&tagName);
	
	/* Find the tag name in the section's tag list: */
	std::list<TagValue>::const_iterator tvIt;
	for(tvIt=sPtr->values.begin();tvIt!=sPtr->values.end()&&tvIt->tag!=tagName;++tvIt)
		;
	
	/* Return tag value: */
	if(tvIt==sPtr->values.end())
		{
		/* Add a new tag/value pair: */
		sPtr->values.push_back(TagValue(tagName,defaultValue));
		
		/* Mark section as edited: */
		sPtr->edited=true;
		
		return defaultValue;
		}
	else
		return tvIt->value;
	}

void ConfigurationFileBase::Section::storeTagValue(const char* relativeTagPath,const std::string& newValue)
	{
	/* Go to the section containing the given tag: */
	const char* tagName=0;
	Section* sPtr=getSection(relativeTagPath,&tagName);
	
	/* Add the tag/value pair: */
	sPtr->addTagValue(tagName,newValue);
	}

/**************************************
Methods of class ConfigurationFileBase:
**************************************/

ConfigurationFileBase::ConfigurationFileBase(void)
	:rootSection(new Section(0,std::string("")))
	{
	}

ConfigurationFileBase::ConfigurationFileBase(const char* sFileName)
	:rootSection(0)
	{
	/* Load the configuration file: */
	load(sFileName);
	}

ConfigurationFileBase::~ConfigurationFileBase(void)
	{
	delete rootSection;
	}

void ConfigurationFileBase::load(const char* newFileName)
	{
	/* Delete current configuration file contents: */
	delete rootSection;
	
	/* Create root section: */
	rootSection=new Section(0,std::string(""));
	
	/* Store the file name: */
	fileName=newFileName;
	
	/* Merge contents of given configuration file: */
	merge(newFileName);
	
	/* Reset edit flag: */
	rootSection->clearEditFlag();
	}

void ConfigurationFileBase::merge(const char* mergeFileName)
	{
	/* Try opening configuration file: */
	File file(mergeFileName,"rt");
	
	/* Read configuration file contents: */
	Section* sectionPtr=rootSection;
	int lineNumber=0;
	while(!file.eof())
		{
		/* Concatenate lines from configuration file: */
		std::string line="";
		char lineBuffer[1024];
		bool firstLine=true;
		while(true)
			{
			/* Read line from file: */
			if(file.gets(lineBuffer,sizeof(lineBuffer))==0)
				break;
			++lineNumber;
			
			/* Skip initial whitespace: */
			char* lineStartPtr;
			for(lineStartPtr=lineBuffer;*lineStartPtr!='\0'&&isspace(*lineStartPtr);++lineStartPtr)
				;
			
			/* Find end of line: */
			char* lineEndPtr;
			for(lineEndPtr=lineStartPtr;*lineEndPtr!='\0';++lineEndPtr)
				;
			
			/* Check if line was read completely: */
			if(lineEndPtr[-1]!='\n')
				throw MalformedConfigFileError(__PRETTY_FUNCTION__,"Line too long",lineNumber,mergeFileName);
			
			if(lineEndPtr-2>=lineBuffer&&lineEndPtr[-2]=='\\')
				{
				/* Remove the continuation characters "\\\n" from the buffer: */
				lineEndPtr[-2]='\0';
				
				/* Concatenate the current buffer to the line and continue reading: */
				if(firstLine||*lineStartPtr!='#')
					line+=lineStartPtr;
				}
			else
				{
				/* Remove the line end character from the buffer: */
				lineEndPtr[-1]='\0';
				
				/* Concatenate the current buffer to the line and stop reading: */
				if(firstLine||*lineStartPtr!='#')
					line+=lineStartPtr;
				break;
				}
			
			firstLine=false;
			}
		
		/* Get pointers to beginning and end of line: */
		const char* linePtr=line.data();
		const char* lineEndPtr=linePtr+line.size();
		
		/* Check if the line contains a comment: */
		for(const char* lPtr=linePtr;lPtr!=lineEndPtr;++lPtr)
			if(*lPtr=='#')
				{
				lineEndPtr=lPtr;
				break;
				}
		
		/* Remove whitespace from the end of the line: */
		while(lineEndPtr!=linePtr&&isspace(lineEndPtr[-1]))
			--lineEndPtr;
		
		/* Check for empty lines: */
		if(linePtr==lineEndPtr)
			continue;
		
		/* Extract first string from line: */
		const char* decodeEnd;
		std::string token=ValueCoder<std::string>::decode(linePtr,lineEndPtr,&decodeEnd);
		linePtr=decodeEnd;
		while(linePtr!=lineEndPtr&&isspace(*linePtr))
			++linePtr;
		
		if(strcasecmp(token.c_str(),"section")==0)
			{
			/* Check if the section name starts with a double quote for backwards compatibility: */
			std::string sectionName;
			if(linePtr!=lineEndPtr&&*linePtr=='\"')
				{
				/* Parse the section name as a string: */
				sectionName=ValueCoder<std::string>::decode(linePtr,lineEndPtr,&decodeEnd);
				}
			else
				{
				/* Read everything after the "section" token as the section name, including whitespace and special characters: */
				sectionName=std::string(linePtr,lineEndPtr);
				}
			
			/* Add a new subsection to the current section and make it the current section: */
			if(sectionName.empty())
				throw MalformedConfigFileError(__PRETTY_FUNCTION__,"Missing section name after section command",lineNumber,mergeFileName);
			sectionPtr=sectionPtr->addSubsection(sectionName);
			}
		else if(strcasecmp(token.c_str(),"endsection")==0)
			{
			/* End the current section: */
			if(sectionPtr->parent!=0)
				sectionPtr=sectionPtr->parent;
			else
				throw MalformedConfigFileError(__PRETTY_FUNCTION__,"Extra endsection command",lineNumber,mergeFileName);
			}
		else if(linePtr!=lineEndPtr)
			{
			/* Check for the special "+=" operator: */
			if(*linePtr=='+'&&linePtr+1!=lineEndPtr&&linePtr[1]=='=')
				{
				/* Skip the operator and whitespace and get the new tag value: */
				for(linePtr+=2;linePtr!=lineEndPtr&&isspace(*linePtr);++linePtr)
					;
				if(linePtr!=lineEndPtr)
					{
					/* Get the current tag value, defaulting to an empty list if the tag does not exist yet: */
					std::string currentValue=sectionPtr->retrieveTagValue(token.c_str(),"()");
					
					/* Check that the current tag ends with a closing parenthesis, and the new tag value starts with an opening parenthesis: */
					if(*linePtr=='('&&*(currentValue.end()-1)==')')
						{
						/* Concatenate the current and new tag values: */
						currentValue.erase(currentValue.end()-1);
						
						/* Insert a list item separator if the current value is not the empty list: */
						if(*(currentValue.end()-1)!='(')
							currentValue.append(", ");
						
						currentValue.append(std::string(linePtr+1,lineEndPtr));
						
						/* Store the concatenated tag values: */
						sectionPtr->addTagValue(token,currentValue);
						}
					else
						throw MalformedConfigFileError(__PRETTY_FUNCTION__,"+= operator used on non-list",lineNumber,mergeFileName);
					}
				}
			else
				{
				/* Add a tag/value pair to the current section: */
				sectionPtr->addTagValue(token,std::string(linePtr,lineEndPtr));
				}
			}
		else
			{
			/* Remove the tag from the current section: */
			sectionPtr->removeTag(token);
			}
		}
	}

void ConfigurationFileBase::mergeCommandline(int& argc,char**& argv)
	{
	/* Process all command line arguments: */
	for(int i=1;i<argc;++i)
		{
		/* Check for a tag: */
		if(argv[i][0]=='-')
			{
			if(i<argc-1)
				{
				/* Add the tag/value pair to the root section: */
				rootSection->storeTagValue(argv[i]+1,argv[i+1]);
				
				/* Remove the tag and value from the command line: */
				argc-=2;
				for(int j=i;j<argc;++j)
					argv[j]=argv[j+2];
				i-=2;
				}
			else
				{
				/* Remove the solo tag from the command line: */
				--argc;
				--i;
				}
			}
		}
	}

void ConfigurationFileBase::saveAs(const char* newFileName)
	{
	/* Store the new file name: */
	fileName=newFileName;
	
	/* Save the root section: */
	File file(fileName.c_str(),"wt");
	rootSection->save(file,0);
	}

namespace {

/**************
Helper classes:
**************/

class FileParser // Class to parse and patch configuration files
	{
	/* Embedded classes: */
	private:
	enum CharacterClasses // Enumerated type for character classes
		{
		None=0x0,
		Whitespace=0x1,
		String=0x2,
		AutoQuotedString=0x4,
		QuotedString=0x8
		};
	
	struct Section // Structure identifying a section during configuration file parsing
		{
		/* Elements: */
		const char* pathBegin; // Pointer to beginning of path suffix associated with this section, or null if off the path
		const char* pathEnd; // Pointer to the end of the path suffix's first component, or null if off the path
		bool tagSection; // Flag whether this is the section containing the tag to be replaced
		bool tagReplaced; // Flag whether the tag has already be replaced
		
		/* Constructors and destructors: */
		Section(const char* parentPathEnd) // Creates a child section
			:pathBegin(parentPathEnd),
			 pathEnd(0),
			 tagSection(false),tagReplaced(false)
			{
			if(pathBegin!=0)
				{
				/* Skip any slashes at the beginning of the path suffix: */
				while(*pathBegin=='/')
					++pathBegin;
				
				/* Find the end of the path suffix's first component: */
				for(pathEnd=pathBegin;*pathEnd!='\0'&&*pathEnd!='/';++pathEnd)
					;
				
				/* Check if this is the final section along the path: */
				tagSection=*pathEnd=='\0';
				}
			}
		};
	
	/* Elements: */
	private:
	int ccArray[257]; // Array of character class bit masks
	int* cc; // Adjusted pointer into character class array
	FILE* inFile; // The input file
	unsigned int line; // The current line number in the input file
	int c; // The last character read from the input file
	bool copy; // Flag whether to copy characters from the input to the output file
	FILE* outFile; // The (temporary) output file
	
	/* Private methods: */
	void strip(std::string& string) // Strips trailing whitespace from the given string
		{
		while(!string.empty()&&(cc[int(string.back())]&Whitespace)!=0)
			string.pop_back();
		}
	unsigned int skipWhitespace(void) // Skips whitespace in the current line
		{
		unsigned int result=0;
		
		/* Skip whitespace: */
		while((cc[c]&Whitespace)!=0x0)
			{
			if(copy)
				fputc(c,outFile);
			c=fgetc(inFile);
			++result;
			}
		
		return result;
		}
	void skipComment(void) // Skips a comment in the current line
		{
		/* Check for the comment character: */
		if(c=='#')
			{
			/* Skip until the end of the current line: */
			while(c!=EOF&&c!='\n')
				{
				if(copy)
					fputc(c,outFile);
				c=fgetc(inFile);
				}
			}
		}
	void continueLine(void)
		{
		/* Skip the line end: */
		if(copy)
			fputc(c,outFile);
		c=fgetc(inFile);
		++line;
		
		/* Skip initial whitespace in the continued line: */
		while((cc[c]&Whitespace)!=0x0)
			{
			if(copy)
				fputc(c,outFile);
			c=fgetc(inFile);
			}
		}
	std::string readString(bool autoQuote,bool& quoted) // Reads a quoted or unquoted string
		{
		std::string result;
		
		/* Set the character class for non-escaped string characters: */
		int stringCc=autoQuote?AutoQuotedString:String;
		
		/* Check if the string is quoted or not: */
		if(c=='"')
			{
			quoted=true;
			
			/* Adjust the character class: */
			stringCc=QuotedString;
			
			/* Skip the quote character: */
			c=fgetc(inFile);
			}
		else
			quoted=false;
		
		/* Read characters until an invalid string character is encountered: */
		while((cc[c]&stringCc)!=0x0)
			{
			/* Store the character and read the next: */
			result.push_back(c);
			c=fgetc(inFile);
			}
		
		/* Check if the string was quoted: */
		if(stringCc==QuotedString)
			{
			/* Check for and skip the closing quote: */
			if(c!='"')
				throw makeStdErr(__PRETTY_FUNCTION__,"Missing closing quote in line %u",line);
			c=fgetc(inFile);
			}
		
		return result;
		}
	unsigned int writeString(const std::string& string,bool quoted)
		{
		if(quoted)
			fputc('"',outFile);
		fputs(string.c_str(),outFile);
		if(quoted)
			fputc('"',outFile);
		return quoted?string.length()+2:string.length();
		}
	std::string readValue(void)
		{
		std::string result;
		
		/* Read the tag value, including the optional special "+=" operator: */
		skipWhitespace();
		while(c!=EOF&&c!='\n')
			{
			/* Check for line continuation: */
			if(c=='\\')
				{
				/* Read the next character: */
				if(copy)
					fputc(c,outFile);
				c=fgetc(inFile);
				if(c=='\n')
					{
					/* Process the line continuation: */
					continueLine();
					}
				else
					{
					/* False alarm: */
					result.push_back('\\');
					}
				}
			else
				{
				/* Store the character and read the next: */
				result.push_back(c);
				if(copy)
					fputc(c,outFile);
				c=fgetc(inFile);
				}
			}
		
		return result;
		}
	void writeValue(const char* value,const char* whitespace,unsigned int width)
		{
		for(const char* vPtr=value;*vPtr!='\0';++vPtr)
			{
			if(*vPtr=='\n')
				{
				/* Insert a line continuation: */
				fputc('\\',outFile);
				fputc('\n',outFile);
				fputs(whitespace,outFile);
				for(unsigned int i=0;i<width;++i)
					fputc(' ',outFile);
				}
			else
				fputc(*vPtr,outFile);
			}
		}
	
	/* Constructors and destructors: */
	public:
	FileParser(FILE* sInFile,FILE* sOutFile) // Creates a parser for the given file
		:cc(ccArray+1),
		 inFile(sInFile),
		 line(1),c(fgetc(inFile)),copy(true),
		 outFile(sOutFile)
		{
		/* Initialize the character classes: */
		cc[EOF]=None;
		for(int c=0;c<256;++c)
			{
			cc[c]=None;
			if(c!='\n'&&isspace(c))
				cc[c]|=Whitespace;
			if(isalnum(c)||c=='_'||c=='\\')
				cc[c]|=String;
			if(c!='\n'&&c!='#')
				cc[c]|=AutoQuotedString;
			if(c!='\n'&&c!='"')
				cc[c]|=QuotedString;
			}
		}
	
	/* Methods: */
	void patch(const char* replaceTag,const char* replaceValue)
		{
		/* Create a stack of visited sections along the replacement tag's path: */
		std::vector<Section> sections;
		
		/* Enter the root section into the stack: */
		sections.push_back(Section(replaceTag));
		
		/* Read the input file one (continued) line at a time: */
		while(c!=EOF)
			{
			/* Read initial whitespace: */
			char whitespace[1024];
			char* wsPtr=whitespace;
			while((cc[c]&Whitespace)!=0x0)
				{
				*(wsPtr++)=c;
				fputc(c,outFile);
				c=fgetc(inFile);
				}
			*wsPtr='\0';
			
			/* Read the tag, which could be a "section" or "endsection" keyword: */
			bool quoted;
			std::string tag=readString(false,quoted);
			if(tag=="section")
				{
				/* Output the section keyword: */
				writeString(tag,quoted);
				
				/* Read and output the section name: */
				skipWhitespace();
				std::string section=readString(true,quoted);
				writeString(section,quoted);
				strip(section);
				
				/* Check if the new section is on the path to the replacement tag: */
				Section& s=sections.back();
				if(s.pathBegin!=0&&section==std::string(s.pathBegin,s.pathEnd))
					{
					/* Push an on-path section: */
					sections.push_back(Section(s.pathEnd));
					}
				else
					{
					/* Push an off-path section: */
					sections.push_back(Section(0));
					}
				}
			else if(tag=="endsection")
				{
				/* Check if the replacement tag has to be inserted: */
				Section& s=sections.back();
				if(s.tagSection&&!s.tagReplaced)
					{
					/* Output the replacement tag and value: */
					fputc('\t',outFile);
					unsigned int width=writeString(std::string(s.pathBegin,s.pathEnd),false);
					fputc(' ',outFile);
					++width;
					*(wsPtr++)='\t';
					*wsPtr='\0';
					writeValue(replaceValue,whitespace,width);
					
					/* Start a new line with the same initial whitespace: */
					fputc('\n',outFile);
					*(--wsPtr)='\0';
					fputs(whitespace,outFile);
					}
				
				/* Output the endsection keyword: */
				writeString(tag,quoted);
				
				/* Pop the section: */
				sections.pop_back();
				if(sections.empty())
					throw makeStdErr(__PRETTY_FUNCTION__,"Extra endsection in line %u",line);
				}
			else if(!tag.empty())
				{
				/* Output the tag: */
				unsigned int width=writeString(tag,quoted);
				
				/* Replace the value if this is the tag: */
				width+=skipWhitespace();
				Section& s=sections.back();
				if(s.tagSection&&tag==std::string(s.pathBegin,s.pathEnd))
					{
					/* Skip the value: */
					copy=false;
					readValue();
					copy=true;
					
					/* Write the replacement value: */
					writeValue(replaceValue,whitespace,width);
					
					s.tagReplaced=true;
					}
				else
					{
					/* Read the value: */
					std::string value=readValue();
					}
				}
			
			/* Skip whitespace and/or comment: */
			skipWhitespace();
			skipComment();
			
			/* Check that the line was completely read: */
			if(c!=EOF&&c!='\n')
				throw makeStdErr(__PRETTY_FUNCTION__,"Malformed line in line %u",line);
			
			/* Skip the end-of-line: */
			if(c=='\n')
				{
				fputc(c,outFile);
				++line;
				c=fgetc(inFile);
				}
			}
		}
	};

}

void ConfigurationFileBase::patchFile(const char* fileName,const char* tagPath,const char* newValue)
	{
	/* Open a temporary output file: */
	size_t fnLen=strlen(fileName);
	char* tempFileName=new char[fnLen+6+1];
	memcpy(tempFileName,fileName,fnLen);
	for(int i=0;i<6;++i)
		tempFileName[fnLen+i]='X';
	tempFileName[fnLen+6]='\0';
	int tempFd=mkstemp(tempFileName);
	if(tempFd<0)
		throw makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot patch configuration file %s",fileName);
	
	try
		{
		/* Put a File wrapper around the temporary file: */
		File tempFile(tempFd,"w+");
		
		/* Try opening the given configuration file: */
		File file(fileName,"rt");
		
		/* Create a file parser: */
		FileParser parser(file.getFilePtr(),tempFile.getFilePtr());
		
		/* Remove leading slashes from the tag path: */
		while(*tagPath=='/')
			++tagPath;
		
		/* Patch the file: */
		parser.patch(tagPath,newValue);
		}
	catch(const std::runtime_error& err)
		{
		/* Delete the temporary file: */
		unlink(tempFileName);
		
		/* Re-throw the exception: */
		throw makeStdErr(__PRETTY_FUNCTION__,"Cannot patch configuration file %s due to exception %s",fileName,err.what());
		}
	
	/* Atomically replace the original file with the temporary file: */
	if(rename(tempFileName,fileName)!=0)
		{
		/* Delete the temporary file and throw an exception: */
		int error=errno;
		unlink(tempFileName);
		throw makeLibcErr(__PRETTY_FUNCTION__,error,"Cannot patch configuration file %s",fileName);
		}
	}

#if 0 // This shit is obsolete

namespace {

/****************
Helper functions:
****************/

char processEscape(int c)
	{
	switch(c)
		{
		case 'a':
			return '\a';
		
		case 'b':
			return '\b';
		
		case 'f':
			return '\f';
		
		case 'n':
			return '\n';
		
		case 'r':
			return '\r';
		
		case 't':
			return '\t';
		
		case 'v':
			return '\v';
		
		default:
			return char(c);
		}
	}

/**************
Helper classes:
**************/

struct SectionMatch // Structure defining the match of a section with the tag path
	{
	/* Elements: */
	public:
	std::string name; // The section name
	const char* matchedPrefix; // Pointer to the first component of the tag path that is not matched by this section, or NULL
	const char* nextSlash; // Pointer to the next slash or end-of-string in the tag path
	
	/* Constructors and destructors: */
	SectionMatch(const char* sNameBegin,const char* sNameEnd,const char* sMatchedPrefix =0,const char* sNextSlash =0)
		:name(sNameBegin,sNameEnd),
		 matchedPrefix(sMatchedPrefix),nextSlash(sNextSlash)
		{
		}
	};

}

void ConfigurationFileBase::patchFile(const char* fileName,const char* tagPath,const char* newValue)
	{
	/* Open a temporary output file: */
	size_t fnLen=strlen(fileName);
	char* tempFileName=new char[fnLen+6+1];
	memcpy(tempFileName,fileName,fnLen);
	for(int i=0;i<6;++i)
		tempFileName[fnLen+i]='X';
	tempFileName[fnLen+6]='\0';
	int tempFd=mkstemp(tempFileName);
	if(tempFd<0)
		throw makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot patch configuration file %s",fileName);
	
	try
		{
		/* Put a File wrapper around the temporary file: */
		File tempFile(tempFd,"w+");
		
		/* Try opening the given configuration file: */
		File file(fileName,"rt");
		
		/* Create a stack of matched sections: */
		std::vector<SectionMatch> sections; // Stack of section names
		
		/* Remove leading slashes from the tag path: */
		while(*tagPath=='/')
			++tagPath;
		
		/* Enter the root section into the stack as a sentinel: */
		const char* matchedPrefix=tagPath; // Pointer to the first character of the tag path not yet matched to the current section path
		const char* nextSlash; // Pointer to the next slash (or end of string) in the tag path
		for(nextSlash=matchedPrefix;*nextSlash!='/'&&*nextSlash!='\0';++nextSlash)
			;
		sections.push_back(SectionMatch(tagPath,tagPath,matchedPrefix,nextSlash)); // A root section as sentinel
		
		/* Parse the given configuration file: */
		enum State // States of the file parser automaton
			{
			LINE, // At beginning of line, reading whitespace
			TAG,QUOTEDTAG, // Parsing a naked or quoted tag
			SECTIONWS, // Reading whitespace between the "section" keyword and the section name
			SECTION,QUOTEDSECTION, // Reading a naked or quoted section name
			VALUEWS, // Reading whitespace between a tag and its value
			VALUEWSPLUS, // Read a "+" between a tag and its value
			VALUEWSPLUSEQUAL, // Read a "+=" between a tag and its value
			VALUE, // Parsing a value
			VALUESKIPWS, // Skipping whitespace inside a value due to a line break
			SKIPVALUE, // Skipping the value of a matched tag
			COMMENT, // Skipping a comment
			SKIPLINE // Skipping until the end of the current line
			} state=LINE;
		unsigned int lineNumber=1;
		char whitespace[1024]; // Whitespace at the beginning of the current line
		char* wsPtr=whitespace;
		char tag[1024]; // The currently-parsed tag
		char* tPtr=tag;
		char section[1024]; // The currently-parsed section name
		char* sPtr=section;
		bool inTagSection=false; // Flag whether the parser is currently inside the tag's section
		bool tagValueReplaced=false; // Flag whether the tag has been replaced
		std::string value; // The currently-parsed value
		// bool valueContinuation=false; // Flag whether the current value is a list continuation
		bool escaped=false;
		int quote=-1;
		bool copyChars=true;
		int c;
		while((c=file.getc())>=0)
			{
			switch(state)
				{
				case LINE:
					if(c=='\n')
						{
						++lineNumber;
						wsPtr=whitespace;
						}
					else if(c=='#')
						state=COMMENT;
					else if(c=='"'||c=='\'')
						{
						quote=c;
						state=QUOTEDTAG;
						tPtr=tag;
						copyChars=false;
						}
					else if(isspace(c))
						*(wsPtr++)=char(c);
					else
						{
						state=TAG;
						tPtr=tag;
						if(c=='\\')
							escaped=true;
						else
							*(tPtr++)=char(c);
						copyChars=false;
						}
					break;
				
				case TAG:
					if(escaped)
						{
						if(c=='\n')
							++lineNumber;
						else
							*(tPtr++)=processEscape(c);
						escaped=false;
						}
					else if(c=='\\')
						escaped=true;
					else if(c=='\n'||isspace(c))
						{
						/* Tag is finished; process it: */
						*tPtr='\0';
						if(strcasecmp(tag,"section")==0)
							{
							if(c=='\n')
								throw MalformedConfigFileError(__PRETTY_FUNCTION__,"Missing section name",lineNumber,fileName);
							else
								state=SECTIONWS;
							}
						else if(strcasecmp(tag,"endsection")==0)
							{
							/* Check if this was the tag's section, and the tag has not been replaced: */
							if(inTagSection)
								{
								if(!tagValueReplaced)
									{
									/* Put the tag and value into the current section before leaving it: */
									for(const char* tPtr=matchedPrefix;*tPtr!='\0';++tPtr)
										fputc(*tPtr,tempFile.getFilePtr());
									fputc(' ',tempFile.getFilePtr());
									for(const char* vPtr=newValue;*vPtr!='\0';++vPtr)
										{
										if(*vPtr=='\n')
											{
											/* Start a new line and indent it to the tag's indentation: */
											fputc('\n',tempFile.getFilePtr());
											for(char* iPtr=whitespace;iPtr!=wsPtr;++iPtr)
												fputc(*iPtr,tempFile.getFilePtr());
											
											}
										else
											fputc(*vPtr,tempFile.getFilePtr());
										}
									std::cout<<"Entered tag "<<matchedPrefix<<" "<<newValue<<" before leaving section"<<std::endl;
									}
								
								// DEBUGGING
								std::cout<<"Leaving tag's section"<<std::endl;
								inTagSection=false;
								}
							
							/* Pop the current section off the stack: */
							if(sections.size()<=1)
								throw MalformedConfigFileError(__PRETTY_FUNCTION__,"Extra endsection command",lineNumber,fileName);
							sections.pop_back();
							if(sections.back().matchedPrefix!=0)
								{
								matchedPrefix=sections.back().matchedPrefix;
								nextSlash=sections.back().nextSlash;
								}
							
							if(c=='\n')
								{
								/* Parse the next line: */
								++lineNumber;
								state=LINE;
								wsPtr=whitespace;
								}
							else
								state=SKIPLINE;
							}
						else
							{
							/* Check if the current section path matches the tag path: */
							if(sections.back().matchedPrefix!=0&&*nextSlash=='\0')
								{
								/* Match the tag against the tag path's suffix: */
								char* cPtr1;
								const char* cPtr2;
								for(cPtr1=tag,cPtr2=matchedPrefix;cPtr1<tPtr&&cPtr2<nextSlash&&*cPtr1==*cPtr2;++cPtr1,++cPtr2)
									;
								if(cPtr1==tPtr&&cPtr2==nextSlash)
									{
									/* Output the replacement value to the temporary file: */
									fputc(' ',tempFile.getFilePtr());
									for(const char* vPtr=newValue;*vPtr!='\0';++vPtr)
										{
										if(*vPtr=='\n')
											{
											/* Start a new line and indent it to the tag's indentation: */
											fputc('\n',tempFile.getFilePtr());
											for(char* iPtr=whitespace;iPtr!=wsPtr;++iPtr)
												fputc(*iPtr,tempFile.getFilePtr());
											
											}
										else
											fputc(*vPtr,tempFile.getFilePtr());
										}
									
									/* Stop copying characters and skip the tag's value: */
									copyChars=false;
									state=SKIPVALUE;
									}
								else
									state=VALUEWS;
								}
							else
								state=VALUEWS;
							}
						
						if(c=='\n')
							{
							/* Parse the next line: */
							++lineNumber;
							copyChars=true;
							state=LINE;
							wsPtr=whitespace;
							}
						}
					else
						*(tPtr++)=char(c);
					break;
				
				case QUOTEDTAG:
					if(escaped)
						{
						if(c=='\n')
							++lineNumber;
						else
							*(tPtr++)=processEscape(c);
						escaped=false;
						}
					else if(c=='\\')
						escaped=true;
					else if(c!=quote)
						*(tPtr++)=char(c);
					else
						{
						/* Tag is finished; process it: */
						*tPtr='\0';
						if(strcasecmp(tag,"section")==0)
							state=SECTIONWS;
						else if(strcasecmp(tag,"endsection")==0)
							{
							/* Check if this was the tag's section, and the tag has not been replaced: */
							if(inTagSection)
								{
								if(!tagValueReplaced)
									{
									/* Put the tag and value into the current section before leaving it: */
									std::cout<<"Entered tag "<<matchedPrefix<<" "<<newValue<<" before leaving section"<<std::endl;
									// ...
									}
								
								// DEBUGGING
								std::cout<<"Leaving tag's section"<<std::endl;
								inTagSection=false;
								}
							
							/* Pop the current section off the stack: */
							if(sections.size()<=1)
								throw MalformedConfigFileError(__PRETTY_FUNCTION__,"Extra endsection command",lineNumber,fileName);
							sections.pop_back();
							if(sections.back().matchedPrefix!=0)
								{
								matchedPrefix=sections.back().matchedPrefix;
								nextSlash=sections.back().nextSlash;
								}
							
							/* Skip the rest of the line: */
							state=SKIPLINE;
							}
						else
							{
							/* Check if the current section path matches the tag path: */
							if(sections.back().matchedPrefix!=0&&*nextSlash=='\0')
								{
								/* Match the tag against the tag path's suffix: */
								char* cPtr1;
								const char* cPtr2;
								for(cPtr1=tag,cPtr2=matchedPrefix;cPtr1<tPtr&&cPtr2<nextSlash&&*cPtr1==*cPtr2;++cPtr1,++cPtr2)
									;
								if(cPtr1==tPtr&&cPtr2==nextSlash)
									{
									/* Output the replacement value to the temporary file: */
									fputc(quote,tempFile.getFilePtr());
									fputc(' ',tempFile.getFilePtr());
									for(const char* vPtr=newValue;*vPtr!='\0';++vPtr)
										{
										if(*vPtr=='\n')
											{
											/* Start a new line and indent it to the tag's indentation: */
											fputc('\n',tempFile.getFilePtr());
											for(char* iPtr=whitespace;iPtr!=wsPtr;++iPtr)
												fputc(*iPtr,tempFile.getFilePtr());
											
											}
										else
											fputc(*vPtr,tempFile.getFilePtr());
										}
									tagValueReplaced=true;
									
									/* Stop copying characters and skip the tag's value: */
									copyChars=false;
									state=SKIPVALUE;
									}
								else
									state=VALUEWS;
								}
							else
								state=VALUEWS;
							}
						}
					break;
				
				case SECTIONWS:
					if(c=='\n'||c=='#')
						throw MalformedConfigFileError(__PRETTY_FUNCTION__,"Missing section name",lineNumber,fileName);
					else if(c=='"'||c=='\'')
						{
						quote=c;
						state=QUOTEDSECTION;
						sPtr=section;
						}
					else if(!isspace(c))
						{
						state=SECTION;
						sPtr=section;
						if(c=='\\')
							escaped=true;
						else
							*(sPtr++)=char(c);
						}
					break;
				
				case SECTION:
					if(escaped)
						{
						if(c=='\n')
							++lineNumber;
						else
							*(sPtr++)=processEscape(c);
						escaped=false;
						}
					else if(c=='\\')
						escaped=true;
					else if(c=='\n'||c=='#')
						{
						/* Section name is finished, remove trailing whitespace and process it: */
						while(sPtr>section&&isspace(sPtr[-1]))
							--sPtr;
						SectionMatch sm(section,sPtr);
						
						/* Check if the current section path matches a prefix of the given tag path: */
						if(sections.back().matchedPrefix!=0&&*nextSlash!='\0')
							{
							/* Match this section name against the unmatched suffix of the tag path: */
							char* cPtr1;
							const char* cPtr2;
							for(cPtr1=section,cPtr2=matchedPrefix;cPtr1<sPtr&&cPtr2<nextSlash&&*cPtr1==*cPtr2;++cPtr1,++cPtr2)
								;
							if(cPtr1==sPtr&&cPtr2==nextSlash)
								{
								matchedPrefix=nextSlash+1;
								for(nextSlash=matchedPrefix;*nextSlash!='/'&&*nextSlash!='\0';++nextSlash)
									;
								sm.matchedPrefix=matchedPrefix;
								sm.nextSlash=nextSlash;
								
								/* Check if this is the section containing the tag: */
								if(*nextSlash=='\0')
									{
									inTagSection=true;
									
									// DEBUGGING
									std::cout<<"Entering tag's section"<<std::endl;
									}
								}
							}
						
						/* Push the section onto the stack: */
						sections.push_back(sm);
						
						if(c=='#')
							state=COMMENT;
						else
							{
							/* Parse the next line: */
							++lineNumber;
							state=LINE;
							wsPtr=whitespace;
							}
						}
					else
						*(sPtr++)=char(c);
					break;
				
				case QUOTEDSECTION:
					if(escaped)
						{
						if(c=='\n')
							++lineNumber;
						else
							*(sPtr++)=processEscape(c);
						escaped=false;
						}
					else if(c=='\\')
						escaped=true;
					else if(c==quote)
						{
						/* Section name is finished, process it: */
						SectionMatch sm(section,sPtr);
						
						/* Check if the current section path matches a prefix of the given tag path: */
						if(sections.back().matchedPrefix!=0&&*nextSlash!='\0')
							{
							/* Match this section name against the unmatched suffix of the tag path: */
							char* cPtr1;
							const char* cPtr2;
							for(cPtr1=section,cPtr2=matchedPrefix;cPtr1<sPtr&&cPtr2<nextSlash&&*cPtr1==*cPtr2;++cPtr1,++cPtr2)
								;
							if(cPtr1==sPtr&&cPtr2==nextSlash)
								{
								matchedPrefix=nextSlash+1;
								for(nextSlash=matchedPrefix;*nextSlash!='/'&&*nextSlash!='\0';++nextSlash)
									;
								sm.matchedPrefix=matchedPrefix;
								sm.nextSlash=nextSlash;
								
								/* Check if this is the section containing the tag: */
								if(*nextSlash=='\0')
									{
									inTagSection=true;
									
									// DEBUGGING
									std::cout<<"Entering tag's section"<<std::endl;
									}
								}
							}
						
						/* Push the section onto the stack: */
						sections.push_back(sm);
						
						/* Skip the rest of the line: */
						state=SKIPLINE;
						}
					else
						*(sPtr++)=char(c);
					break;
				
				case VALUEWS:
					if(c=='\n'||c=='#')
						{
						/* Value is empty; do nothing */
						
						if(c=='#')
							state=COMMENT;
						else
							{
							/* Parse next line: */
							++lineNumber;
							state=LINE;
							wsPtr=whitespace;
							}
						}
					else if(c=='+')
						state=VALUEWSPLUS;
					else if(!isspace(c))
						{
						state=VALUE;
						value.clear();
						if(c=='\\')
							escaped=true;
						else
							value.push_back(char(c));
						// valueContinuation=false;
						}
					break;
				
				case VALUEWSPLUS:
					if(c=='=')
						state=VALUEWSPLUSEQUAL;
					else
						throw MalformedConfigFileError(__PRETTY_FUNCTION__,"Malformed += list continuation",lineNumber,fileName);
					break;
				
				case VALUEWSPLUSEQUAL:
					if(c=='(')
						{
						state=VALUE;
						value.clear();
						// valueContinuation=true;
						}
					else if(c=='\n'||!isspace(c))
						throw MalformedConfigFileError(__PRETTY_FUNCTION__,"Malformed += list continuation",lineNumber,fileName);
					break;
				
				case VALUE:
					if(escaped)
						{
						if(c=='\n')
							{
							++lineNumber;
							state=VALUESKIPWS;
							}
						else
							{
							/* Escape sequences other than line breaks are copied verbatim to be parsed by value coders: */
							value.push_back('\\');
							value.push_back(char(c));
							}
						escaped=false;
						}
					else if(c=='\\')
						escaped=true;
					else if(c=='"'||c=='\'')
						{
						if(quote==c)
							{
							/* End a quote: */
							quote=-1;
							}
						else if(quote==-1)
							{
							/* Start a quote: */
							quote=c;
							}
						value.push_back(c);
						}
					else if(c=='\n'||(c=='#'&&quote==-1))
						{
						/* Value is finished */
						
						if(c=='#')
							state=COMMENT;
						else
							{
							/* Parse next line: */
							++lineNumber;
							state=LINE;
							wsPtr=whitespace;
							}
						}
					else
						value.push_back(c);
					break;
				
				case VALUESKIPWS:
					if(c=='\\')
						{
						state=VALUE;
						escaped=true;
						}
					else if(c=='"'||c=='\'')
						{
						if(quote==c)
							{
							/* End a quote: */
							quote=-1;
							}
						else if(quote==-1)
							{
							/* Start a quote: */
							quote=c;
							}
						state=VALUE;
						value.push_back(c);
						}
					else if(c=='\n'||(c=='#'&&quote==-1))
						{
						/* Value is finished */
						
						if(c=='#')
							state=COMMENT;
						else
							{
							/* Parse next line: */
							++lineNumber;
							state=LINE;
							wsPtr=whitespace;
							}
						}
					else if(!isspace(c))
						{
						state=VALUE;
						value.push_back(c);
						}
					break;
				
				case SKIPVALUE:
					if(escaped)
						{
						if(c=='\n')
							++lineNumber;
						escaped=false;
						}
					else if(c=='\\')
						escaped=true;
					else if(c=='"'||c=='\'')
						{
						if(quote==c)
							{
							/* End a quote: */
							quote=-1;
							}
						else if(quote==-1)
							{
							/* Start a quote: */
							quote=c;
							}
						}
					else if(c=='\n'||(c=='#'&&quote==-1))
						{
						/* Value is skipped: */
						copyChars=true;
						
						if(c=='#')
							state=COMMENT;
						else
							{
							/* Parse the next line: */
							++lineNumber;
							state=LINE;
							wsPtr=whitespace;
							}
						}
					break;
				
				case COMMENT:
					if(c=='\n')
						{
						/* Parse the next line: */
						++lineNumber;
						state=LINE;
						wsPtr=whitespace;
						}
					break;
				
				case SKIPLINE:
					if(c=='\n')
						{
						/* Parse the next line: */
						++lineNumber;
						state=LINE;
						wsPtr=whitespace;
						}
					else if(c=='#')
						state=COMMENT;
					else if(!isspace(c))
						throw MalformedConfigFileError(__PRETTY_FUNCTION__,"Dangling bits at line end",lineNumber,fileName);
					break;
				}
			
			if(copyChars)
				fputc(c,tempFile.getFilePtr());
			}
		}
	catch(...)
		{
		/* Delete the temporary file: */
		unlink(tempFileName);
		
		/* Re-throw the exception: */
		throw;
		}
	
	/* Atomically replace the original file with the temporary file: */
	if(rename(tempFileName,fileName)!=0)
		{
		/* Delete the temporary file and throw an exception: */
		int error=errno;
		unlink(tempFileName);
		throw makeLibcError(__PRETTY_FUNCTION__,error,"Cannot patch configuration file %s",fileName);
		}
	}

#endif // End of obsolete shit

/*****************************************
Methods of class ConfigurationFileSection:
*****************************************/

std::string ConfigurationFileSection::getPath(void) const
	{
	return baseSection->getPath();
	}

void ConfigurationFileSection::setSection(const char* relativePath)
	{
	baseSection=baseSection->getSection(relativePath);
	}

ConfigurationFileSection ConfigurationFileSection::getSection(const char* relativePath) const
	{
	return ConfigurationFileSection(baseSection->getSection(relativePath));
	}

void ConfigurationFileSection::clear(void)
	{
	baseSection->clear();
	}

void ConfigurationFileSection::removeSubsection(const std::string& subsectionName)
	{
	baseSection->removeSubsection(subsectionName);
	}

void ConfigurationFileSection::removeTag(const std::string& tag)
	{
	baseSection->removeTag(tag);
	}

/**********************************
Methods of class ConfigurationFile:
**********************************/

void ConfigurationFile::load(const char* newFileName)
	{
	/* Call base class method: */
	ConfigurationFileBase::load(newFileName);
	
	/* Reset the current section pointer to the root section: */
	baseSection=rootSection;
	}

std::string ConfigurationFile::getCurrentPath(void) const
	{
	return baseSection->getPath();
	}

void ConfigurationFile::setCurrentSection(const char* relativePath)
	{
	baseSection=baseSection->getSection(relativePath);
	}

ConfigurationFileSection ConfigurationFile::getCurrentSection(void) const
	{
	return ConfigurationFileSection(baseSection);
	}

ConfigurationFileSection ConfigurationFile::getSection(const char* relativePath) const
	{
	return ConfigurationFileSection(baseSection->getSection(relativePath));
	}

void ConfigurationFile::list(void) const
	{
	/* List all subsections of current section: */
	for(Section* sPtr=baseSection->firstSubsection;sPtr!=0;sPtr=sPtr->sibling)
		printf("%s/\n",sPtr->name.c_str());
	
	/* List all tags in current section: */
	for(std::list<Section::TagValue>::const_iterator tvIt=baseSection->values.begin();tvIt!=baseSection->values.end();++tvIt)
		printf("%s\n",tvIt->tag.c_str());
	}

}
