/***********************************************************************
GeoTIFF - Definitions of important GeoTIFF TIFF tags and GeoTIFF GeoKeys
and related codes.
Numerical tag, key, and code values from GeoTIFF specification at
http://web.archive.org/web/20160814115308/http://www.remotesensing.org:80/geotiff/spec/geotiffhome.html
Copyright (c) 2018-2022 Oliver Kreylos

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

#include <Images/GeoTIFF.h>

#include <tiffio.h>

namespace Images {

class GeoTIFFExtender // Class to hook a tag extender for GeoTIFF tags into the TIFF library upon program loading
	{
	/* Elements: */
	private:
	static GeoTIFFExtender theTiffExtender; // Static TIFF tag extender object
	static TIFFExtendProc parentTagExtender; // Pointer to a previously-registered tag extender
	
	/* Private methods: */
	static void tagExtender(TIFF* tiff) // The tag extender function
		{
		/* Define the GeoTIFF and related GDAL tags: */
		static const TIFFFieldInfo geotiffFieldInfo[]=
			{
			{TIFFTAG_GEOPIXELSCALE,-1,-1,TIFF_DOUBLE,FIELD_CUSTOM,true,true,(char*)"GeoPixelScale"},
			{TIFFTAG_GEOTRANSMATRIX,-1,-1,TIFF_DOUBLE,FIELD_CUSTOM,true,true,(char*)"GeoTransformationMatrix"},
			{TIFFTAG_GEOTIEPOINTS,-1,-1,TIFF_DOUBLE,FIELD_CUSTOM,true,true,(char*)"GeoTiePoints"},
			{TIFFTAG_GEOKEYDIRECTORY,-1,-1,TIFF_SHORT,FIELD_CUSTOM,true,true,(char*)"GeoKeyDirectory"},
			{TIFFTAG_GEODOUBLEPARAMS,-1,-1,TIFF_DOUBLE,FIELD_CUSTOM,true,true,(char*)"GeoDoubleParams"},
			{TIFFTAG_GEOASCIIPARAMS,-1,-1,TIFF_ASCII,FIELD_CUSTOM,true,false,(char*)"GeoASCIIParams"},
			{TIFFTAG_GDAL_METADATA,-1,-1,TIFF_ASCII,FIELD_CUSTOM,true,false,(char*)"GDALMetadataValue"},
			{TIFFTAG_GDAL_NODATA,-1,-1,TIFF_ASCII,FIELD_CUSTOM,true,false,(char*)"GDALNoDataValue"}
			};
		
		/* Merge them into the new TIFF file's tag directory: */
		TIFFMergeFieldInfo(tiff,geotiffFieldInfo,sizeof(geotiffFieldInfo)/sizeof(TIFFFieldInfo));
		
		/* Call the parent tag extender, if there was one: */
		if(parentTagExtender!=0)
			(*parentTagExtender)(tiff);
		}
	
	/* Constructors and destructors: */
	public:
	GeoTIFFExtender(void)
		{
		/* Register this class's TIFF tag extender and remember the previous one: */
		parentTagExtender=TIFFSetTagExtender(tagExtender);
		}
	};

GeoTIFFExtender GeoTIFFExtender::theTiffExtender;
TIFFExtendProc GeoTIFFExtender::parentTagExtender=0;

}
