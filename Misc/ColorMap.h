/***********************************************************************
ColorMap - Templatized class to map scalar values within a defined range
to colors.
Copyright (c) 2023-2025 Oliver Kreylos

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

#ifndef MISC_COLORMAP_INCLUDED
#define MISC_COLORMAP_INCLUDED

#include <vector>

namespace Misc {

template <class ColorParam>
class ColorMap
	{
	/* Embedded classes: */
	public:
	typedef ColorParam Color; // Type of mapped colors
	
	struct Entry // Structure for color map entries
		{
		/* Elements: */
		public:
		double key; // The key value associated with this entry
		Color color; // The color associated with this entry
		
		/* Constructors and destructors: */
		Entry(void) // Dummy constructor
			{
			}
		Entry(double sKey,const Color& sColor) // Elementwise constructor
			:key(sKey),color(sColor)
			{
			}
		};
	
	/* Elements: */
	private:
	unsigned int numEntries; // Number of color map entries; always >=1
	Entry* entries; // Array of color map entries
	
	/* Constructors and destructors: */
	public:
	ColorMap(unsigned int sNumEntries,const double* sKeys,const Color* sColors); // Creates a color map for the given number of entries and source arrays
	ColorMap(const std::vector<Entry>& sEntries); // Creates a color map from the given vector of entries
	ColorMap(const ColorMap& source); // Copy constructor
	ColorMap(ColorMap&& source); // Move constructor
	~ColorMap(void);
	
	/* Methods: */
	ColorMap& operator=(const ColorMap& source); // Assignment operator
	ColorMap& operator=(ColorMap&& source); // Move assignment operator
	ColorMap& setEntry(unsigned int index,double key,const Color& color); // Sets the color map entry of the given index
	ColorMap& insertEntry(unsigned int index,double key,const Color& color); // Inserts a new color map entry before the given index
	ColorMap& removeEntry(unsigned int index); // Removes the color map entry at the given index
	ColorMap& scaleRange(double minValue,double maxValue); // Scales the color map key range uniformly to the given interval
	ColorMap& setRange(double minValue,double maxValue); // Sets the color map's key range to the given interval by extending and/or clipping the entry array
	unsigned int getNumEntries(void) const // Returns the number of color map entries
		{
		return numEntries;
		}
	const Entry& getEntry(unsigned int index) const // Returns the color map entry of the given index
		{
		return entries[index];
		}
	double getKey(unsigned int index) const // Returns the key of the color map entry of the given index
		{
		return entries[index].key;
		}
	const Color& getColor(unsigned int index) const // Returns the color of the color map entry of the given index
		{
		return entries[index].color;
		}
	Color map(double value) const // Returns the color mapped to the given scalar value
		{
		/* Check the given value against the color map's range: */
		if(value<=entries[0].key)
			return entries[0].color;
		else if(value>=entries[numEntries-1].key)
			return entries[numEntries-1].color;
		else
			{
			/* Find the index of the key interval containing the given scalar value via binary search: */
			unsigned int l=0;
			unsigned int r=numEntries;
			while(r-l>1)
				{
				unsigned int m=(l+r)/2;
				if(entries[m].key<=value)
					l=m;
				else
					r=m;
				}
			
			/* Blend the colors associated with the ends of the found key interval: */
			return blend(entries[l].color,entries[l+1].color,(value-entries[l].key)/(entries[l+1].key-entries[l].key));
			}
		}
	};

}

#endif
