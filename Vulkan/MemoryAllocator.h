/***********************************************************************
MemoryAllocator - Class managing allocation of different Vulkan memory
types.
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

#ifndef VULKAN_MEMORYALLOCATOR_INCLUDED
#define VULKAN_MEMORYALLOCATOR_INCLUDED

#include <vector>
#include <vulkan/vulkan.h>
#include <Vulkan/Common.h>
#include <Vulkan/DeviceAttached.h>

namespace Vulkan {

class MemoryAllocator:public DeviceAttached
	{
	/* Embedded classes: */
	private:
	struct MemoryBlock // Structure representing an allocated block of a certain type of memory
		{
		/* Embedded classes: */
		public:
		struct FreeChunk // Structure representing available memory chunks
			{
			/* Elements: */
			public:
			VkDeviceSize size;
			VkDeviceSize offset;
			
			/* Constructors and destructors: */
			FreeChunk(VkDeviceSize sSize,VkDeviceSize sOffset)
				:size(sSize),offset(sOffset)
				{
				}
			};
		
		/* Elements: */
		uint32_t memoryType; // Device-specific memory type
		VkMemoryPropertyFlags propertyFlags; // Memory type's property flags
		bool exportable; // Flag whether the memory block can be exported
		VkDeviceMemory deviceMemory; // Vulkan device memory handle
		VkDeviceSize size; // Memory block's allocated size
		VkDeviceSize atomSize; // Memory block's atom size for mapping to host memory
		std::vector<FreeChunk> freeList; // The list of available memory chunks, ordered by offset
		MemoryBlock* succ; // Pointer to the next memory block of the same type
		
		/* Constructors and destructors: */
		MemoryBlock(uint32_t sMemoryType,VkMemoryPropertyFlags sPropertyFlags,bool sExportable,VkDeviceMemory sDeviceMemory,VkDeviceSize sSize,VkDeviceSize sAtomSize);
		
		/* Methods: */
		VkDeviceMemory getHandle(void) const // Returns the block's device memory handle
			{
			return deviceMemory;
			}
		VkDeviceSize getSize(void) const // Returns the block's allocated size
			{
			return size;
			}
		VkDeviceSize allocate(VkDeviceSize chunkSize,VkDeviceSize chunkAlignment); // Returns the offset of a previously available memory chunk of the given size, or the size of the memory block if no chunk is big enough
		void release(VkDeviceSize chunkSize,VkDeviceSize chunkOffset); // Releases a previously allocated memory chunk
		};
	
	public:
	class Allocation // Class representing a memory allocation
		{
		friend class MemoryAllocator;
		
		/* Elements: */
		private:
		MemoryBlock* memoryBlock; // Pointer to the memory block in which the memory was allocated
		VkDeviceSize size; // Size of the allocation
		VkDeviceSize offset; // Offset of the allocation
		
		/* Constructors and destructors: */
		public:
		Allocation(void) // Creates an invalid allocation
			:memoryBlock(0),size(0),offset(0)
			{
			}
		private:
		Allocation(MemoryBlock& sMemoryBlock,VkDeviceSize sSize,VkDeviceSize sOffset) // Elementwise construction
			:memoryBlock(&sMemoryBlock),size(sSize),offset(sOffset)
			{
			}
		Allocation(const Allocation& source); // Prohibit copy constructor
		Allocation& operator=(const Allocation& source); // Prohibit assignment operator
		public:
		Allocation(Allocation&& source) // Move constructor
			:memoryBlock(source.memoryBlock),size(source.size),offset(source.offset)
			{
			/* Invalidate the source object: */
			source.memoryBlock=0;
			source.offset=source.size=0;
			}
		Allocation& operator=(Allocation&& source) // Move assignment operator
			{
			if(this!=&source)
				{
				/* Release the currently allocated memory: */
				if(memoryBlock!=0)
					memoryBlock->release(size,offset);
				
				/* Copy source state: */
				memoryBlock=source.memoryBlock;
				size=source.size;
				offset=source.offset;
				
				/* Invalidate the source object: */
				source.memoryBlock=0;
				source.offset=source.size=0;
				}
			
			return *this;
			}
		~Allocation(void) // Releases the allocated memory
			{
			if(memoryBlock!=0)
				memoryBlock->release(size,offset);
			}
		
		/* Methods: */
		bool isValid(void) const // Returns true if the allocation is valid
			{
			return memoryBlock!=0;
			}
		bool isHostCoherent(void) const // Returns true if the memory block backing the allocation chunk is host coherent
			{
			return memoryBlock!=0?(memoryBlock->propertyFlags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)!=0x0U:true;
			}
		VkDeviceMemory getHandle(void) const // Returns the device memory handle of the allocated chunk
			{
			return memoryBlock!=0?memoryBlock->getHandle():VK_NULL_HANDLE;
			}
		VkDeviceSize getBlockSize(void) const // Returns the size of the memory block containing the allocated chunk
			{
			return memoryBlock!=0?memoryBlock->getSize():0;
			}
		VkDeviceSize getSize(void) const // Returns the allocated chunk's size
			{
			return size;
			}
		VkDeviceSize getOffset(void) const // Returns the allocated chunk's offset
			{
			return offset;
			}
		VkDeviceSize getAtomSize(void) const // Returns the allocated chunk's atom size for mapping to host memory
			{
			return memoryBlock->atomSize;
			}
		};
	
	/* Elements: */
	private:
	VkDeviceSize blockSize; // Allocation size for memory blocks
	VkDeviceSize nonCoherentAtomSize; // Atom size when mapping non-host coherent memory ranges
	std::vector<MemoryBlock*> blockChains; // List of chains of allocated memory blocks
	
	/* Private methods: */
	uint32_t findMemoryType(const VkMemoryRequirements& requirements,VkMemoryPropertyFlags properties) const; // Finds a memory type that matches the given requirements
	MemoryBlock* allocateBlock(uint32_t memoryType,VkMemoryPropertyFlags propertyFlags,bool exportable,VkDeviceSize size); // Allocates a block of memory
	
	/* Constructors and destructors: */
	public:
	MemoryAllocator(Device& sDevice,VkDeviceSize sBlockSize,VkDeviceSize sNonCoherentAtomSize); // Creates an allocator for the given Vulkan device
	private:
	MemoryAllocator(const MemoryAllocator& source); // Prohibit copy constructor
	MemoryAllocator& operator=(const MemoryAllocator& source); // Prohibit assignment operator
	public:
	~MemoryAllocator(void); // Releases all allocated memory and destroys the allocator
	
	/* Methods: */
	static CStringList& addRequiredInstanceExtensions(CStringList& extensions); // Adds the list of instance extensions required to allocate device memory to the given extension list
	static CStringList& addRequiredDeviceExtensions(CStringList& extensions); // Adds the list of device extensions required to allocate device memory to the given extension list
	Allocation allocate(const VkMemoryRequirements& requirements,VkMemoryPropertyFlags properties,bool exportable); // Allocates a chunk of memory fitting the given requirements and properties
	void* map(const Allocation& allocation,VkMemoryMapFlags flags); // Maps the given allocated memory chunk to CPU-accessible memory and returns a pointer
	void unmap(const Allocation& allocation); // Unmaps currently mapped device memory region
	int getExportFd(const Allocation& allocation); // Returns a file descriptor to import memory allocated as exportable into another process
	};

}

#endif
