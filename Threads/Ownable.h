/***********************************************************************
Ownable - Base class for reference-counted shared objects where a subset
of reference holders hold some form of ownership of the object, the
upshot being that an object derived from this class will get notified if
the owner count reaches zero, and will get destroyed if the reference
count reaches zero.
Copyright (c) 2026 Oliver Kreylos

This file is part of the Portable Threading Library (Threads).

The Portable Threading Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Portable Threading Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Portable Threading Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef THREADS_OWNABLE_INCLUDED
#define THREADS_OWNABLE_INCLUDED

#include <Misc/Autopointer.h>
#include <Threads/Atomic.h>

namespace Threads {

class Ownable
	{
	/* Elements: */
	private:
	Atomic<unsigned int> refCount; // Current number of references to this object held by pointers of any type
	bool owned; // Flag if the object currently has an owner
	
	/* Protected methods: */
	protected:
	virtual void disowned(void) // Method called when an owned object has just become disowned
		{
		}
	
	/* Constructors and destructors: */
	public:
	Ownable(void) // Creates a presumed-owned object with a reference count of zero
		:refCount(0),owned(true)
		{
		}
	Ownable(const Ownable& source) // Copy constructor; creates a presumed-owned object with a reference count of zero
		:refCount(0),owned(true)
		{
		}
	virtual ~Ownable(void) // Destructor
		{
		}
	
	/* Methods: */
	Ownable& operator=(const Ownable& source) // Copy assignment operator; assigning changes neither reference count nor ownership status
		{
		return *this;
		}
	void ref(void) // Method called when a non-owning pointer starts referencing this object
		{
		/* Increment the reference counter: */
		refCount.preAdd(1);
		}
	void unref(void) // Method called when a non-owning autopointer stops referencing this object; destroys this object when the reference count reaches zero
		{
		/* Decrement the reference counter, and delete this object if the count reached zero: */
		if(refCount.preSub(1)==0)
			delete this;
		}
	bool isOwned(void) const // Returns true if this object has an owner
		{
		return owned;
		}
	void own(void) // Method called when an ownership-establishing pointer starts referencing this object
		{
		/* Mark this object as owned: */
		// owned=true; This is actually a no-op; we don't need to know when an object was owned, only when it was disowned
		
		/* Increment the reference counter: */
		refCount.preAdd(1);
		}
	void disown(void) // Method called when an ownership-establishing pointer stops referencing this object
		{
		/* Mark this object as disowned: */
		owned=false;
		
		/* Signal this object that it has been disowned: */
		disowned();
		
		/* Decrement the reference counter, and delete this object if the count reached zero: */
		if(refCount.preSub(1)==0)
			delete this;
		}
	};

template <class TargetParam>
class OwningPointer // Smart pointer class to manage lifetime (through reference counting) and ownership of ownable objects
	{
	/* Embedded classes: */
	public:
	typedef TargetParam Target; // Type of the objects that can be owned by objects of this class
	typedef Misc::Autopointer<Target> SharedPointer; // Non-owning smart pointer referencing the same target type
	
	/* Elements: */
	private:
	Target* owned; // Pointer to the owned object
	
	/* Constructors and destructors: */
	public:
	OwningPointer(void) // Creates an invalid owner pointer
		:owned(0)
		{
		}
	OwningPointer(Target* sOwned) // Creates owner pointer from standard pointer; takes ownership of the object
		:owned(sOwned)
		{
		/* Take ownership of the object: */
		if(owned!=0)
			owned->own();
		}
	private:
	OwningPointer(const OwningPointer& source); // Prohibit copy constructor
	public:
	OwningPointer(OwningPointer&& source) // Move constructor; transfers object ownership from source to this pointer
		:owned(source.owned)
		{
		/* Invalidate the source: */
		source.owned=0;
		}
	~OwningPointer(void) // Destructor; releases ownership of the owned object
		{
		/* Release ownership of the currently owned object: */
		if(owned!=0)
			owned->disown();
		}
	
	/* Methods: */
	OwningPointer& operator=(Target* newOwned) // Assignment operator from standard pointer; takes ownership of the object
		{
		/* Check that this is not an identity assignment: */
		if(owned!=newOwned)
			{
			/* Disown the currently owned object: */
			if(owned!=0)
				owned->disown();
			
			/* Take ownership of the new object: */
			owned=newOwned;
			if(owned!=0)
				owned->own();
			}
		
		return *this;
		}
	private:
	OwningPointer& operator=(const OwningPointer& source); // Prohibit copy assignment operator
	public:
	OwningPointer& operator=(OwningPointer&& source) // Move assignment operator; transfers object ownership from source to this pointer
		{
		/* Check that this is not an identity assignment: */
		if(this!=&source)
			{
			/* Release ownership of the currently owned object: */
			if(owned!=0)
				owned->disown();
			
			/* Take over the source's owned object and invalidate the source: */
			owned=source.owned;
			source.owned=0;
			}
		
		return *this;
		}
	friend bool operator==(const OwningPointer& ptr1,const OwningPointer& ptr2) // Equality operator
		{
		return ptr1.owned==ptr2.owned;
		}
	friend bool operator==(const OwningPointer& ptr1,const Target* ptr2) // Ditto
		{
		return ptr1.owned==ptr2;
		}
	friend bool operator==(const Target* ptr1,const OwningPointer& ptr2) // Ditto
		{
		return ptr1==ptr2.owned;
		}
	friend bool operator!=(const OwningPointer& ptr1,const OwningPointer& ptr2) // Inequality operator
		{
		return ptr1.owned!=ptr2.owned;
		}
	friend bool operator!=(const OwningPointer& ptr1,const Target* ptr2) // Ditto
		{
		return ptr1.owned!=ptr2;
		}
	friend bool operator!=(const Target* ptr1,const OwningPointer& ptr2) // Ditto
		{
		return ptr1!=ptr2.owned;
		}
	Target& operator*(void) const // Indirection operator
		{
		return *owned;
		}
	Target* operator->(void) const // Arrow operator
		{
		return owned;
		}
	SharedPointer share(void) // Return a non-owning smart pointer to the owned object
		{
		return SharedPointer(owned);
		}
	};

}

#endif
