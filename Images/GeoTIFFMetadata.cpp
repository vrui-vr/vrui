/***********************************************************************
GeoTIFFMetadata - Class representing common image metadata provided by
the GeoTIFF library.
Copyright (c) 2011-2024 Oliver Kreylos

This file is part of the Image Handling Library (Images).

The Image Handling Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Image Handling Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Image Handling Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <Images/GeoTIFFMetadata.h>

#include <stdint.h>
#include <stdlib.h>
#include <tiffio.h>
#include <Images/GeoTIFF.h>

namespace Images {

bool readGeoTIFFMetadata(TIFF* tiff,GeoTIFFMetadata& metadata)
	{
	/* Invalid the metadata structure: */
	metadata.haveMap=false;
	metadata.haveDim=false;
	metadata.haveNoData=false;
	
	/* Query GeoTIFF tags: */
	uint16_t elemCount=0;
	double* dElems=0;
	char* cElems=0;
	if(TIFFGetField(tiff,TIFFTAG_GEOPIXELSCALE,&elemCount,&dElems)&&elemCount>=2)
		{
		metadata.haveDim=true;
		metadata.dim[0]=dElems[0];
		metadata.dim[1]=dElems[1];
		}
	if(TIFFGetField(tiff,TIFFTAG_GEOTIEPOINTS,&elemCount,&dElems)&&elemCount==6&&dElems[0]==0.0&&dElems[1]==0.0&&dElems[2]==0.0&&dElems[5]==0.0)
		{
		/* Assume point pixels for now: */
		metadata.haveMap=true;
		metadata.map[0]=dElems[3];
		metadata.map[1]=dElems[4];
		}
	if(TIFFGetField(tiff,TIFFTAG_GEOTRANSMATRIX,&elemCount,&dElems)&&elemCount==16)
		{
		/* Assume point pixels for now: */
		metadata.haveMap=true;
		metadata.map[0]=dElems[3];
		metadata.map[1]=dElems[7];
		metadata.haveDim=true;
		metadata.dim[0]=dElems[0];
		metadata.dim[1]=dElems[5];
		}
	if(TIFFGetField(tiff,TIFFTAG_GDAL_NODATA,&cElems))
		{
		metadata.haveNoData=true;
		metadata.noData=atof(cElems);
		}
	
	/* Extract and parse GeoTIFF key directory: */
	bool pixelIsArea=true; // GeoTIFF pixels are area by default
	uint16_t numKeys=0;
	uint16_t* keys=0;
	if(TIFFGetField(tiff,TIFFTAG_GEOKEYDIRECTORY,&numKeys,&keys)&&numKeys>=4)
		{
		uint16_t numDoubleParams=0;
		double* doubleParams=0;
		if(!TIFFGetField(tiff,TIFFTAG_GEODOUBLEPARAMS,&numDoubleParams,&doubleParams))
			doubleParams=0;
		char* asciiParams=0;
		if(!TIFFGetField(tiff,TIFFTAG_GEOASCIIPARAMS,&asciiParams))
			asciiParams=0;
		
		/* Parse key directory header: */
		uint16_t* keyPtr=keys;
		numKeys=keyPtr[3]; // Actual number of keys is in the header
		keyPtr+=4;
		
		/* Parse all keys: */
		for(uint16_t key=0;key<numKeys;++key,keyPtr+=4)
			{
			/* Check for relevant keys: */
			if(keyPtr[0]==GEOTIFFKEY_RASTERTYPE&&keyPtr[1]==0&&keyPtr[3]==GEOTIFFCODE_RASTERPIXELISPOINT)
				pixelIsArea=false;
			}
		}
	
	/* Check if the metadata's map must be adjusted: */
	if(pixelIsArea&&metadata.haveMap&&metadata.haveDim)
		{
		metadata.map[0]+=metadata.dim[0]*0.5;
		metadata.map[1]+=metadata.dim[1]*0.5;
		}
	
	return true;
	}

bool writeGeoTIFFMetadata(TIFF* tiff,const GeoTIFFMetadata& metadata)
	{
	bool result=true;
	
	/* Write map coordinates if defined: */
	if(metadata.haveMap)
		{
		double tiePoints[6];
		tiePoints[2]=tiePoints[1]=tiePoints[0]=0.0;
		for(int i=0;i<2;++i)
			{
			tiePoints[3+i]=metadata.map[i];
			if(metadata.haveDim)
				tiePoints[3+i]-=metadata.dim[i]*0.5; // Adjust map coordinates because GeoTIFF pixels are cell-centered by default
			}
		tiePoints[5]=0.0;
		result=result&&TIFFSetField(tiff,TIFFTAG_GEOTIEPOINTS,6,tiePoints)!=0;
		}
	
	/* Write pixel dimensions if defined: */
	if(metadata.haveDim)
		result=result&&TIFFSetField(tiff,TIFFTAG_GEOPIXELSCALE,2,metadata.dim)!=0;
	
	/* Write NODATA value if defined: */
	if(metadata.haveNoData)
		{
		char noData[64];
		snprintf(noData,sizeof(noData),"%g",metadata.noData);
		result=result&&TIFFSetField(tiff,TIFFTAG_GDAL_NODATA,noData)!=0;
		}
	
	return result;
	}

}
