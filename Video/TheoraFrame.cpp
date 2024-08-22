/***********************************************************************
TheoraFrame - Wrapper class for th_img_plane structure.
Copyright (c) 2010-2022 Oliver Kreylos

This file is part of the Basic Video Library (Video).

The Basic Video Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The Basic Video Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Basic Video Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <Video/TheoraFrame.h>

#include <string.h>
#include <Misc/Size.h>
#include <Video/TheoraInfo.h>

namespace Video {

/****************************
Methods of class TheoraFrame:
****************************/

TheoraFrame::TheoraFrame(void)
	:privateData(false)
	{
	/* Reset all planes: */
	for(int i=0;i<3;++i)
		{
		planes[i].width=planes[i].height=0;
		planes[i].stride=0;
		planes[i].data=0;
		offsets[i]=0;
		}
	}

TheoraFrame::~TheoraFrame(void)
	{
	if(privateData)
		delete[] planes[0].data;
	}

void TheoraFrame::copy(const TheoraFrame& source)
	{
	/* Copy each of the three image planes: */
	for(int planeIndex=0;planeIndex<3;++planeIndex)
		{
		const unsigned char* sRowPtr=source.planes[planeIndex].data;
		unsigned char* dRowPtr=planes[planeIndex].data;
		for(int y=0;y<planes[planeIndex].height;++y)
			{
			/* Copy the pixel row: */
			memcpy(dRowPtr,sRowPtr,planes[planeIndex].width);
			
			/* Go to the next pixel row: */
			sRowPtr+=source.planes[planeIndex].stride;
			dRowPtr+=planes[planeIndex].stride;
			}
		}
	}

void TheoraFrame::init420(const TheoraInfo& info)
	{
	/* Take the size of the Y' component from the info structure, and reduce to calculate chroma components: */
	Misc::Size<2> ySize(info.frame_width,info.frame_height);
	Misc::Size<2> cSize(ySize[0]>>1,ySize[1]>>1);
	
	/* Release previously allocated memory: */
	if(privateData)
		delete[] planes[0].data;
	
	/* Create an interleaved memory layout: */
	privateData=true;
	planes[0].width=ySize[0];
	planes[0].height=ySize[1];
	planes[0].stride=(ySize[0]*3)/2;
	planes[0].data=new unsigned char[(ySize.volume()*3)/2];
	planes[1].width=cSize[0];
	planes[1].height=cSize[1];
	planes[1].stride=ySize[0]*3;
	planes[1].data=planes[0].data+ySize[0];
	planes[2].width=cSize[0];
	planes[2].height=cSize[1];
	planes[2].stride=ySize[0]*3;
	planes[2].data=planes[0].data+(ySize[0]*5)/2;
	
	/* Calculate frame offsets: */
	offsets[0]=info.pic_y*planes[0].stride+info.pic_x;
	for(int i=1;i<3;++i)
		offsets[i]=(info.pic_y>>1)*planes[i].stride+(info.pic_x>>1);
	}

void TheoraFrame::init422(const TheoraInfo& info)
	{
	/* Take the size of the Y' component from the info structure, and reduce horizontally to calculate chroma components: */
	Misc::Size<2> ySize(info.frame_width,info.frame_height);
	Misc::Size<2> cSize(ySize[0]>>1,ySize[1]);
	
	/* Release previously allocated memory: */
	if(privateData)
		delete[] planes[0].data;
	
	/* Create an interleaved memory layout: */
	privateData=true;
	planes[0].width=ySize[0];
	planes[0].height=ySize[1];
	planes[0].stride=ySize[0]*2;
	planes[0].data=new unsigned char[ySize.volume()*2];
	planes[1].width=cSize[0];
	planes[1].height=cSize[1];
	planes[1].stride=ySize[0]*2;
	planes[1].data=planes[0].data+ySize[0];
	planes[2].width=cSize[0];
	planes[2].height=cSize[1];
	planes[2].stride=ySize[0]*2;
	planes[2].data=planes[0].data+ySize[0]*3/2;
	
	/* Calculate frame offsets: */
	offsets[0]=info.pic_y*planes[0].stride+info.pic_x;
	for(int i=1;i<3;++i)
		offsets[i]=(info.pic_y>>1)*planes[i].stride+info.pic_x;
	}

void TheoraFrame::init444(const TheoraInfo& info)
	{
	/* Take the size of the Y' component from the info structure, and use same size to calculate chroma components: */
	Misc::Size<2> ySize(info.frame_width,info.frame_height);
	Misc::Size<2> cSize=ySize;
	
	/* Release previously allocated memory: */
	if(privateData)
		delete[] planes[0].data;
	
	/* Create an interleaved memory layout: */
	privateData=true;
	planes[0].width=ySize[0];
	planes[0].height=ySize[1];
	planes[0].stride=ySize[0]*3;
	planes[0].data=new unsigned char[ySize.volume()*3];
	planes[1].width=cSize[0];
	planes[1].height=cSize[1];
	planes[1].stride=ySize[0]*3;
	planes[1].data=planes[0].data+ySize[0];
	planes[2].width=cSize[0];
	planes[2].height=cSize[1];
	planes[2].stride=ySize[0]*3;
	planes[2].data=planes[0].data+ySize[0]*2;
	
	/* Calculate frame offsets: */
	for(int i=0;i<3;++i)
		offsets[i]=info.pic_y*planes[i].stride+info.pic_x;
	}

void TheoraFrame::release(void)
	{
	if(privateData)
		delete[] planes[0].data;
	for(int i=0;i<3;++i)
		planes[i].data=0;
	privateData=false;
	}

}
