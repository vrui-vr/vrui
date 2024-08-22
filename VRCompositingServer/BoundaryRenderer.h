/***********************************************************************
BoundaryRenderer - Module to render a physical VR environment's
boundaries into a pre-distortion frame buffer.
Copyright (c) 2023 Oliver Kreylos

This file is part of the Vrui VR Compositing Server (VRCompositor).

The Vrui VR Compositing Server is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Vrui VR Compositing Server is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vrui VR Compositing Server; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef BOUNDARYRENDERER_INCLUDED
#define BOUNDARYRENDERER_INCLUDED

#include <vector>
#include <Misc/SizedTypes.h>
#include <Geometry/Point.h>
#include <vulkan/vulkan.h>
#include <Vulkan/ShaderModule.h>
#include <Vulkan/DescriptorSetLayout.h>
#include <Vulkan/PipelineLayout.h>
#include <Vulkan/GraphicsPipeline.h>
#include <Vulkan/RenderPass.h>
#include <Vulkan/Buffer.h>
#include <Vulkan/Framebuffer.h>
#include <Vrui/Internal/VRDeviceState.h>
#include <Vrui/Internal/VRCompositorProtocol.h>

/* Forward declarations: */
namespace IO {
class Directory;
}
namespace Vulkan {
class Device;
class MemoryAllocator;
class ImageView;
class CommandPool;
class CommandBuffer;
}
namespace Vrui {
class EnvironmentDefinition;
}

class BoundaryRenderer
	{
	/* Embedded classes: */
	public:
	typedef Misc::Float32 Scalar; // Scalar type
	typedef Geometry::Point<Scalar,3> Point; // Point type
	
	/* Elements: */
	private:
	Vulkan::Device& device; // Logical device on which the rendering takes place
	Vulkan::DescriptorSetLayout descriptorSetLayout; // Descriptor set layout for the graphics pipeline
	Vulkan::PipelineLayout pipelineLayout; // Layout for the graphics pipeline
	Vulkan::RenderPass renderPass; // Render pass to render the boundary model to one of the compositor's input images
	Vulkan::GraphicsPipeline graphicsPipeline; // Graphics pipeline for boundary rendering
	std::vector<Point> vertices; // List of boundary model vertices
	Vulkan::Buffer vertexBuffer; // Buffer containing boundary model vertices
	std::vector<Vulkan::Framebuffer> framebuffers; // List of frame buffers for the compositor's input images
	
	/* Constructors and destructors: */
	public:
	BoundaryRenderer(Vulkan::Device& sDevice,Vulkan::MemoryAllocator& allocator,Vulkan::CommandPool& commandPool,VkFormat imageFormat,IO::Directory& shaderDir,const VkExtent2D& imageSize,std::vector<Vulkan::ImageView>& imageViews,const Vrui::EnvironmentDefinition& environmentDefinition);
	~BoundaryRenderer(void);
	
	/* Methods: */
	void updateEnvironmentDefinition(Vulkan::Device& sDevice,Vulkan::MemoryAllocator& allocator,Vulkan::CommandPool& commandPool,const Vrui::EnvironmentDefinition& newEnvironmentDefinition);
	void render(const Vrui::VRCompositorProtocol::HMDConfiguration& hmdConfiguration,const Vrui::TrackerState& headTrackerState,int imageIndex,Vulkan::CommandBuffer& commandBuffer);
	};

#endif
