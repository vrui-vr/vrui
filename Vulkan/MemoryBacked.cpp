/***********************************************************************
MemoryBacked - Class representing Vulkan objects that are backed by GPU
or CPU memory.
Copyright (c) 2022-2024 Oliver Kreylos

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

#include <Vulkan/MemoryBacked.h>

#include <Misc/StdError.h>
#include <Vulkan/Device.h>

namespace Vulkan {

/*****************************
Methods of class MemoryBacked:
*****************************/

MemoryBacked::MemoryBacked(Device& sDevice)
	:DeviceAttached(sDevice)
	{
	}

MemoryBacked::MemoryBacked(MemoryBacked&& source)
	:DeviceAttached(std::move(source)),
	 allocation(std::move(source.allocation))
	{
	}

MemoryBacked::~MemoryBacked(void)
	{
	}

MemoryBacked& MemoryBacked::operator=(MemoryBacked&& source)
	{
	if(this!=&source)
		{
		if(device.getHandle()!=source.device.getHandle())
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot move memory-backed objects between devices");
		allocation=std::move(source.allocation);
		}
	
	return *this;
	}

void* MemoryBacked::map(VkMemoryMapFlags flags,bool willBeRead)
	{
	if(!allocation.isValid())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Memory is not allocated");
	
	/* Align the memory range to the allocated chunk's atom size: */
	VkDeviceSize start=allocation.getOffset();
	start=(start/allocation.getAtomSize())*allocation.getAtomSize();
	VkDeviceSize end=allocation.getOffset()+allocation.getSize();
	end=((end+allocation.getAtomSize()-1)/allocation.getAtomSize())*allocation.getAtomSize();
	
	/* Map the allocation's memory region: */
	void* result=0;
	throwOnError(vkMapMemory(device.getHandle(),allocation.getHandle(),start,end-start,flags,&result),
	             __PRETTY_FUNCTION__,"map Vulkan device memory");
	
	/* Invalidate the mapped memory range if it will be read and is not host coherent: */
	if(willBeRead&&!allocation.isHostCoherent())
		{
		/* Invalidate the entire allocated memory range: */
		VkMappedMemoryRange mappedMemoryRange;
		mappedMemoryRange.sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedMemoryRange.pNext=0;
		mappedMemoryRange.memory=allocation.getHandle();
		mappedMemoryRange.offset=start;
		mappedMemoryRange.size=end-start;
		
		throwOnError(vkInvalidateMappedMemoryRanges(device.getHandle(),1,&mappedMemoryRange),
		             __PRETTY_FUNCTION__,"invalidate mapped memory");
		}
	
	return static_cast<char*>(result)+(allocation.getOffset()-start);
	}

void MemoryBacked::unmap(bool wasWritten)
	{
	if(!allocation.isValid())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Memory is not allocated");
	
	/* Flush the mapped memory range if it was written and is not host coherent: */
	if(wasWritten&&!allocation.isHostCoherent())
		{
		/* Align the memory range to the allocated chunk's atom size: */
		VkDeviceSize start=allocation.getOffset();
		start=(start/allocation.getAtomSize())*allocation.getAtomSize();
		VkDeviceSize end=allocation.getOffset()+allocation.getSize();
		end=((end+allocation.getAtomSize()-1)/allocation.getAtomSize())*allocation.getAtomSize();
		
		/* Flush the entire allocated memory range: */
		VkMappedMemoryRange mappedMemoryRange;
		mappedMemoryRange.sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedMemoryRange.pNext=0;
		mappedMemoryRange.memory=allocation.getHandle();
		mappedMemoryRange.offset=start;
		mappedMemoryRange.size=end-start;
		
		throwOnError(vkFlushMappedMemoryRanges(device.getHandle(),1,&mappedMemoryRange),
		             __PRETTY_FUNCTION__,"flush mapped memory");
		}
	
	/* Unmap the allocation's memory region: */
	vkUnmapMemory(device.getHandle(),allocation.getHandle());
	}

int MemoryBacked::getExportFd(void) const
	{
	if(!allocation.isValid())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Memory is not allocated");
	
	/* Retrieve the function pointer: */
	PFN_vkGetMemoryFdKHR vkGetMemoryFdKHRFunc=device.getFunction<PFN_vkGetMemoryFdKHR>("vkGetMemoryFdKHR");
	
	/* Set up a command structure: */
	VkMemoryGetFdInfoKHR memoryGetFdInfo;
	memoryGetFdInfo.sType=VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
	memoryGetFdInfo.pNext=0;
	memoryGetFdInfo.memory=allocation.getHandle();
	memoryGetFdInfo.handleType=VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
	
	/* Retrieve the file descriptor: */
	int result=0;
	throwOnError(vkGetMemoryFdKHRFunc(device.getHandle(),&memoryGetFdInfo,&result),
	             __PRETTY_FUNCTION__,"retrieve memory file descriptor");
	return result;
	}

}
