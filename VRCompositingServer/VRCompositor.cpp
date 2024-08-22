/***********************************************************************
VRCompositor - Class to display a stream of stereoscopic frames rendered
by a VR application on a VR headset's screen(s).
Copyright (c) 2022-2024 Oliver Kreylos

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

#define PRINT_HMDCONFIG 1

#include "VRCompositor.h"

#include <stdexcept>
#include <iostream>
#include <Misc/SizedTypes.h>
#include <Misc/FunctionCalls.h>
#include <IO/OpenFile.h>
#include <Vulkan/Instance.h>
#include <Vulkan/Device.h>
#include <Vulkan/ImageView.h>
#include <Vulkan/Buffer.h>
#include <Vulkan/GraphicsPipeline.h>
#include <Vulkan/DescriptorSet.h>
#include <Vrui/Internal/HMDConfiguration.h>
#include <Vrui/Internal/VRDeviceState.h>
#include <Vrui/Internal/VRDeviceClient.h>

#include "LatencyTester.h"
#include "BoundaryRenderer.h"

#if PRINT_HMDCONFIG
#include <Misc/OutputOperators.h>
#include <Geometry/OutputOperators.h>
#endif

// DEBUGGING
#include <unistd.h>

namespace {

/**************
Helper classes:
**************/

struct CorrectionMeshVertex // Structure holding a vertex of the lens distortion correction mesh
	{
	/* Embedded classes: */
	public:
	typedef Misc::Float32 Scalar; // Scalar type for vertex positions and texture coordinates
	
	/* Elements: */
	Scalar red[2]; // Texture coordinate for the red channel
	Scalar green[2]; // Texture coordinate for the green channel
	Scalar blue[2]; // Texture coordinate for the blue channel
	Scalar pos[2]; // Vertex position in normalized device coordinates
	
	/* Methods: */
	static void addVertexInputDescriptions(Vulkan::GraphicsPipeline::Constructor& gpc) // Adds binding and attribute descriptions compatible with the vertex type to the given graphics pipeline constructor
		{
		/* Add the binding description: */
		gpc.addVertexInputBinding(0,sizeof(CorrectionMeshVertex),VK_VERTEX_INPUT_RATE_VERTEX);
		
		/* Add attribute descriptions for each mesh vertex component: */
		uint32_t offset=0U;
		for(uint32_t i=0;i<4;++i,offset+=2*sizeof(Scalar))
			gpc.addVertexInputAttribute(i,0,VK_FORMAT_R32G32_SFLOAT,offset);
		}
	};

struct ReprojectionState // Structure holding reprojection state for the distortion correction shader
	{
	/* Elements: */
	public:
	float rotation[3][4]; // The reprojection rotation as a column-major 3x3 matrix, with one padding element per column as of GLSL std430 memory layout
	float viewportOffset[2]; // Offset of the viewport-to-tangent space transformation
	float viewportScale[2]; // Scale of the viewport-to-tangent space transformation
	};

/****************
Helper functions:
****************/

Vulkan::CStringList getDeviceExtensions(void)
	{
	/* Collect the list of required extensions: */
	Vulkan::CStringList result;
	Vulkan::MemoryAllocator::addRequiredDeviceExtensions(result);
	
	return result;
	}

Vulkan::DescriptorPool createDescriptorPool(Vulkan::Device& device)
	{
	/* Create a descriptor pool constructor: */
	Vulkan::DescriptorPool::Constructor dpc;
	
	/* Add one texture samplers for each input image to the descriptor pool: */
	dpc.addDescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,3);
	
	/* Create and return the descriptor pool: */
	return Vulkan::DescriptorPool(device,0x0,3,dpc);
	}

Vulkan::DescriptorSetLayout createDescriptorSetLayout(Vulkan::Device& device,Vulkan::Sampler& sampler)
	{
	/* Create a descriptor set layout constructor: */
	Vulkan::DescriptorSetLayout::Constructor dslc;
	
	/* Add the texture sampler to the descriptor set layout: */
	dslc.addBinding(0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT);
	
	/* Create and return the descriptor set layout: */
	return Vulkan::DescriptorSetLayout(device,dslc);
	}

Vulkan::PipelineLayout createPipelineLayout(Vulkan::Device& device,Vulkan::DescriptorSetLayout& descriptorSetLayout)
	{
	/* Create a pipeline layout constructor: */
	Vulkan::PipelineLayout::Constructor plc;
	
	/* Add the descriptor set layout to the pipeline layout: */
	plc.addDescriptorSetLayout(descriptorSetLayout.getHandle());
	
	/* Add a push constant range to upload reprojection state: */
	VkPushConstantRange pushConstantRange;
	pushConstantRange.offset=0;
	pushConstantRange.size=sizeof(ReprojectionState);
	pushConstantRange.stageFlags=VK_SHADER_STAGE_VERTEX_BIT;
	plc.addPushConstantRange(pushConstantRange);
	
	/* Create and return the pipeline layout: */
	return Vulkan::PipelineLayout(device,plc);
	}

}

/*****************************
Methods of class VRCompositor:
*****************************/

void VRCompositor::updateHmdConfiguration(bool initial)
	{
	typedef CorrectionMeshVertex::Scalar Scalar;
	
	/* Read the VR device server's first HMD configuration: */
	if(vrDeviceClient.getNumHmdConfigurations()<1)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"VR device server does not define any head-mounted display devices");
	vrDeviceClient.lockHmdConfigurations();
	const Vrui::HMDConfiguration& hc=vrDeviceClient.getHmdConfiguration(0);
	
	/* Get the index of the tracker attached to the HMD: */
	headDeviceTrackerIndex=hc.getTrackerIndex();
	
	/* Get the index of the HMD's face detector button: */
	faceDetectorButtonIndex=hc.getFaceDetectorButtonIndex();
	
	/* Write a new HMD configuration into the shared-memory object: */
	HMDConfiguration& hmdConfiguration=sharedMemorySegment->hmdConfiguration.startWrite();
	
	/* Set the pre-distortion frame buffer size: */
	Vrui::ISize rtSize=hc.getRenderTargetSize();
	hmdConfiguration.frameSize=Vrui::ISize(rtSize[0]*2,rtSize[1]);
	
	#if PRINT_HMDCONFIG
	std::cout<<"Pre-distortion frame buffer size: "<<rtSize<<std::endl;
	#endif
	
	/* Set per-eye parameters: */
	for(int eye=0;eye<2;++eye)
		{
		/* Set the pre-distortion per-eye viewport: */
		hmdConfiguration.eyeRects[eye]=Vrui::IRect(Vrui::IOffset(eye==0?0:rtSize[0],0),rtSize);
		
		#if PRINT_HMDCONFIG
		std::cout<<"Pre-distortion viewport for eye "<<eye<<": "<<hmdConfiguration.eyeRects[eye]<<std::endl;
		#endif
		
		/* Set the 3D eye position and rotation: */
		hmdConfiguration.eyePositions[eye]=hc.getEyePosition(eye);
		hmdConfiguration.eyeRotations[eye]=hc.getEyeRotation(eye);
		
		/* Set the tangent-space eye FoV boundaries: */
		for(int i=0;i<4;++i)
			hmdConfiguration.eyeFovs[eye][i]=hc.getFov(eye)[i];
		
		/* Calculate a screen transformation and size to match the Vrui application client's HMD configuration: */
		screenTransforms[eye]=Vrui::ONTransform::translateFromOriginTo(hc.getEyePosition(eye));
		screenTransforms[eye]*=Vrui::ONTransform::rotate(hc.getEyeRotation(eye));
		screenTransforms[eye]*=Vrui::ONTransform::translate(Vrui::Vector(hc.getFov(eye)[0],hc.getFov(eye)[2],-1));
		
		#if PRINT_HMDCONFIG
		std::cout<<"Screen transformation for eye "<<eye<<": "<<screenTransforms[eye]<<std::endl;
		#endif
		
		for(int i=0;i<2;++i)
			screenSizes[eye][i]=hc.getFov(eye)[i*2+1]-hc.getFov(eye)[i*2+0];
		}
	
	#if PRINT_HMDCONFIG
	std::cout<<"Display latency "<<hc.getDisplayLatency()<<" ns"<<std::endl;
	#endif
	
	/* Write the display latency: */
	hmdConfiguration.exposeOffset=Vrui::TimeVector(0,hc.getDisplayLatency());
	// hmdConfiguration.exposeOffset=Vrui::TimeVector(0,16500000); // Experimentally determined for Valve Index at 90Hz
	
	/* Finish writing the HMD configuration: */
	sharedMemorySegment->hmdConfiguration.finishWrite();
	
	/* Get the size of the HMD's display: */
	Vrui::ISize hmdDisplaySize(hmd.getVisibleRegion().width,hmd.getVisibleRegion().height);
	
	#if PRINT_HMDCONFIG
	std::cout<<"HMD display size: "<<hmdDisplaySize<<std::endl;
	#endif
	
	/* Ensure that the union of the HMD configuration's post-distortion per-eye viewports match the HMD's display: */
	Vrui::IRect totalViewport=hc.getViewport(0);
	totalViewport.unite(hc.getViewport(1));
	if(totalViewport.offset!=Vrui::IOffset(0,0)||totalViewport.size!=hmdDisplaySize)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"VR device server's post-distortion eye viewports do not cover HMD's display");
	
	if(initial||distortionMeshVersion!=hc.getDistortionMeshVersion())
		{
		/* Calculate the size of the distortion correction mesh buffer holding the left- and right-eye meshes: */
		Vulkan::Size newCorrectionMeshSize=hc.getDistortionMeshSize();
		VkDeviceSize numVertices=newCorrectionMeshSize.volume();
		VkDeviceSize numIndices=(newCorrectionMeshSize[1]-1)*(newCorrectionMeshSize[0]*2+1);
		VkDeviceSize correctionBufferSize=2*numVertices*sizeof(CorrectionMeshVertex)+2*numIndices*sizeof(Misc::UInt16);
		
		/* Re-allocate the correction mesh buffer if the mesh size has changed: */
		if(correctionMeshSize!=newCorrectionMeshSize)
			{
			/* Replace the permanent correction mesh buffer: */
			delete correctionMeshBuffer;
			correctionMeshSize=newCorrectionMeshSize;
			correctionMeshBuffer=new Vulkan::Buffer(device,correctionBufferSize,VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_INDEX_BUFFER_BIT,allocator,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,false);
			correctionMeshIndexDataOffset=2*numVertices*sizeof(CorrectionMeshVertex);
			}
		
		/* Upload the new correction mesh into a temporary staging buffer: */
		Vulkan::Buffer stagingBuffer(device,correctionBufferSize,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,allocator,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,false);
		
		/* Upload the correction mesh vertices: */
		CorrectionMeshVertex* dmvPtr=static_cast<CorrectionMeshVertex*>(stagingBuffer.map(0x0));
		for(int eye=0;eye<2;++eye)
			{
			/* Access the HMD configuration's per-eye post-distortion frame buffer viewport and distortion mesh: */
			const Vrui::IRect viewport=hc.getViewport(eye);
			const Vrui::HMDConfiguration::DistortionMeshVertex* smvPtr=hc.getDistortionMesh(eye);
			
			#if PRINT_HMDCONFIG
			std::cout<<"Post-distortion viewport for eye "<<eye<<": "<<viewport<<std::endl;
			#endif
			
			/* Calculate a sample position offset to access the eye's sub-image in pre-distortion frames: */
			Scalar eyeOffset(eye);
			
			/* Create the per-eye mesh vertices: */
			for(unsigned int y=0;y<correctionMeshSize[1];++y)
				{
				Scalar mvy=(Scalar(viewport.offset[1])+Scalar(y)*Scalar(viewport.size[1])/Scalar(correctionMeshSize[1]-1))/Scalar(hmdDisplaySize[1]);
				for(unsigned int x=0;x<correctionMeshSize[0];++x,++dmvPtr,++smvPtr)
					{
					/* Set the mesh vertex's distortion-correcting sample positions for the three primary colors: */
					dmvPtr->red[0]=(eyeOffset+Scalar(smvPtr->red[0]))/Scalar(2);
					dmvPtr->red[1]=Scalar(smvPtr->red[1]);
					dmvPtr->green[0]=(eyeOffset+Scalar(smvPtr->green[0]))/Scalar(2);
					dmvPtr->green[1]=Scalar(smvPtr->green[1]);
					dmvPtr->blue[0]=(eyeOffset+Scalar(smvPtr->blue[0]))/Scalar(2);
					dmvPtr->blue[1]=Scalar(smvPtr->blue[1]);
					
					Scalar mvx=(Scalar(viewport.offset[0])+Scalar(x)*Scalar(viewport.size[0])/Scalar(correctionMeshSize[0]-1))/Scalar(hmdDisplaySize[0]);
					
					/* Calculate the mesh vertex's position based on the eye's post-distortion viewport: */
					dmvPtr->pos[0]=Scalar(2)*mvx-Scalar(1);
					dmvPtr->pos[1]=Scalar(1)-Scalar(2)*mvy;
					}
				}
			}
		
		/* Upload the correction mesh triangle strip indices: */
		Misc::UInt16* dmiPtr=reinterpret_cast<Misc::UInt16*>(dmvPtr);
		Misc::UInt16 baseIndex=0;
		for(int eye=0;eye<2;++eye)
			{
			for(unsigned int y=1;y<correctionMeshSize[1];++y)
				{
				for(unsigned int x=0;x<correctionMeshSize[0];++x,dmiPtr+=2)
					{
					dmiPtr[0]=baseIndex+Misc::UInt16((y-1)*correctionMeshSize[0]+x);
					dmiPtr[1]=baseIndex+Misc::UInt16(y*correctionMeshSize[0]+x);
					}
				
				/* Add the primitive restart code to start the next triangle strip: */
				*dmiPtr=Misc::UInt16(-1);
				++dmiPtr;
				}
			
			/* Offset vertex indices for the next per-eye mesh: */
			baseIndex+=Misc::UInt16(numVertices);
			}
		
		stagingBuffer.unmap();
		
		/* Copy the correction mesh from the staging buffer into the permanent buffer: */
		correctionMeshBuffer->copy(0,stagingBuffer,0,correctionBufferSize,commandPool);
		
		/* Mark the distortion correction mesh as up-to-date: */
		distortionMeshVersion=hc.getDistortionMeshVersion();
		}
	
	/* Release the VR device server's HMD configuration: */
	vrDeviceClient.unlockHmdConfigurations();
	}

void VRCompositor::hmdConfigurationUpdatedCallback(const Vrui::HMDConfiguration& newHmdConfiguration)
	{
	/* Mark the HMD configuration as outdated: */
	hmdConfigurationUpdated=true;
	}

void VRCompositor::environmentDefinitionUpdatedCallback(const Vrui::EnvironmentDefinition& newEnvironmentDefinition)
	{
	/* Update the environment definition: */
	Threads::Mutex::Lock environmentDefinitionLock(environmentDefinitionMutex);
	environmentDefinition=newEnvironmentDefinition;
	environmentDefinitionUpdated=true;
	}

Vulkan::Image VRCompositor::createInputImage(VkFormat inputImageFormat,bool initImage,const unsigned char initColor[4])
	{
	/* Access the current HMD configuration: */
	const HMDConfiguration& hmdConfiguration=sharedMemorySegment->hmdConfiguration.readBack();
	
	/* Create an uninitialized image of the appropriate size and format: */
	VkExtent3D inputImageExtent;
	inputImageExtent.width=hmdConfiguration.frameSize[0];
	inputImageExtent.height=hmdConfiguration.frameSize[1];
	inputImageExtent.depth=1;
	VkImageUsageFlags inputImageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
	if(initImage)
		{
		/* Add the transfer destination usage bit because we're initializing the image here: */
		inputImageUsage|=VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
	Vulkan::Image image(device,VK_IMAGE_TYPE_2D,inputImageFormat,inputImageExtent,VK_IMAGE_TILING_OPTIMAL,inputImageUsage,false,allocator,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,true);
	
	if(initImage)
		{
		/* Create a CPU-accessible staging pixel buffer to initialize the image to a solid color: */
		size_t imageDataSize=hmdConfiguration.frameSize.volume()*4; // 8-bit RGBA pixels
		Vulkan::Buffer stagingBuffer(device,imageDataSize,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,allocator,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,false);
		unsigned char* bufferPtr=static_cast<unsigned char*>(stagingBuffer.map(0x0));
		unsigned char* bufferEnd=bufferPtr+imageDataSize;
		for(;bufferPtr!=bufferEnd;bufferPtr+=4)
			{
			/* Initialize the pixel's color: */
			for(int i=0;i<4;++i)
				bufferPtr[i]=initColor[i];
			}
		stagingBuffer.unmap();
		
		/* Transition the image's layout for optimal upload: */
		image.transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,transientCommandPool);
		
		/* Copy the staging buffer's content to the image: */
		image.copyFromBuffer(stagingBuffer,hmdConfiguration.frameSize,transientCommandPool);
		}
	
	/* Transition the image's layout for optimal shader access: */
	image.transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,transientCommandPool);
	
	return image;
	}

void VRCompositor::updateGraphicsPipeline(void)
	{
	/* Delete the current graphics pipeline: */
	delete graphicsPipeline;
	
	/* Create a graphics pipeline constructor: */
	Vulkan::GraphicsPipeline::Constructor gpc;
	
	/* Add the shaders to the pipeline: */
	gpc.addShaderStage(vertexShader);
	gpc.addShaderStage(fragmentShader);
	
	/* Add descriptions for the input vertex array: */
	CorrectionMeshVertex::addVertexInputDescriptions(gpc);
	
	/* Set up input assembly: */
	VkPipelineInputAssemblyStateCreateInfo& ias=gpc.getInputAssemblyState();
	ias.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	ias.primitiveRestartEnable=VK_TRUE;
	
	/* Set up viewports and scissor rectangles: */
	VkViewport viewport;
	viewport.x=0.0f;
	viewport.y=0.0f;
	viewport.width=float(hmd.getVisibleRegion().width);
	viewport.height=float(hmd.getVisibleRegion().height);
	viewport.minDepth=1.0f;
	viewport.maxDepth=1.0f;
	gpc.addViewport(viewport);
	VkRect2D scissor;
	scissor.offset.x=0;
	scissor.offset.y=0;
	scissor.extent=hmd.getVisibleRegion();
	gpc.addScissor(scissor);
	
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
	
	/* Create the new graphics pipeline: */
	graphicsPipeline=new Vulkan::GraphicsPipeline(device,gpc,pipelineLayout,hmd.getRenderPass(),0);
	}

void VRCompositor::render(const VRCompositorProtocol::RenderResult& renderResult,const Vrui::TimePoint& exposureTime)
	{
	/* Access the current HMD configuration: */
	const HMDConfiguration& hmdConfiguration=sharedMemorySegment->hmdConfiguration.readBack();
	
	/* Get the most recent head device tracking state: */
	if(vrDeviceClient.hasSharedMemory())
		vrDeviceClient.updateDeviceStates();
	vrDeviceClient.lockState();
	const Vrui::VRDeviceState& vds=vrDeviceClient.getState();
	bool faceDetected=vds.getButtonState(faceDetectorButtonIndex);
	bool trackerValid=vds.getTrackerValid(headDeviceTrackerIndex);
	Vrui::TrackerState headTransform=vds.getTrackerState(headDeviceTrackerIndex).positionOrientation;
	Vrui::Vector headLv=vds.getTrackerState(headDeviceTrackerIndex).linearVelocity;
	Vrui::Vector headAv=vds.getTrackerState(headDeviceTrackerIndex).angularVelocity;
	Vrui::VRDeviceState::TimeStamp headTs=vds.getTrackerTimeStamp(headDeviceTrackerIndex);
	vrDeviceClient.unlockState();
	
	/* Predict the head pose for the given exposure time: */
	Vrui::VRDeviceState::TimeStamp exposureTs(long(exposureTime.tv_sec)*1000000L+(exposureTime.tv_nsec+500L)/1000L);
	Vrui::VRDeviceState::TimeStamp deltaTs=exposureTs-headTs;
	Vrui::Scalar delta=Vrui::Scalar(deltaTs)*Vrui::Scalar(1.0e-6);
	Vrui::TrackerState predHeadTransform(headLv*delta+headTransform.getTranslation(),Vrui::Rotation::rotateScaledAxis(headAv*delta)*headTransform.getRotation());
	predHeadTransform.renormalize();
	
	/* Calculate the left- and right-eye reprojection states: */
	ReprojectionState reprojectionStates[2];
	for(int eye=0;eye<2;++eye)
		{
		if(active&&reprojection)
			{
			/* Calculate the reprojection rotation: */
			Vrui::Rotation rot=Vrui::Rotation::identity;
			rot*=Geometry::invert(screenTransforms[eye].getRotation());
			rot*=Geometry::invert(renderResult.headDeviceTransform.getRotation());
			rot*=predHeadTransform.getRotation();
			rot*=screenTransforms[eye].getRotation();
			rot.renormalize();
			
			/* Write the reprojection rotation into the column-major matrix pushed to the distortion correction shader: */
			Geometry::Matrix<Vrui::Scalar,3,3> rotMat;
			rot.writeMatrix(rotMat);
			for(int i=0;i<3;++i)
				for(int j=0;j<3;++j)
					reprojectionStates[eye].rotation[j][i]=float(rotMat(i,j));
			}
		else
			{
			/* Reset the reprojection rotation to identity: */
			for(int i=0;i<3;++i)
				for(int j=0;j<3;++j)
					reprojectionStates[eye].rotation[j][i]=float(i==j?1:0);
			}
		
		/* Calculate the viewport-to-tangent space transformation, accounting for the half-offset x coordinates to access the left/right subimages: */
		Vrui::Scalar vpWidth=hmdConfiguration.eyeFovs[eye][1]-hmdConfiguration.eyeFovs[eye][0];
		reprojectionStates[eye].viewportOffset[0]=float(hmdConfiguration.eyeFovs[eye][0]-Vrui::Scalar(eye)*vpWidth);
		reprojectionStates[eye].viewportScale[0]=float(Vrui::Scalar(2)*vpWidth);
		reprojectionStates[eye].viewportOffset[1]=float(hmdConfiguration.eyeFovs[eye][2]);
		reprojectionStates[eye].viewportScale[1]=float(hmdConfiguration.eyeFovs[eye][3]-hmdConfiguration.eyeFovs[eye][2]);
		}
	
	/* Prepare a command buffer for the next image: */
	commandBuffer.begin(0x0);
	
	if(!active&&trackerValid&&faceDetected)
		{
		/* Render the physical-space boundaries into the locked input image: */
		boundaryRenderer->render(hmdConfiguration,predHeadTransform,renderResult.imageIndex,commandBuffer);
		
		/* Insert an image memory barrier on the locked input image: */
		VkImageMemoryBarrier barrier;
		barrier.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.pNext=0;
		barrier.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
		barrier.oldLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
		barrier.image=inputImages[renderResult.imageIndex].getHandle();
		barrier.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel=0;
		barrier.subresourceRange.levelCount=1;
		barrier.subresourceRange.baseArrayLayer=0;
		barrier.subresourceRange.layerCount=1;
		commandBuffer.pipelineBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0x0,barrier);
		}
	
	/* Begin a render pass by clearing the framebuffer: */
	VkRect2D region;
	region.offset.x=0;
	region.offset.y=0;
	region.extent=hmd.getVisibleRegion();
	std::vector<VkClearValue> clearValues;
	clearValues.reserve(1);
	VkClearValue clear;
	if(faceDetected&&!trackerValid)
		{
		/* Clear the framebuffer to gray to indicate loss of head tracking: */
		clear.color.float32[0]=0.333f;
		clear.color.float32[1]=0.333f;
		clear.color.float32[2]=0.333f;
		clear.color.float32[3]=1.0f;
		}
	else
		{
		/* Clear the framebuffer to the current clear color: */
		for(int i=0;i<4;++i)
			clear.color.float32[i]=clearColor[i];
		}
	clearValues.push_back(clear);
	commandBuffer.beginRenderPass(hmd.getRenderPass(),hmd.getAcquiredFramebuffer(),region,clearValues);
	
	if(trackerValid&&faceDetected)
		{
		/* Bind the graphics pipeline: */
		commandBuffer.bindPipeline(*graphicsPipeline);
		
		/* Bind the vertex and index buffers to the graphics pipeline: */
		commandBuffer.bindVertexBuffers(0,*correctionMeshBuffer,0);
		commandBuffer.bindIndexBuffer(*correctionMeshBuffer,correctionMeshIndexDataOffset,VK_INDEX_TYPE_UINT16);
		
		/* Bind the descriptor set to the graphics pipeline: */
		commandBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS,pipelineLayout,0,descriptorSets[renderResult.imageIndex]);
		
		/* Draw the left- and right-eye distortion correction meshes: */
		uint32_t indicesPerMesh=(correctionMeshSize[1]-1)*(correctionMeshSize[0]*2+1);
		for(int eye=0;eye<2;++eye)
			{
			commandBuffer.pushConstants(pipelineLayout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(ReprojectionState),&reprojectionStates[eye]);
			commandBuffer.drawIndexed(indicesPerMesh,1,eye*indicesPerMesh,0,0);
			}
		}
	
	/* End the render pass: */
	commandBuffer.endRenderPass();
	
	/* Finalize the command buffer: */
	commandBuffer.end();
	
	/* Submit the command buffer to the device's rendering queue: */
	device.submitRenderingCommand(frameBufferAvailable,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,commandBuffer,renderingFinished,renderingFinishedFence);
	}

#if VRCOMPOSITOR_USE_LATENCYTESTER

void VRCompositor::latencySampleCallback(unsigned int timeStamp)
	{
	if(latencyTestState==2)
		{
		/* Calculate the display latency: */
		std::cout<<"Display latency: "<<double(latencyTest.setAndDiff())*1.0e3<<" ms"<<std::endl;
		
		latencyTestState=3;
		}
	}

void VRCompositor::latencyButtonEventCallback(unsigned int timeStamp)
	{
	/* Request a latency test: */
	if(latencyTestState==0)
		latencyTestState=1;
	}

#endif

Vulkan::CStringList VRCompositor::getInstanceExtensions(void)
	{
	/* Collect the list of required extensions: */
	Vulkan::CStringList result;
	result.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	Vulkan::MemoryAllocator::addRequiredInstanceExtensions(result);
	HMD::addRequiredInstanceExtensions(result);
	
	return result;
	}

namespace {

/****************
Helper functions:
****************/

VkSamplerCreateInfo setupInputImageSampler(void)
	{
	VkSamplerCreateInfo result;
	
	result.sType=VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	result.pNext=0;
	result.flags=0x0;
	result.magFilter=VK_FILTER_LINEAR;
	result.minFilter=VK_FILTER_LINEAR;
	result.mipmapMode=VK_SAMPLER_MIPMAP_MODE_LINEAR;
	result.addressModeU=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	result.addressModeV=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	result.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	result.mipLodBias=0.0f;
	result.anisotropyEnable=VK_FALSE;
	result.maxAnisotropy=0.0f;
	result.compareEnable=VK_FALSE;
	result.compareOp=VK_COMPARE_OP_ALWAYS;
	result.minLod=0.0f;
	result.maxLod=0.0f;
	result.borderColor=VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	result.unnormalizedCoordinates=VK_FALSE;
	
	return result;
	}

}

VRCompositor::VRCompositor(Threads::EventDispatcher& sDispatcher,Vrui::VRDeviceClient& sVrDeviceClient,Vulkan::Instance& sInstance,const std::string& hmdName,double targetRefreshRate)
	:dispatcher(sDispatcher),vrDeviceClient(sVrDeviceClient),
	 environmentDefinitionUpdated(false),
	 headDeviceTrackerIndex(0),faceDetectorButtonIndex(0),
	 distortionMeshVersion(0),hmdConfigurationUpdated(false),
	 sharedMemory(VRCOMPOSITOR_SHAREDMEMORY_NAME,sizeof(SharedMemorySegment)),
	 sharedMemorySegment(new(sharedMemory.getValue<SharedMemorySegment>(0)) SharedMemorySegment),
	 instance(sInstance),
	 hmd(instance,hmdName,targetRefreshRate,getDeviceExtensions()),
	 device(hmd.getDevice()),
	 allocator(device,VkDeviceSize(128)*VkDeviceSize(1024*1024),hmd.getDirectDevice().getDeviceLimits().nonCoherentAtomSize),
	 commandPool(device,device.getRenderingQueueFamilyIndex(),VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
	 transientCommandPool(device,device.getRenderingQueueFamilyIndex(),VK_COMMAND_POOL_CREATE_TRANSIENT_BIT),
	 descriptorPool(createDescriptorPool(device)),
	 renderResults(sharedMemorySegment->renderResults),
	 correctionMeshSize(0,0),correctionMeshBuffer(0),correctionMeshIndexDataOffset(0),
	 inputImageSampler(device,setupInputImageSampler()),
	 vertexShader(device,*IO::openDirectory(VRCOMPOSITOR_SHADERDIR),Vulkan::ShaderModule::Vertex,"DistortionCorrection"),
	 fragmentShader(device,*IO::openDirectory(VRCOMPOSITOR_SHADERDIR),Vulkan::ShaderModule::Fragment,"DistortionCorrection"),
	 descriptorSetLayout(createDescriptorSetLayout(device,inputImageSampler)),
	 pipelineLayout(createPipelineLayout(device,descriptorSetLayout)),
	 graphicsPipeline(0),
	 commandBuffer(commandPool.allocateCommandBuffer()),
	 frameBufferAvailable(device),renderingFinished(device),
	 renderingFinishedFence(device),
	 boundaryRenderer(0),
	 active(false),
	 reprojection(true),
	 #if VRCOMPOSITOR_USE_LATENCYTESTER
	 latencyTester(0),latencyTestState(0),
	 #endif
	 keepRunning(false)
	{
	/* Retrieve the device daemon's environment definition: */
	vrDeviceClient.getEnvironmentDefinition(environmentDefinition);
	
	/* Activate the device client: */
	vrDeviceClient.activate();
	
	/* Initialize the shared memory segment: */
	sharedMemorySegment->protocolVersion=protocolVersion;
	
	/* Initialize the client-facing and internal HMD configuration: */
	updateHmdConfiguration(true);
	
	/* Install callbacks to get notified when the HMD configuration or the server's environment definition changes: */
	vrDeviceClient.setHmdConfigurationUpdatedCallback(headDeviceTrackerIndex,Misc::createFunctionCall(this,&VRCompositor::hmdConfigurationUpdatedCallback));
	vrDeviceClient.setEnvironmentDefinitionUpdatedCallback(Misc::createFunctionCall(this,&VRCompositor::environmentDefinitionUpdatedCallback));
	
	/* Create the input images and their views: */
	VkFormat inputImageFormat=VK_FORMAT_R8G8B8A8_SRGB; // Assume rendered frames are stored in non-linear compressed sRGB
	// VkFormat inputImageFormat=VK_FORMAT_R8G8B8A8_UNORM; // Assume rendered frames are stored using linear RGB
	inputImages.reserve(3);
	inputImageViews.reserve(3);
	static const unsigned char initColors[3][4]=
		{
		{255,0,255,255},{255,0,0,255},{0,0,0,255} // Last (black) image will be displayed initially
		};
	for(int i=0;i<3;++i)
		{
		/* Create the input image: */
		inputImages.push_back(createInputImage(inputImageFormat,true,initColors[i]));
	
		/* Create the input image view: */
		inputImageViews.push_back(Vulkan::ImageView(inputImages.back(),inputImageFormat));
		}
	
	/* Initialize the rendering result triple buffer: */
	Vrui::TimePoint now;
	for(unsigned int i=0;i<3;++i)
		{
		renderResults.getBuffer(i).imageIndex=i;
		renderResults.getBuffer(i).renderTime=now;
		renderResults.getBuffer(i).headDeviceTransform=Vrui::ONTransform::identity;
		}
	
	/* Initialize the graphics pipeline: */
	updateGraphicsPipeline();
	
	/* Initialize the descriptor sets to render from the three input images: */
	descriptorSets.reserve(3);
	for(unsigned int i=0;i<3;++i)
		{
		descriptorSets.push_back(descriptorPool.allocateDescriptorSet(descriptorSetLayout));
		descriptorSets.back().setCombinedImageSampler(0,0,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,inputImageViews[i],inputImageSampler);
		}
	
	#if PRINT_HMDCONFIG
	std::cout<<"HMD's display mode is "<<hmd.getVisibleRegion().width<<'x'<<hmd.getVisibleRegion().height<<'@'<<double(hmd.getRefreshRate())/1000.0<<std::endl;
	#endif
	
	/* Create a boundary renderer: */
	const HMDConfiguration& hmdConfiguration=sharedMemorySegment->hmdConfiguration.readBack();
	VkExtent2D inputImageExtent;
	inputImageExtent.width=hmdConfiguration.frameSize[0];
	inputImageExtent.height=hmdConfiguration.frameSize[1];
	boundaryRenderer=new BoundaryRenderer(device,allocator,commandPool,inputImageFormat,*IO::openDirectory(VRCOMPOSITOR_SHADERDIR),inputImageExtent,inputImageViews,environmentDefinition);
	
	/* Set the initial clear color: */
	for(int i=0;i<3;++i)
		clearColor[i]=0.0f;
	clearColor[3]=1.0f;
	
	#if VRCOMPOSITOR_USE_LATENCYTESTER
	
	try
		{
		/* Create a latency tester: */
		latencyTester=new LatencyTester(RawHID::BUSTYPE_USB,0,dispatcher);
		latencyTester->setLatencyConfiguration(false,LatencyTester::Color(128,128,128));
		latencyTester->setLatencyDisplay(2,0x40400040U);
		
		/* Register callbacks: */
		latencyTester->setSampleCallback(Misc::createFunctionCall(this,&VRCompositor::latencySampleCallback),LatencyTester::Color(128,128,128));
		latencyTester->setButtonEventCallback(Misc::createFunctionCall(this,&VRCompositor::latencyButtonEventCallback));
		}
	catch(const std::runtime_error& err)
		{
		std::cerr<<"Unable to create latency tester due to exception "<<err.what()<<std::endl;
		}
	
	#endif
	}

VRCompositor::~VRCompositor(void)
	{
	/* Deactivate the device server: */
	vrDeviceClient.deactivate();
	
	/* Remove the HMD configuration and environment definition change callbacks: */
	vrDeviceClient.setHmdConfigurationUpdatedCallback(headDeviceTrackerIndex,0);
	vrDeviceClient.setEnvironmentDefinitionUpdatedCallback(0);
	
	/* Release all allocated resources: */
	delete boundaryRenderer;
	delete graphicsPipeline;
	delete correctionMeshBuffer;
	
	#if VRCOMPOSITOR_USE_LATENCYTESTER
	delete latencyTester;
	#endif
	}

int VRCompositor::getSharedMemoryBlockFd(void)
	{
	return sharedMemory.getFd();
	}

int VRCompositor::getInputImageBlockFd(void)
	{
	/* Check whether all input images are backed by the same memory block: */
	for(unsigned int i=1;i<3;++i)
		if(inputImages[i].getAllocation().getHandle()!=inputImages[0].getAllocation().getHandle())
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Input images do not share memory handle");
	
	/* Return the file descriptor for the memory block backing all three input images: */
	return inputImages[0].getExportFd();
	}

VkDeviceSize VRCompositor::getInputImageBlockSize(void)
	{
	/* Check whether all input images are backed by the same memory block: */
	for(unsigned int i=1;i<3;++i)
		if(inputImages[i].getAllocation().getHandle()!=inputImages[0].getAllocation().getHandle())
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Input images do not share memory handle");
	
	/* Return the total size of the memory block backing all three input images: */
	return inputImages[0].getAllocation().getBlockSize();
	}

namespace {

long tonsec(const Realtime::Time& time)
	{
	return time.tv_sec*long(1000000000)+time.tv_nsec;
	}

}

void VRCompositor::run(Threads::EventDispatcher::ListenerKey vsyncSignalKey)
	{
	/* If the device client does not have shared memory, start streaming VR device states: */
	if(!vrDeviceClient.hasSharedMemory())
		vrDeviceClient.startStream(0);
	
	/* Lock the most recent render result in the input triple buffer: */
	if(renderResults.lockNewValue())
		{
		/* Need to do an image transition here, I suppose? */
		// ...
		
		// DEBUGGING
		// std::cout<<"Locked new image "<<renderResults.getLockedValue().imageIndex<<std::endl;
		}
	
	/* Acquire the first image from the HMD's swap chain: */
	hmd.acquireSwapchainImage(frameBufferAvailable);
	
	/* Render the first frame and present it to the HMD's swap chain: */
	render(renderResults.getLockedValue(),Vrui::TimePoint());
	hmd.present(renderingFinished);
	
	/* Start the vblank estimator: */
	hmd.startVblankEstimator();
	
	/* Write the predicted next vblank event time and the current vblank period estimate into the shared memory segment: */
	VblankTimer& vblankTimer=sharedMemorySegment->vblankTimer.startWrite();
	vblankTimer.frameIndex=hmd.getVblankCounter();
	vblankTimer.nextVblankTime=hmd.predictNextVblank();
	vblankTimer.vblankPeriod=hmd.getVblankPeriod();
	sharedMemorySegment->vblankTimer.finishWrite();
	
	/* Wait until the first frame is actually done rendering: */
	renderingFinishedFence.wait(true);
	
	// DEBUGGING
	nappytime=0;
	
	/* Reduce busy waiting by sleeping for some amount of time before waiting for vertical retrace: */
	Vrui::TimeVector busyWaitPeriod(0,5000000); // Start busy waiting for 5ms until vblank estimates solidify
	
	/* Render frames until whenever: */
	keepRunning=true;
	while(keepRunning)
		{
		// DEBUGGING
		if(nappytime!=0)
			{
			usleep(nappytime);
			nappytime=0;
			}
		
		/* Check if the environment definition has been updated: */
		{
		Threads::Mutex::Lock environmentDefinitionLock(environmentDefinitionMutex);
		if(environmentDefinitionUpdated)
			{
			/* Update the boundary renderer: */
			boundaryRenderer->updateEnvironmentDefinition(device,allocator,commandPool,environmentDefinition);
			
			environmentDefinitionUpdated=false;
			}
		}
		
		/* Check if the HMD configuration has been updated: */
		if(hmdConfigurationUpdated)
			{
			updateHmdConfiguration(false);
			hmdConfigurationUpdated=false;
			}
		
		/* Acquire the next image from the HMD's swap chain: */
		hmd.acquireSwapchainImage(frameBufferAvailable);
		
		#if 0
		
		/* Wait until shortly before the predicted next vertical retrace to minimize busy-waiting on vertical retrace: */
		Vrui::TimePoint preVsyncTime=sharedMemorySegment->vblankTimer.readBack().nextVblankTime;
		preVsyncTime-=busyWaitPeriod;
		Vrui::TimePoint::sleep(preVsyncTime);
		
		#endif
		
		/* Wait for the next vertical retrace: */
		uint64_t numMissedVblanks=hmd.vsync();
		if(numMissedVblanks>0)
			{
			std::cerr<<"VRCompositor: Missed "<<numMissedVblanks<<" vblank events at frame "<<hmd.getVblankCounter()<<" with busy wait period "<<busyWaitPeriod.tv_nsec/1000<<" usec"<<std::endl;
			
			/* Increase the busy wait period to not miss the next vblank: */
			busyWaitPeriod.tv_nsec+=1000000;
			}
		else if(busyWaitPeriod.tv_nsec>500000)
			{
			/* Slowly reduce the busy wait period down to 0.5ms: */
			busyWaitPeriod.tv_nsec-=100000;
			}
		
		#if VRCOMPOSITOR_USE_LATENCYTESTER
		
		int lts=latencyTestState;
		if(lts!=0)
			{
			if(lts==1)
				{
				/* Show a white screen to trigger the latency tester: */
				for(int i=0;i<3;++i)
					clearColor[i]=1.0f;
				
				/* Start a latency test: */
				Misc::UInt8 testIntensity(255);
				latencyTester->startLatencyTest(LatencyTester::Color(testIntensity,testIntensity,testIntensity));
				latencyTest=hmd.getVblankTime();
				
				latencyTestState=2;
				}
			else if(lts==3)
				{
				/* Reset the clear color to black: */
				for(int i=0;i<3;++i)
					clearColor[i]=0.0f;
				
				/* Finish the latency test: */
				latencyTestState=0;
				}
			}
		
		#endif
		
		/* Write the predicted next vblank event time and the current vblank period estimate into the shared memory segment: */
		VblankTimer& vblankTimer=sharedMemorySegment->vblankTimer.startWrite();
		vblankTimer.frameIndex=hmd.getVblankCounter();
		vblankTimer.nextVblankTime=hmd.predictNextVblank();
		vblankTimer.vblankPeriod=hmd.getVblankPeriod();
		sharedMemorySegment->vblankTimer.finishWrite();
		
		/* Send a vsync signal to a connected client: */
		dispatcher.signal(vsyncSignalKey,0);
		
		/* Lock the next image in the input triple buffer: */
		bool newImage=renderResults.lockNewValue();
		RenderResult& renderResult=renderResults.getLockedValue();
		if(newImage)
			{
			/* Need to do an image transition here, I suppose? */
			// ...
			
			// DEBUGGING
			// std::cout<<"Locked new image "<<renderResult.imageIndex;
			// std::cout<<", "<<tonsec(sleepWakeup-renderResult.renderTime)<<" ns old"<<std::endl;
			}
		else
			{
			// DEBUGGING
			// std::cout<<"Repeating old image "<<renderResult.imageIndex;
			// std::cout<<", now "<<tonsec(sleepWakeup-renderResult.renderTime)<<" ns old"<<std::endl;
			}
		
		/* Render the locked input image: */
		render(renderResult,hmd.getVblankTime()+sharedMemorySegment->hmdConfiguration.readBack().exposeOffset);
		
		/* Present the finished framebuffer to the HMD's swap chain: */
		hmd.present(renderingFinished);
		
		/* Wait until this frame is actually done rendering: */
		renderingFinishedFence.wait(true);
		}
	
	/* If the device client does not have shared memory, stop streaming VR device states: */
	if(!vrDeviceClient.hasSharedMemory())
		vrDeviceClient.stopStream();
	
	/* Wait for the logical device to finish all pending operations: */
	device.waitIdle();
	}

void VRCompositor::shutdown(void)
	{
	/* Tell the run method to stop: */
	keepRunning=false;
	}

void VRCompositor::activate(void)
	{
	active=true;
	}

void VRCompositor::deactivate(void)
	{
	active=false;
	}

void VRCompositor::toggleReprojection(void)
	{
	reprojection=!reprojection;
	std::cout<<"Reprojection "<<(reprojection?"enabled":"disabled")<<std::endl;
	}

void VRCompositor::adjustExposeOffset(long step)
	{
	HMDConfiguration hmdConfiguration=sharedMemorySegment->hmdConfiguration.readBack();
	hmdConfiguration.exposeOffset+=Vrui::TimeVector(0,step);
	std::cout<<"Expose offset: "<<tonsec(hmdConfiguration.exposeOffset)<<std::endl;
	sharedMemorySegment->hmdConfiguration.write(hmdConfiguration);
	}

#if 0

/****************
Main entry point:
****************/

int main(void)
	{
	/* Create the compositor object: */
	VRCompositor compositor("Valve Corporation Index HMD",90.0);
	
	// DEBUGGING - set the time base:
	timeBase.set();
	
	/* Run it: */
	compositor.run();
	
	return 0;
	}

#endif
