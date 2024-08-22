/***********************************************************************
ColorMap - Templatized class to map scalar values within a defined range
to colors.
Copyright (c) 2023 Oliver Kreylos

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

namespace Misc {

template <class ColorParam>
class ColorMap
	{
	/* Embedded classes: */
	public:
	typedef ColorParam Color; // Type of mapped colors
	
	/* Elements: */
	private:
	unsigned int numKeys; // Number of key scalar values
	double* keys; // Array of key scalar values
	Color* keyColors; // Array of colors mapped to the key scalar values
	
	/* Constructors and destructors: */
	public:
	ColorMap(unsigned int sNumKeys,const double* sKeys =0,const Color* sKeyColors =0); // Creates a color map for the given number of keys; initializes color map arrays if source arrays are given
	ColorMap(const ColorMap& source); // Copy constructor
	ColorMap(ColorMap&& source); // Move constructor
	~ColorMap(void);
	
	/* Methods: */
	ColorMap& operator=(const ColorMap& source); // Assignment operator
	ColorMap& operator=(ColorMap&& source); // Move assignment operator
	ColorMap& setNumKeys(unsigned int newNumKeys); // Sets the color map's number of keys; invalidates color map
	ColorMap& setKey(unsigned int index,double key,const Color& keyColor); // Sets the given key/color pair
	unsigned int getNumKeys(void) const // Returns the number of key values
		{
		return numKeys;
		}
	double getKey(unsigned int index) const // Returns the key value of the given index
		{
		return keys[index];
		}
	const Color& getKeyColor(unsigned int index) const // Returns the color associated with the key of the given index
		{
		return keyColors[index];
		}
	Color map(double value) const // Returns the color mapped to the given scalar value
		{
		/* Check the given value against the color map's range: */
		if(value<=keys[0])
			return keyColors[0];
		else if(value>=keys[numKeys-1])
			return keyColors[numKeys-1];
		else
			{
			/* Find the index of the key interval containing the given scalar value via binary search: */
			unsigned int l=0;
			unsigned int r=numKeys;
			while(r-l>1)
				{
				unsigned int m=(l+r)/2;
				if(keys[m]<=value)
					l=m;
				else
					r=m;
				}
			
			/* Blend the colors associated with the ends of the found key interval: */
			return blend(keyColors[l],keyColors[l+1],(value-keys[l])/(keys[l+1]-keys[l]));
			}
		}
	};

}

#endif
