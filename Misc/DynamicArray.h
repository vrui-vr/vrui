/***********************************************************************
DynamicArray - Base class for arrays, i.e., contiguous blocks of memory,
that grow and shrink based on their usage. Intended to be used by
higher-level data structures that need dynamic contiguous storage.
Copyright (c) 2026 Oliver Kreylos

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

#ifndef MISC_DYNAMICARRAY_INCLUDED
#define MISC_DYNAMICARRAY_INCLUDED

#include <stddef.h>

namespace Misc {

template <class ElementParam>
class DynamicArray
	{
	/* Embedded classes: */
	public:
	typedef ElementParam Element; // Type of elements in the array
	
	/* Elements: */
	private:
	size_t alloc1,alloc2; // The smaller and current allocated array sizes (used to calculate a Fibonacci sequence)
	protected:
	Element* elements; // Currently allocated memory block
	size_t numElements; // Number of elements in the array
	
	/* Private methods: */
	private:
	void destroy(void) // Destroys the current array
		{
		/* Destroy all array elements: */
		Element* elementsEnd=elements+numElements;
		for(Element* ePtr=elements;ePtr!=elementsEnd;++ePtr)
			ePtr->~Element();
		
		/* Release the allocated memory block: */
		free(elements);
		}
	void grow(void) // Increases the allocation size to the next bigger step
		{
		size_t alloc3=alloc1+alloc2;
		alloc1=alloc2;
		alloc2=alloc3;
		}
	void shrink(void) // Decreases the allocation size to the next smaller step
		{
		size_t alloc0=alloc2-alloc1;
		alloc2=alloc1;
		alloc1=alloc0;
		}
	void realloc(void) // Re-allocates the memory block after alloc1 and alloc2 have been updated
		{
		/* Allocate a new memory block and move over the current array entries: */
		Element* newElements=static_cast<Element*>(malloc(alloc2*sizeof(Element)));
		Element* elementsEnd=elements+numElements;
		Element* ePtr1=newElements;
		for(Element* ePtr0=elements;ePtr0!=elementsEnd;++ePtr0,++ePtr1)
			new (ePtr1) Element(std::move(*ePtr0));
		
		/* Replace the old memory block: */
		free(elements);
		elements=newElements;
		}
	
	/* Constructors and destructors: */
	public:
	DynamicArray(void) // Creates an empty array of default size
		:alloc1(1),alloc2(1),elements(static_cast<Element*>(malloc(alloc2*sizeof(Element)))),
		 numElements(0)
		{
		}
	private:
	DynamicArray(const DynamicArray& source); // Prohibit copy constructor
	public:
	DynamicArray(DynamicArray&& source) // Move constructor
		:alloc1(source.alloc1),alloc2(source.alloc2),elements(source.elements),numElements(source.numElements)
		{
		/* Invalidate the source: */
		source.alloc2=source.alloc1=0;
		source.elements=0;
		source.numElements=0;
		}
	~DynamicArray(void) // Destroys all array elements and releases allocated memory
		{
		/* Destroy the array: */
		destroy();
		}
	
	/* Methods: */
	private:
	DynamicArray& operator=(const DynamicArray& source); // Prohibit copy assignment operator
	public:
	DynamicArray& operator=(DynamicArray&& source) // Move assignment operator
		{
		if(this!=&source)
			{
			/* Destroy the current array: */
			destroy();
			
			/* Attach to the source's array and invalidate the source: */
			alloc1=source.alloc1;
			source.alloc1=0;
			alloc2=source.alloc2;
			source.alloc2=0;
			elements=source.elements;
			source.elements=0;
			numElements=source.numElements;
			source.numElements=0;
			}
		
		return *this;
		}
	bool reserve(size_t reserveNumElements) // Allocates a memory block large enough to hold the given number of elements; reservation will not survive a call to pop_back() or clear(); returns true if memory was re-allocated, i.e., references were invalidated
		{
		bool result=false;
		
		/* Check if the currently allocated block is too small: */
		if(alloc2<reserveNumElements)
			{
			/* Increase the allocation size until it is big enough: */
			while(alloc2<reserveNumElements)
				grow();
			
			/* Re-allocate the memory block: */
			realloc();
			
			result=true;
			}
		
		return result;
		}
	bool empty(void) const // Returns true if the array has no elements
		{
		return numElements==0;
		}
	size_t size(void) const // Returns the number of elements in the array
		{
		return numElements;
		}
	const Element* data(void) const // Returns a raw pointer to the start of the array
		{
		return elements;
		}
	Element* data(void) // Ditto
		{
		return elements;
		}
	const Element& operator[](size_t index) const // Returns the array element at the index-th position
		{
		return elements[index];
		}
	Element& operator[](size_t index) // Ditto
		{
		return elements[index];
		}
	bool push_back(const Element& newElement) // Adds a new element to the end of the array using a copy constructor; returns true if memory was re-allocated, i.e., references were invalidated
		{
		bool result=false;
		
		/* Increase the size of the memory block if there is no more space: */
		if(numElements==alloc2)
			{
			/* Grow and reallocate the memory block: */
			grow();
			realloc();
			
			result=true;
			}
		
		/* Append the new element to the end of the array: */
		new (elements+numElements) Element(newElement);
		++numElements;
		
		return result;
		}
	bool push_back(Element&& newElement) // Ditto, using a move constructor; returns true if memory was re-allocated, i.e., references were invalidated
		{
		bool result=false;
		
		/* Increase the size of the memory block if there is no more space: */
		if(numElements==alloc2)
			{
			/* Grow and reallocate the memory block: */
			grow();
			realloc();
			
			result=true;
			}
		
		/* Append the new element to the end of the array: */
		new (elements+numElements) Element(std::move(newElement));
		++numElements;
		
		return result;
		}
	bool pop_back(void) // Removes the last element from the array; returns true if memory was re-allocated, i.e., references were invalidated
		{
		bool result=false;
		
		/* Remove the element from the end of the array: */
		--numElements;
		elements[numElements].~Element();
		
		/* Decrease the size of the memory block if there is too much space: */
		if(numElements<alloc1&&alloc2>1) // This specific check avoids flip-flopping allocation sizes on pop/push or push/pop pairs; we also never go below allocation size 1
			{
			/* Shrink and reallocate the memory block: */
			shrink();
			realloc();
			
			result=true;
			}
		
		return result;
		}
	void clear(void) // Removes all elements from the array
		{
		/* Destroy all array elements: */
		Element* elementsEnd=elements+numElements;
		for(Element* ePtr=elements;ePtr!=elementsEnd;++ePtr)
			ePtr->~Element();
		numElements=0;
		
		/* Re-initialize the allocated memory block: */
		alloc2=alloc1=1;
		free(elements);
		elements=static_cast<Element*>(malloc(alloc2*sizeof(Element)));
		}
	};

}

#endif
