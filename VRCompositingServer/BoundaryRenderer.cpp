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

#include "BoundaryRenderer.h"

#include <utility>
#include <vector>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <IO/OpenFile.h>
#include <Math/Math.h>
#include <Geometry/PCACalculator.h>
#include <Geometry/Vector.h>
#include <Geometry/Plane.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/ProjectiveTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#include <Vulkan/Device.h>
#include <Vulkan/GraphicsPipeline.h>
#include <Vulkan/ImageView.h>
#include <Vulkan/CommandPool.h>
#include <Vulkan/CommandBuffer.h>
#include <Vrui/EnvironmentDefinition.h>

/*********************************
Methods of class BoundaryRenderer:
*********************************/

namespace {

/**************
Helper classes:
**************/

struct RenderState // Structure holding rendering state uploaded to the graphics pipeline as a push constant
	{
	/* Elements: */
	public:
	BoundaryRenderer::Scalar pmv[4][4]; // Combined projection/modelview matrix
	BoundaryRenderer::Scalar color[4]; // Line rendering color
	};

/****************
Helper functions:
****************/

Vulkan::DescriptorSetLayout createDescriptorSetLayout(Vulkan::Device& device)
	{
	/* Create a descriptor set layout constructor: */
	Vulkan::DescriptorSetLayout::Constructor dslc;
	
	/* Create and return the descriptor set layout: */
	return Vulkan::DescriptorSetLayout(device,dslc);
	}

Vulkan::PipelineLayout createPipelineLayout(Vulkan::Device& device,Vulkan::DescriptorSetLayout& descriptorSetLayout)
	{
	/* Create a pipeline layout constructor: */
	Vulkan::PipelineLayout::Constructor plc;
	
	/* Add the descriptor set layout to the pipeline layout: */
	plc.addDescriptorSetLayout(descriptorSetLayout.getHandle());
	
	/* Add a push constant range to upload rendering state: */
	VkPushConstantRange pushConstantRange;
	pushConstantRange.offset=0;
	pushConstantRange.size=sizeof(RenderState);
	pushConstantRange.stageFlags=VK_SHADER_STAGE_VERTEX_BIT;
	plc.addPushConstantRange(pushConstantRange);
	
	/* Create and return the pipeline layout: */
	return Vulkan::PipelineLayout(device,plc);
	}

Vulkan::RenderPass createRenderPass(Vulkan::Device& device,VkFormat imageFormat)
	{
	/* Create a render pass: */
	Vulkan::RenderPass::Constructor rpc;
	
	/* Add a color attachment to the render pass: */
	VkAttachmentDescription colorAttachment;
	colorAttachment.flags=0x0;
	colorAttachment.format=imageFormat;
	colorAttachment.samples=VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // The output from this render pass will be input to the distortion correction shader
	rpc.addAttachment(colorAttachment);
	
	/* Add a subpass to the render pass: */
	VkAttachmentReference subpassAttachment;
	subpassAttachment.attachment=0;
	subpassAttachment.layout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkSubpassDescription subpass;
	subpass.flags=0x0;
	subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.inputAttachmentCount=0;
	subpass.pInputAttachments=0;
	subpass.colorAttachmentCount=1;
	subpass.pColorAttachments=&subpassAttachment;
	subpass.pResolveAttachments=0;
	subpass.pDepthStencilAttachment=0;
	subpass.preserveAttachmentCount=0;
	subpass.pPreserveAttachments=0;
	rpc.addSubpass(subpass);
	
	return Vulkan::RenderPass(device,rpc);
	}

Vulkan::GraphicsPipeline createGraphicsPipeline(Vulkan::Device& device,IO::Directory& shaderDir,Vulkan::PipelineLayout& pipelineLayout,Vulkan::RenderPass& renderPass)
	{
	/* Create a graphics pipeline constructor: */
	Vulkan::GraphicsPipeline::Constructor gpc;
	
	/* Add the boundary rendering shaders to the pipeline: */
	Vulkan::ShaderModule vertexShader(device,shaderDir,Vulkan::ShaderModule::Vertex,"BoundaryRenderer");
	gpc.addShaderStage(vertexShader);
	Vulkan::ShaderModule fragmentShader(device,shaderDir,Vulkan::ShaderModule::Fragment,"BoundaryRenderer");
	gpc.addShaderStage(fragmentShader);
	
	/* Add the input vertex binding description: */
	gpc.addVertexInputBinding(0,sizeof(BoundaryRenderer::Point),VK_VERTEX_INPUT_RATE_VERTEX);
	
	/* Add attribute descriptions for each vertex component: */
	gpc.addVertexInputAttribute(0,0,VK_FORMAT_R32G32B32_SFLOAT,0);
	
	/* Set up input assembly: */
	gpc.setInputAssemblyPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
	gpc.setInputAssemblyPrimitiveRestart(false);
	
	/* Set up the rasterizer: */
	VkPipelineRasterizationStateCreateInfo& rs=gpc.getRasterizationState();
	rs.depthClampEnable=VK_FALSE;
	rs.rasterizerDiscardEnable=VK_FALSE;
	rs.polygonMode=VK_POLYGON_MODE_FILL;
	rs.cullMode=VK_CULL_MODE_BACK_BIT;
	rs.frontFace=VK_FRONT_FACE_CLOCKWISE;
	rs.depthBiasEnable=VK_FALSE;
	rs.depthBiasConstantFactor=0.0f;
	rs.depthBiasClamp=0.0f;
	rs.depthBiasSlopeFactor=0.0f;
	rs.lineWidth=1.0f;
	
	/* Set up multisampling: */
	VkPipelineMultisampleStateCreateInfo& ms=gpc.getMultisampleState();
	ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
	ms.sampleShadingEnable=VK_FALSE;
	ms.minSampleShading=1.0f;
	ms.pSampleMask=0;
	ms.alphaToCoverageEnable=VK_FALSE;
	ms.alphaToOneEnable=VK_FALSE;
	
	/* Set up color blending: */
	VkPipelineColorBlendAttachmentState cba;
	cba.blendEnable=VK_FALSE;
	cba.srcColorBlendFactor=VK_BLEND_FACTOR_ONE;
	cba.dstColorBlendFactor=VK_BLEND_FACTOR_ZERO;
	cba.colorBlendOp=VK_BLEND_OP_ADD;
	cba.srcAlphaBlendFactor=VK_BLEND_FACTOR_ONE;
	cba.dstAlphaBlendFactor=VK_BLEND_FACTOR_ZERO;
	cba.alphaBlendOp=VK_BLEND_OP_ADD;
	cba.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
	gpc.addColorBlendAttachment(cba);
	VkPipelineColorBlendStateCreateInfo& cbs=gpc.getColorBlendState();
	cbs.logicOpEnable=VK_FALSE;
	cbs.logicOp=VK_LOGIC_OP_COPY;
	for(int i=0;i<4;++i)
		cbs.blendConstants[i]=0.0f;
	
	/* Enable dynamic viewports and scissor rectangles: */
	gpc.addDynamicViewports(1);
	gpc.addDynamicScissors(1);
	
	/* Create and return the graphics pipeline: */
	return Vulkan::GraphicsPipeline(device,gpc,pipelineLayout,renderPass,0);
	}

std::vector<BoundaryRenderer::Point> createBoundaryModel(const Vrui::EnvironmentDefinition& environmentDefinition)
	{
	/* Access the environment definition's boundary polygons: */
	typedef Vrui::EnvironmentDefinition::Polygon Polygon;
	typedef Vrui::EnvironmentDefinition::PolygonList PolygonList;
	const PolygonList& boundary=environmentDefinition.boundary;
	
	/* Determine the maximum number of vertices of all polygons: */
	size_t maxNumVertices=0;
	for(PolygonList::const_iterator bIt=boundary.begin();bIt!=boundary.end();++bIt)
		maxNumVertices=Math::max(maxNumVertices,bIt->size());
	
	/* Create vertices and line segments for all boundary polygons: */
	Vrui::Scalar lineSpacing=Vrui::Scalar(0.25)*environmentDefinition.getMeterFactor();
	std::vector<BoundaryRenderer::Point> vertices;
	Vrui::Point* intersections=new Vrui::Point[maxNumVertices];
	for(PolygonList::const_iterator bIt=boundary.begin();bIt!=boundary.end();++bIt)
		{
		const Polygon& bp=*bIt;
		
		/* Calculate the polygon's centroid and normal vector via PCA: */
		Geometry::PCACalculator<3> pca;
		for(Polygon::const_iterator vIt=bp.begin();vIt!=bp.end();++vIt)
			pca.accumulatePoint(*vIt);
		pca.calcCovariance();
		double evs[3];
		pca.calcEigenvalues(evs);
		Vrui::Plane plane(pca.calcEigenvector(evs[2]),pca.calcCentroid());
		
		/* Project all polygon vertices into the plane: */
		Polygon pbp;
		pbp.reserve(bp.size());
		for(Polygon::const_iterator vIt=bp.begin();vIt!=bp.end();++vIt)
			pbp.push_back(plane.project(*vIt));
		
		/* Determine the primary axes most aligned with the polygon's plane: */
		int normalAxis=Geometry::findParallelAxis(plane.getNormal());
		int axis0=(normalAxis+1)%3;
		int axis1=(normalAxis+2)%3;
		
		/* Create line segments for the polygon's edges: */
		Polygon::iterator v0It=pbp.end()-1;
		for(Polygon::iterator v1It=pbp.begin();v1It!=pbp.end();v0It=v1It,++v1It)
			{
			vertices.push_back(BoundaryRenderer::Point(*v0It));
			vertices.push_back(BoundaryRenderer::Point(*v1It));
			}
		
		/* Create grid lines inside the polygon: */
		for(int axisIndex=0;axisIndex<2;++axisIndex)
			{
			/* Determine the primary axis with which to align this set of grid lines: */
			int axis=axisIndex==0?axis0:axis1;
			int sort=axisIndex==0?axis1:axis0;
			
			/* Calculate the extent of the polygon along the selected primary axis: */
			Vrui::Scalar min,max;
			max=min=pbp.front()[axis];
			for(Polygon::iterator vIt=pbp.begin()+1;vIt!=pbp.end();++vIt)
				{
				min=Math::min(min,(*vIt)[axis]);
				max=Math::max(max,(*vIt)[axis]);
				}
			
			/* Intersect the polygon with a sequence of planes aligned to a primary axis: */
			#if 1
			Vrui::Scalar step=Math::sqrt(Vrui::Scalar(1)-Math::sqr(plane.getNormal()[axis]))*lineSpacing;
			int numLines=Math::ceil((max-min)/step)-1;
			Vrui::Scalar level=min+((max-min)-step*Vrui::Scalar(numLines-1))*Vrui::Scalar(0.5);
			for(int line=0;line<numLines;++line,level+=step)
			#else
			for(Vrui::Scalar level=(Math::floor(min/lineSpacing)+Vrui::Scalar(1))*lineSpacing;level<max;level+=lineSpacing)
			#endif
				{
				/* Find all intersections between the grid plane and the polygon's edges: */
				size_t numIntersections=0;
				Polygon::iterator v0It=pbp.end()-1;
				for(Polygon::iterator v1It=pbp.begin();v1It!=pbp.end();v0It=v1It,++v1It)
					{
					if(((*v0It)[axis]<=level&&(*v1It)[axis]>level)||((*v1It)[axis]<=level&&(*v0It)[axis]>level)) // Edge crosses plane
						{
						/* Calculate the intersection point: */
						intersections[numIntersections]=Geometry::affineCombination(*v0It,*v1It,(level-(*v0It)[axis])/((*v1It)[axis]-(*v0It)[axis]));
						++numIntersections;
						
						/* Sort the array of intersections by position along the sorting axis: */
						for(size_t i=numIntersections-1;i>0&&intersections[i][sort]<intersections[i-1][sort];--i)
							std::swap(intersections[i-1],intersections[i]);
						}
					}
				
				/* Number of intersections is always even; draw line segments between all pairs: */
				for(size_t i=0;i<numIntersections;++i)
					vertices.push_back(BoundaryRenderer::Point(intersections[i]));
				}
			}
		}
	delete[] intersections;
	
	return vertices;
	}

Vulkan::Buffer createVertexBuffer(Vulkan::Device& device,Vulkan::MemoryAllocator& allocator,Vulkan::CommandPool& commandPool,const std::vector<BoundaryRenderer::Point>& vertices)
	{
	/* Upload the boundary model vertices into a temporary staging buffer: */
	VkDeviceSize bufferSize(vertices.size()*sizeof(BoundaryRenderer::Point));
	Vulkan::Buffer stagingBuffer(device,bufferSize,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,allocator,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,false);
	BoundaryRenderer::Point* vPtr=static_cast<BoundaryRenderer::Point*>(stagingBuffer.map(0x0));
	for(std::vector<BoundaryRenderer::Point>::const_iterator vIt=vertices.begin();vIt!=vertices.end();++vIt,++vPtr)
		*vPtr=*vIt;
	stagingBuffer.unmap();
	
	/* Copy the boundary model from the staging buffer into the permanent buffer: */
	Vulkan::Buffer vertexBuffer(device,bufferSize,VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,allocator,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,false);
	vertexBuffer.copy(0,stagingBuffer,0,bufferSize,commandPool);
	
	return vertexBuffer;
	}

}

BoundaryRenderer::BoundaryRenderer(Vulkan::Device& sDevice,Vulkan::MemoryAllocator& allocator,Vulkan::CommandPool& commandPool,VkFormat imageFormat,IO::Directory& shaderDir,const VkExtent2D& imageSize,std::vector<Vulkan::ImageView>& imageViews,const Vrui::EnvironmentDefinition& environmentDefinition)
	:device(sDevice),
	 descriptorSetLayout(createDescriptorSetLayout(device)),
	 pipelineLayout(createPipelineLayout(device,descriptorSetLayout)),
	 renderPass(createRenderPass(device,imageFormat)),
	 graphicsPipeline(createGraphicsPipeline(device,shaderDir,pipelineLayout,renderPass)),
	 vertices(createBoundaryModel(environmentDefinition)),
	 vertexBuffer(createVertexBuffer(device,allocator,commandPool,vertices))
	{
	/* Create framebuffers for the compositor's input images: */
	framebuffers.reserve(imageViews.size());
	for(std::vector<Vulkan::ImageView>::iterator ivIt=imageViews.begin();ivIt!=imageViews.end();++ivIt)
		{
		std::vector<VkImageView> attachments;
		attachments.push_back(ivIt->getHandle());
		framebuffers.push_back(Vulkan::Framebuffer(device,renderPass,attachments,imageSize,1));
		}
	}

BoundaryRenderer::~BoundaryRenderer(void)
	{
	}

void BoundaryRenderer::updateEnvironmentDefinition(Vulkan::Device& sDevice,Vulkan::MemoryAllocator& allocator,Vulkan::CommandPool& commandPool,const Vrui::EnvironmentDefinition& newEnvironmentDefinition)
	{
	/* Create a new boundary model: */
	vertices=createBoundaryModel(newEnvironmentDefinition);
	
	/* Upload the new boundary model into a new vertex buffer: */
	vertexBuffer=createVertexBuffer(device,allocator,commandPool,vertices);
	}

void BoundaryRenderer::render(const Vrui::VRCompositorProtocol::HMDConfiguration& hmdConfiguration,const Vrui::TrackerState& headTrackerState,int imageIndex,Vulkan::CommandBuffer& commandBuffer)
	{
	typedef Geometry::ProjectiveTransformation<double,3> PTransform;
	typedef Geometry::OrthonormalTransformation<double,3> ONTransform;
	
	/* Begin a render pass by clearing the entire framebuffer to black: */
	VkRect2D region;
	region.offset.x=0;
	region.offset.y=0;
	region.extent.width=hmdConfiguration.frameSize[0];
	region.extent.height=hmdConfiguration.frameSize[1];
	std::vector<VkClearValue> clearValues;
	clearValues.reserve(1);
	VkClearValue clear;
	clear.color.float32[0]=0.0f;
	clear.color.float32[1]=0.0f;
	clear.color.float32[2]=0.0f;
	clear.color.float32[3]=1.0f;
	clearValues.push_back(clear);
	commandBuffer.beginRenderPass(renderPass,framebuffers[imageIndex],region,clearValues);
	
	/* Bind the graphics pipeline: */
	commandBuffer.bindPipeline(graphicsPipeline);
	
	/* Bind the vertex buffer to the graphics pipeline: */
	commandBuffer.bindVertexBuffers(0,vertexBuffer,0);
	
	/* Render the boundary model into both eyes of the given input image: */
	for(int eye=0;eye<2;++eye)
		{
		/* Create a render state: */
		RenderState rs;
		
		/* Calculate a projection matrix for the current eye: */
		double near=0.0254;
		double far=1000.0;
		double l=hmdConfiguration.eyeFovs[eye][0];
		double r=hmdConfiguration.eyeFovs[eye][1];
		double b=hmdConfiguration.eyeFovs[eye][2];
		double t=hmdConfiguration.eyeFovs[eye][3];
		PTransform projection=PTransform::identity;
		PTransform::Matrix& p=projection.getMatrix();
		p(0,0)=2.0/(r-l);
		p(0,2)=(r+l)/(r-l);
		p(1,1)=2.0/(t-b);
		p(1,2)=(t+b)/(t-b);
		p(2,2)=near/(far-near);
		p(2,3)=far*near/(far-near);
		p(3,2)=-1.0;
		p(3,3)=0.0;
		
		/* Calculate a modelview matrix from physical space to the current eye: */
		ONTransform modelview=ONTransform(headTrackerState);
		modelview*=ONTransform::translateFromOriginTo(hmdConfiguration.eyePositions[eye]);
		modelview*=ONTransform::rotate(hmdConfiguration.eyeRotations[eye]);
		modelview.doInvert();
		modelview.renormalize();
		
		/* Write the combined projection into the render state's column-major matrix: */
		PTransform pmv=projection;
		pmv*=modelview;
		for(int i=0;i<4;++i)
			for(int j=0;j<4;++j)
				rs.pmv[i][j]=pmv.getMatrix()(j,i);
		
		/* Draw the boundary lines in green: */
		rs.color[0]=0.0f;
		rs.color[1]=0.5f;
		rs.color[2]=0.0f;
		rs.color[3]=1.0f;
		
		/* Push the render state to the graphics pipeline: */
		commandBuffer.pushConstants(pipelineLayout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(RenderState),&rs);
		
		/* Set the viewport and scissor rectangle: */
		commandBuffer.setViewport(0,hmdConfiguration.eyeRects[eye],0.0f,1.0f);
		commandBuffer.setScissor(0,hmdConfiguration.eyeRects[eye]);
		
		/* Draw the boundary model: */
		commandBuffer.draw(uint32_t(vertices.size()),1,0,0);
		}
	
	/* End the render pass: */
	commandBuffer.endRenderPass();
	}
