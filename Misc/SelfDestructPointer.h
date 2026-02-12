/***********************************************************************
SelfDestructPointer - Class for pointers to new-allocated objects that
automatically deletes the object when the pointer goes out of scope.
Does not support multiple references to the same object or anything
fancy like that (use Misc::Autopointer instead), but does not require
any help from the target class, either.
Copyright (c) 2008-2026 Oliver Kreylos

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

#ifndef MISC_SELFDESTRUCTPOINTER_INCLUDED
#define MISC_SELFDESTRUCTPOINTER_INCLUDED

namespace Misc {

template <class TargetParam>
class SelfDestructPointer
	{
	/* Embedded classes: */
	public:
	typedef TargetParam Target; // Type of pointed-to class
	
	/* Elements: */
	private:
	Target* target; // Pointer to the owned target
	
	/* Constructors and destructors: */
	public:
	SelfDestructPointer(void) // Creates invalid pointer
		:target(0)
		{
		}
	SelfDestructPointer(Target* sTarget) // Takes ownership of the given target
		:target(sTarget)
		{
		}
	private:
	SelfDestructPointer(const SelfDestructPointer& source); // Prohibit copy constructor
	public:
	SelfDestructPointer(SelfDestructPointer&& source) // Move constructor
		:target(source.target)
		{
		/* Invalidate the source: */
		source.target=0;
		}
	~SelfDestructPointer(void) // Destroys the pointer target
		{
		/* Destroy the target: */
		delete target;
		}
	
	/* Methods: */
	SelfDestructPointer& operator=(Target* newTarget) // Assignment operator for standard pointer; destroys current target and takes ownership of the given target
		{
		/* Destroy the current target: */
		delete target;
		
		/* Take ownership of the new target: */
		target=newTarget;
		
		return *this;
		}
	private:
	SelfDestructPointer& operator=(const SelfDestructPointer& source); // Prohibit copy assignment operator
	public:
	SelfDestructPointer& operator=(SelfDestructPointer&& source) // Move assignment operator; destroys current target and transfers ownership of the source's target
		{
		if(target!=source.target)
			{
			/* Destroy the current target: */
			delete target;
			
			/* Take ownership of the source's target and invalidate the source: */
			target=source.target;
			source.target=0;
			}
		
		return *this;
		}
	bool isValid(void) const // Returns true if the target is valid
		{
		return target!=0;
		}
	Target& operator*(void) const // Indirection operator
		{
		return *target;
		}
	Target* operator->(void) const // Arrow operator
		{
		return target;
		}
	Target* getTarget(void) const // Returns standard pointer to target
		{
		return target;
		}
	void setTarget(Target* newTarget) // Legacy version of the equivalent operator= method
		{
		/* Destroy the current target: */
		delete target;
		
		/* Take ownership of the new target: */
		target=newTarget;
		}
	Target* releaseTarget(void) // Releases ownership of the current target and returns a standard pointer to it
		{
		Target* result=target;
		target=0;
		return result;
		}
	};

}

#endif
