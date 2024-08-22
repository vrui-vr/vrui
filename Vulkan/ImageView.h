/***********************************************************************
ImageView - Class representing Vulkan image views.
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

#ifndef VULKAN_IMAGEVIEW_INCLUDED
#define VULKAN_IMAGEVIEW_INCLUDED

#include <vulkan/vulkan.h>
#include <Vulkan/DeviceAttached.h>

/* Forward declarations: */
namespace Vulkan {
class Image;
}

namespace Vulkan {

class ImageView:public DeviceAttached
	{
	/* Elements: */
	private:
	VkImageView imageView; // Vulkan image view handle
	
	/* Private methods: */
	void createImageView(VkImage image,VkFormat imageFormat); // Creates the Vulkan image view object
	
	/* Constructors and destructors: */
	public:
	ImageView(Device& sDevice,VkImage image,VkFormat imageFormat); // Creates a default image view for the given Vulkan image handle on the given logical device
	ImageView(Image& image,VkFormat imageFormat); // Creates a default image view for the given image
	ImageView(ImageView&& source); // Move constructor
	virtual ~ImageView(void); // Destroys the image view
	
	/* Methods: */
	VkImageView getHandle(void) const // Returns the Vulkan image view handle
		{
		return imageView;
		}
	};

}

#endif
