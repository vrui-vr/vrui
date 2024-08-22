/***********************************************************************
ImageView - Class representing a Vulkan image view.
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

#include <Vulkan/ImageView.h>

#include <Vulkan/Common.h>
#include <Vulkan/Device.h>
#include <Vulkan/Image.h>

namespace Vulkan {

/**************************
Methods of class ImageView:
**************************/

void ImageView::createImageView(VkImage image,VkFormat imageFormat)
	{
	/* Set up the image view creation structure: */
	VkImageViewCreateInfo imageViewCreateInfo;
	imageViewCreateInfo.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.pNext=0;
	imageViewCreateInfo.flags=0x0;
	imageViewCreateInfo.image=image;
	imageViewCreateInfo.viewType=VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format=imageFormat;
	imageViewCreateInfo.components.r=VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.g=VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.b=VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.a=VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel=0;
	imageViewCreateInfo.subresourceRange.levelCount=1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer=0;
	imageViewCreateInfo.subresourceRange.layerCount=1;
	
	/* Create the image view: */
	throwOnError(vkCreateImageView(device.getHandle(),&imageViewCreateInfo,0,&imageView),
	             __PRETTY_FUNCTION__,"create Vulkan image view");
	}

ImageView::ImageView(Device& sDevice,VkImage image,VkFormat imageFormat)
	:DeviceAttached(sDevice),
	 imageView(VK_NULL_HANDLE)
	{
	/* Create the image view: */
	createImageView(image,imageFormat);
	}

ImageView::ImageView(Image& image,VkFormat imageFormat)
	:DeviceAttached(image.getDevice()),
	 imageView(VK_NULL_HANDLE)
	{
	/* Create the image view: */
	createImageView(image.getHandle(),imageFormat);
	}

ImageView::ImageView(ImageView&& source)
	:DeviceAttached(std::move(source)),
	 imageView(source.imageView)
	{
	/* Invalidate the source: */
	source.imageView=VK_NULL_HANDLE;
	}

ImageView::~ImageView(void)
	{
	/* Destroy the image view: */
	vkDestroyImageView(device.getHandle(),imageView,0);
	}

}
