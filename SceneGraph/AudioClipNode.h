/***********************************************************************
AudioClipNode - Class for audio clips that can be played by Sound nodes.
Copyright (c) 2021-2023 Oliver Kreylos

This file is part of the Simple Scene Graph Renderer (SceneGraph).

The Simple Scene Graph Renderer is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Simple Scene Graph Renderer is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Simple Scene Graph Renderer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef SCENEGRAPH_AUDIOCLIPNODE_INCLUDED
#define SCENEGRAPH_AUDIOCLIPNODE_INCLUDED

#include <Misc/Autopointer.h>
#include <IO/VariableMemoryFile.h>
#include <AL/Config.h>
#include <AL/ALObject.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/Node.h>

/* Forward declarations: */
namespace IO {
class Directory;
}
namespace SceneGraph {
class ALRenderState;
}

namespace SceneGraph {

class AudioClipNode:public Node,public ALObject
	{
	/* Embedded classes: */
	protected:
	struct DataItem:public ALObject::DataItem
		{
		/* Elements: */
		ALuint bufferId; // ID of the audio buffer containing the sound waveform
		unsigned int version; // Version of sound waveform in the buffer
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	public:
	static const char* className; // The class's name
	
	/* Fields: */
	SFString description;
	SFBool loop;
	SFFloat pitch;
	SFTime startTime;
	SFTime stopTime;
	MFString url;
	
	/* Derived state: */
	protected:
	int soundFileFormat; // Identifier for supported sound file formats, currently oniy 0 - WAV and 1 - invalid
	IO::VariableMemoryFilePtr soundFile; // In-memory copy of the sound file containing the audio clip's waveform
	unsigned int version; // Version number of sound waveform
	
	/* Private methods: */
	private:
	void loadSoundFile(IO::Directory& baseDirectory); // Loads the sound file referenced by the current value of the url field, relative to the given base directory
	
	/* Constructors and destructors: */
	public:
	AudioClipNode(void); // Creates a default audio clip node with no sound
	
	/* Methods from class Node: */
	virtual const char* getClassName(void) const;
	virtual void parseField(const char* fieldName,VRMLFile& vrmlFile);
	virtual void update(void);
	virtual void read(SceneGraphReader& reader);
	virtual void write(SceneGraphWriter& writer) const;
	
	/* Methods from class ALObject: */
	virtual void initContext(ALContextData& contextData) const;
	
	/* New methods: */
	void setUrl(const std::string& newUrl,IO::Directory& baseDirectory); // Sets a sound URL relative to the given base directory
	void setUrl(const std::string& newUrl); // Ditto, with URL relative to the current directory
	ALuint getBufferObject(ALRenderState& renderState) const; // Returns the ID of the OpenAL buffer object containing the current sound waveform
	};

typedef Misc::Autopointer<AudioClipNode> AudioClipNodePointer;

}

#endif
