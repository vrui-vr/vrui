/***********************************************************************
MemoryAllocator - Class managing allocation of different Vulkan memory
types.
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

#include <Vulkan/MemoryAllocator.h>

#include <Misc/StdError.h>
#include <Vulkan/Device.h>

namespace Vulkan {

/*********************************************
Methods of class MemoryAllocator::MemoryBlock:
*********************************************/

MemoryAllocator::MemoryBlock::MemoryBlock(uint32_t sMemoryType,VkMemoryPropertyFlags sPropertyFlags,bool sExportable,VkDeviceMemory sDeviceMemory,VkDeviceSize sSize,VkDeviceSize sAtomSize)
	:memoryType(sMemoryType),propertyFlags(sPropertyFlags),exportable(sExportable),
	 deviceMemory(sDeviceMemory),size(sSize),
	 atomSize(sAtomSize),
	 succ(0)
	{
	/* Initialize the free chunk list: */
	freeList.push_back(FreeChunk(size,0));
	}

VkDeviceSize MemoryAllocator::MemoryBlock::allocate(VkDeviceSize chunkSize,VkDeviceSize chunkAlignment)
	{
	/* Find the best-fitting free memory chunk in the free list: */
	std::vector<FreeChunk>::iterator bestFlIt=freeList.end();
	VkDeviceSize bestSize=chunkSize;
	VkDeviceSize aOffset;
	for(std::vector<FreeChunk>::iterator flIt=freeList.begin();flIt!=freeList.end();++flIt)
		{
		/* Align the free chunk: */
		aOffset=((flIt->offset+chunkAlignment-1)/chunkAlignment)*chunkAlignment;
		if(flIt->offset+flIt->size>aOffset)
			{
			VkDeviceSize aSize=flIt->offset+flIt->size-aOffset;
			if(aSize==chunkSize)
				{
				/* Perfect fit, take it: */
				bestFlIt=flIt;
				break;
				}
			else if(bestSize<aSize)
				{
				/* Choose the biggest free chunk to split: */
				bestFlIt=flIt;
				bestSize=aSize;
				}
			}
		}
	
	/* Check if we're out of memory: */
	if(bestFlIt==freeList.end())
		return size;
	
	VkDeviceSize result;
	if(aOffset==bestFlIt->offset)
		{
		/* Allocate from the beginning of the best-fitting chunk: */
		result=bestFlIt->offset;
		bestFlIt->offset+=chunkSize;
		bestFlIt->size-=chunkSize;
		
		/* Remove the free chunk if it has been exhausted: */
		if(bestFlIt->size==0)
			freeList.erase(bestFlIt);
		}
	else
		{
		/* Allocate from the aligned beginning of the best-fitting chunk: */
		result=aOffset;
		VkDeviceSize chunkEnd=bestFlIt->offset+bestFlIt->size;
		bestFlIt->size=aOffset-bestFlIt->offset;
		
		/* Insert a new chunk if there is leftover space at the end: */
		if(chunkEnd>aOffset+chunkSize)
			freeList.insert(bestFlIt+1,FreeChunk(chunkEnd-(aOffset+chunkSize),aOffset+chunkSize));
		}
	
	return result;
	}

void MemoryAllocator::MemoryBlock::release(VkDeviceSize chunkSize,VkDeviceSize chunkOffset)
	{
	/* Find the insertion position for the newly-freed chunk in the free list: */
	std::vector<FreeChunk>::iterator insertIt;
	for(insertIt=freeList.begin();insertIt!=freeList.end()&&insertIt->offset<chunkOffset;++insertIt)
		;
	
	/* Check if the newly-freed chunk can be merged with adjacent free chunks: */
	bool canMergeLeft=insertIt!=freeList.begin()&&insertIt[-1].offset+insertIt[-1].size==chunkOffset;
	bool canMergeRight=insertIt!=freeList.end()&&chunkOffset+chunkSize==insertIt[0].offset;
	if(canMergeLeft&&canMergeRight)
		{
		/* Expand the free chunk to the left and remove the free chunk to the right: */
		insertIt[-1].size+=chunkSize+insertIt[0].size;
		freeList.erase(insertIt);
		}
	else if(canMergeLeft)
		{
		/* Merge with the free chunk to the left: */
		insertIt[-1].size+=chunkSize;
		}
	else if(canMergeRight)
		{
		/* Merge with the free chunk to the right: */
		insertIt[0].offset=chunkOffset;
		insertIt[0].size+=chunkSize;
		}
	else
		{
		/* Insert a new free chunk: */
		freeList.insert(insertIt,FreeChunk(chunkSize,chunkOffset));
		}
	}

/********************************
Methods of class MemoryAllocator:
********************************/

uint32_t MemoryAllocator::findMemoryType(const VkMemoryRequirements& requirements,VkMemoryPropertyFlags properties) const
	{
	/* Query the device's memory properties: */
	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(device.getPhysicalHandle(),&physicalDeviceMemoryProperties);
	
	/* Find a memory type that matches both the memory requirements and the requested memory properties: */
	for(uint32_t memoryType=0;memoryType<physicalDeviceMemoryProperties.memoryTypeCount;++memoryType)
		{
		/* Check whether the memory type is in the supported type set: */
		if(requirements.memoryTypeBits&(0x1U<<memoryType))
			{
			/* Check whether the memory type has all the requested memory type properties: */
			if((physicalDeviceMemoryProperties.memoryTypes[memoryType].propertyFlags&properties)==properties)
				{
				/* Stop looking and return the found type: */
				return memoryType;
				}
			}
		}
	
	/* Throw an exception: */
	throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No memory type matching requirements found");
	}

MemoryAllocator::MemoryBlock* MemoryAllocator::allocateBlock(uint32_t memoryType,VkMemoryPropertyFlags propertyFlags,bool exportable,VkDeviceSize size)
	{
	/* Set up a memory allocation structure: */
	VkMemoryAllocateInfo memoryAllocateInfo;
	memoryAllocateInfo.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.pNext=0;
	memoryAllocateInfo.allocationSize=size;
	memoryAllocateInfo.memoryTypeIndex=memoryType;
	
	/* Check if the memory is supposed to be externally visible: */
	VkExportMemoryAllocateInfoKHR exportMemoryAllocateInfo;
	if(exportable)
		{
		/* Set up an external memory allocation structure: */
		exportMemoryAllocateInfo.sType=VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR;
		exportMemoryAllocateInfo.pNext=0;
		exportMemoryAllocateInfo.handleTypes=VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
		
		/* Link the structure to the base structure: */
		memoryAllocateInfo.pNext=&exportMemoryAllocateInfo;
		}
	
	/* Allocate device memory: */
	VkDeviceMemory deviceMemory;
	throwOnError(vkAllocateMemory(device.getHandle(),&memoryAllocateInfo,0,&deviceMemory),
	             __PRETTY_FUNCTION__,"allocate Vulkan device memory");
	
	/* Return a new memory block: */
	VkDeviceSize atomSize=1;
	if((propertyFlags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)==0x0U)
		atomSize=nonCoherentAtomSize;
	return new MemoryBlock(memoryType,propertyFlags,exportable,deviceMemory,size,atomSize);
	}

MemoryAllocator::MemoryAllocator(Device& sDevice,VkDeviceSize sBlockSize,VkDeviceSize sNonCoherentAtomSize)
	:DeviceAttached(sDevice),
	 blockSize(sBlockSize),
	 nonCoherentAtomSize(sNonCoherentAtomSize)
	{
	}

MemoryAllocator::~MemoryAllocator(void)
	{
	/* Release all allocated memory blocks: */
	for(std::vector<MemoryBlock*>::iterator bcIt=blockChains.begin();bcIt!=blockChains.end();++bcIt)
		{
		MemoryBlock* chainHead=*bcIt;
		while(chainHead!=0)
			{
			/* Free the device memory: */
			vkFreeMemory(device.getHandle(),chainHead->deviceMemory,0);
			
			/* Delete the memory block and go to the next one: */
			MemoryBlock* succ=chainHead->succ;
			delete chainHead;
			chainHead=succ;
			}
		}
	}

CStringList& MemoryAllocator::addRequiredInstanceExtensions(CStringList& extensions)
	{
	/* Add the external memory extensions and their requirements: */
	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	extensions.push_back(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
	
	return extensions;
	}

CStringList& MemoryAllocator::addRequiredDeviceExtensions(CStringList& extensions)
	{
	/* Add the external memory extensions and their requirements: */
	extensions.push_back(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
	extensions.push_back(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
	
	return extensions;
	}

MemoryAllocator::Allocation MemoryAllocator::allocate(const VkMemoryRequirements& requirements,VkMemoryPropertyFlags properties,bool exportable)
	{
	/* Look for a memory block matching the requirements and properties in the block chain list: */
	MemoryBlock* chainHead=0;
	for(std::vector<MemoryBlock*>::iterator bcIt=blockChains.begin();bcIt!=blockChains.end();++bcIt)
		{
		/* Check if the block chain matches: */
		MemoryBlock* mb=*bcIt;
		if((requirements.memoryTypeBits&(0x1U<<mb->memoryType))!=0x0U&&(mb->propertyFlags&properties)==properties&&mb->exportable==exportable)
			{
			/* Allocate from this block chain: */
			chainHead=mb;
			break;
			}
		}
	
	/* If no matching block chain was found, start a new one by allocating a new block matching the requirements: */
	if(chainHead==0)
		{
		chainHead=allocateBlock(findMemoryType(requirements,properties),properties,exportable,blockSize);
		blockChains.push_back(chainHead);
		}
	
	/* Allocate memory within the found block chain: */
	MemoryBlock* currentBlock=chainHead;
	while(true)
		{
		/* Try allocating memory in the current block: */
		VkDeviceSize offset=currentBlock->allocate(requirements.size,requirements.alignment);
		if(offset!=VkDeviceSize(-1))
			{
			/* Found a matching block: */
			return Allocation(*currentBlock,requirements.size,offset);
			}
		else
			{
			/* Allocate a new memory block and try again: */
			currentBlock->succ=allocateBlock(chainHead->memoryType,chainHead->propertyFlags,chainHead->exportable,blockSize);
			currentBlock=currentBlock->succ;
			}
		}
	}

void* MemoryAllocator::map(const MemoryAllocator::Allocation& allocation,VkMemoryMapFlags flags)
	{
	if(allocation.memoryBlock==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Memory is not allocated");
	
	/* Map the buffer's memory region: */
	void* result=0;
	throwOnError(vkMapMemory(device.getHandle(),allocation.memoryBlock->deviceMemory,allocation.offset,allocation.size,flags,&result),
	             __PRETTY_FUNCTION__,"map Vulkan device memory");
	return result;
	}

void MemoryAllocator::unmap(const MemoryAllocator::Allocation& allocation)
	{
	if(allocation.memoryBlock==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Memory is not allocated");
	
	/* Unmap the buffer's memory region: */
	vkUnmapMemory(device.getHandle(),allocation.memoryBlock->deviceMemory);
	}

int MemoryAllocator::getExportFd(const MemoryAllocator::Allocation& allocation)
	{
	if(allocation.memoryBlock==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Memory is not allocated");
	
	/* Retrieve the function pointer: */
	PFN_vkGetMemoryFdKHR vkGetMemoryFdKHRFunc=device.getFunction<PFN_vkGetMemoryFdKHR>("vkGetMemoryFdKHR");
	
	/* Set up a command structure: */
	VkMemoryGetFdInfoKHR memoryGetFdInfo;
	memoryGetFdInfo.sType=VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
	memoryGetFdInfo.pNext=0;
	memoryGetFdInfo.memory=allocation.memoryBlock->deviceMemory;
	memoryGetFdInfo.handleType=VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
	
	/* Retrieve the file descriptor: */
	int result=0;
	throwOnError(vkGetMemoryFdKHRFunc(device.getHandle(),&memoryGetFdInfo,&result),
	             __PRETTY_FUNCTION__,"retrieve memory file descriptor");
	return result;
	}

}
