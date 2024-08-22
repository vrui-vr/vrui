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

#include <Vulkan/ShaderModule.h>

#include <vector>
#include <IO/File.h>
#include <IO/Directory.h>
#include <Vulkan/Common.h>
#include <Vulkan/Device.h>

namespace Vulkan {

/*************************************
Static elements of class ShaderModule:
*************************************/

const char* ShaderModule::stageDirectoryNames[ShaderModule::NUM_STAGES]=
	{
	"vertex","tesselationcontrol","tesselationevaluation","geometry","fragment","compute"
	};

/*****************************
Methods of class ShaderModule:
*****************************/

ShaderModule::ShaderModule(Device& sDevice,const IO::Directory& baseDir,ShaderModule::Stage sStage,const char* shaderName,const char* sEntryPoint)
	:DeviceAttached(sDevice),
	 stage(sStage),
	 shaderModule(VK_NULL_HANDLE),
	 entryPoint(sEntryPoint!=0?sEntryPoint:"main")
	{
	/* Construct the name of the shader bytecode file: */
	std::string shaderFileName=stageDirectoryNames[stage];
	shaderFileName.push_back('/');
	shaderFileName.append(shaderName);
	shaderFileName.append(".spv");
	
	/* Open the shader bytecode file: */
	IO::FilePtr shaderFile=baseDir.openFile(shaderFileName.c_str());
	
	/* Read the file's contents into a vector of bytes: */
	std::vector<char> bytecode;
	while(!shaderFile->eof())
		{
		/* Read a chunk of data directly from the file's buffer: */
		void* buffer;
		size_t bufferSize=shaderFile->readInBuffer(buffer);
		
		/* Append the read chunk to the byte vector: */
		char* bufferBytes=reinterpret_cast<char*>(buffer);
		bytecode.insert(bytecode.end(),bufferBytes,bufferBytes+bufferSize);
		}
	
	/* Set up the shader module creation structure: */
	VkShaderModuleCreateInfo shaderModuleCreateInfo;
	shaderModuleCreateInfo.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.pNext=0;
	shaderModuleCreateInfo.flags=0x0;
	shaderModuleCreateInfo.codeSize=bytecode.size();
	shaderModuleCreateInfo.pCode=reinterpret_cast<uint32_t*>(bytecode.data());
	
	/* Create the shader module: */
	throwOnError(vkCreateShaderModule(device.getHandle(),&shaderModuleCreateInfo,0,&shaderModule),
	             __PRETTY_FUNCTION__,"create Vulkan shader module");
	}

ShaderModule::~ShaderModule(void)
	{
	/* Destroy the shader module: */
	vkDestroyShaderModule(device.getHandle(),shaderModule,0);
	}

}
