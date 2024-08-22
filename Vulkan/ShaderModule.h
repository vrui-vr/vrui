/***********************************************************************
ShaderModule - Class representing Vulkan shader modules.
Copyright (c) 2022-2023 Oliver Kreylos

This file is part of the Vulkan C++ Wrapper Library (Vulkan).

The Vulkan C++ Wrapper Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Vulkan C++ Wrapper Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vulkan C++ Wrapper Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef VULKAN_SHADERMODULE_INCLUDED
#define VULKAN_SHADERMODULE_INCLUDED

#include <string>
#include <vulkan/vulkan.h>
#include <Vulkan/DeviceAttached.h>

/* Forward declarations: */
namespace IO {
class Directory;
}

namespace Vulkan {

class ShaderModule:public DeviceAttached
	{
	/* Embedded classes: */
	public:
	enum Stage // Enumerated type for shader stages
		{
		Vertex=0,
		TessellationControl,
		TessellationEvaluation,
		Geometry,
		Fragment,
		Compute,
		NUM_STAGES
		};
	
	/* Elements: */
	private:
	static const char* stageDirectoryNames[NUM_STAGES]; // Array of names of shader stage directories
	Stage stage; // Shader stage to which the shader module can be attached
	VkShaderModule shaderModule; // Vulkan shader module handle
	std::string entryPoint; // Function name of the module's entry point
	
	/* Constructors and destructors: */
	public:
	ShaderModule(Device& sDevice,const IO::Directory& baseDir,Stage sStage,const char* shaderName,const char* sEntryPoint =0); // Creates a shader module for the named shader in the given stage by reading a SPIR-V bytecode file from underneath the given base directory
	virtual ~ShaderModule(void);
	
	/* Methods: */
	Stage getStage(void) const // Returns the shader stage to which the shader module can be attached
		{
		return stage;
		}
	VkShaderModule getHandle(void) const // Returns the Vulkan shader module handle
		{
		return shaderModule;
		}
	const std::string& getEntryPoint(void) const // Returns the function name of the shader module's entry point
		{
		return entryPoint;
		}
	};

}

#endif
