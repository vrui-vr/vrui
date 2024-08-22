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

#ifndef VRCOMPOSITOR_INCLUDED
#define VRCOMPOSITOR_INCLUDED

#include <string>
#include <vector>
#include <Misc/SizedTypes.h>
#include <Realtime/Time.h>
#include <Realtime/SharedMemory.h>
#include <Threads/Mutex.h>
#include <Threads/EventDispatcher.h>
#include <Geometry/Point.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Vulkan/Types.h>
#include <Vulkan/Common.h>
#include <Vulkan/MemoryAllocator.h>
#include <Vulkan/CommandPool.h>
#include <Vulkan/DescriptorPool.h>
#include <Vulkan/Image.h>
#include <Vulkan/Sampler.h>
#include <Vulkan/ShaderModule.h>
#include <Vulkan/DescriptorSetLayout.h>
#include <Vulkan/PipelineLayout.h>
#include <Vulkan/CommandBuffer.h>
#include <Vulkan/Semaphore.h>
#include <Vulkan/Fence.h>
#include <Vrui/Types.h>
#include <Vrui/EnvironmentDefinition.h>
#include <Vrui/Internal/VRCompositorProtocol.h>

#include "Config.h"
#include "HMD.h"

/* Forward declarations: */
namespace Vrui {
class HMDConfiguration;
class VRDeviceClient;
}
namespace Vulkan {
class Instance;
class Device;
class ImageView;
class Buffer;
class GraphicsPipeline;
class DescriptorSet;
}
#if VRCOMPOSITOR_USE_LATENCYTESTER
class LatencyTester;
#endif
class BoundaryRenderer;

class VRCompositor:public Vrui::VRCompositorProtocol
	{
	/* Elements: */
	private:
	Threads::EventDispatcher& dispatcher; // Reference to the server's event dispatcher
	Vrui::VRDeviceClient& vrDeviceClient; // Client connected to a Vrui VRDeviceDaemon
	Threads::Mutex environmentDefinitionMutex; // Mutex protecting the environment definition
	Vrui::EnvironmentDefinition environmentDefinition; // Definition of the physical environment read from the device daemon
	bool environmentDefinitionUpdated; // Flag if the environment definition has been updated
	int headDeviceTrackerIndex; // Tracker index of the HMD's tracking device
	int faceDetectorButtonIndex; // Index of the button representing the HMD's face detection sensor
	Vrui::ONTransform screenTransforms[2]; // HMD-relative transformations of the HMD's two screens
	Vrui::Scalar screenSizes[2][2]; // Sizes of the HMD's two screens
	unsigned int distortionMeshVersion; // Version numbers of the HMD configuration's distortion correction mesh
	bool hmdConfigurationUpdated; // Flag if the HMD configuration has been updated
	Realtime::SharedMemory sharedMemory; // Shared memory to communicate with VR application clients
	SharedMemorySegment* sharedMemorySegment; // Pointer to shared memory communication data structure
	
	Vulkan::Instance& instance; // Application's Vulkan API instance
	
	HMD hmd; // The head-mounted display controlled by the compositor
	Vulkan::Device& device; // Reference to the logical device representing the GPU to which the HMD is connected
	
	/* Elements related to memory management: */
	Vulkan::MemoryAllocator allocator; // A memory allocator for the logical device
	Vulkan::CommandPool commandPool; // A command pool to allocate command buffers
	Vulkan::CommandPool transientCommandPool; // A command pool to allocate transient command buffers
	Vulkan::DescriptorPool descriptorPool; // A descriptor pool to allocate descriptor sets
	
	/* Elements defining the input triple buffer of undistorted stereoscopic views: */
	std::vector<Vulkan::Image> inputImages; // Array of three input images holding undistorted stereoscopic views rendered by a VR application client
	std::vector<Vulkan::ImageView> inputImageViews; // Views to access the input images
	Threads::TripleBuffer<RenderResult>& renderResults; // Reference to the render results triple buffer in shared memory
	Vrui::TimePoint lastNewImageTime; // Last time a new image was pulled from the rendering result triple buffer
	
	/* Elements defining the reprojection and distortion correction shader: */
	Vulkan::Size correctionMeshSize; // Number of vertices in the distortion correction mesh
	Vulkan::Buffer* correctionMeshBuffer; // Buffer containing the correction mesh vertices
	VkDeviceSize correctionMeshIndexDataOffset; // Offset of index data in correction mesh buffer
	Vulkan::Sampler inputImageSampler; // Sampler to access the input images from the reprojection and distortion correction shader
	Vulkan::ShaderModule vertexShader,fragmentShader; // Vulkan shader modules to reproject and distortion-correct the current input image
	
	/* The graphics pipeline for distortion correction and compositing: */
	Vulkan::DescriptorSetLayout descriptorSetLayout; // Descriptor set layout for the graphics pipeline
	Vulkan::PipelineLayout pipelineLayout; // Layout for the graphics pipeline
	Vulkan::GraphicsPipeline* graphicsPipeline; // Graphics pipeline to render geometry in a render pass
	
	/* Elements to submit rendering commands and synchronize rendering: */
	std::vector<Vulkan::DescriptorSet> descriptorSets; // A descriptor set triplet to access the input images from the fragment shader
	Vulkan::CommandBuffer commandBuffer; // A persistent command buffer rewritten on every frame
	Vulkan::Semaphore frameBufferAvailable; // Semaphore to delay rendering into a framebuffer until that framebuffer is available
	Vulkan::Semaphore renderingFinished; // Semaphore to delay presenting a framebuffer until rendering is complete
	Vulkan::Fence renderingFinishedFence; // Fence to notify the CPU when the GPU is done rendering
	
	BoundaryRenderer* boundaryRenderer; // Module to render the VR environment's physical boundaries when no application client is connected
	
	bool active; // Flag whether a VR application client is currently connected
	bool reprojection; // Flag whether to reproject rendered frames based on current head orientation
	
	float clearColor[4]; // Clear color to fill the screen before rendering; modified to handle latency testing
	#if VRCOMPOSITOR_USE_LATENCYTESTER
	LatencyTester* latencyTester; // Pointer to Oculus latency tester device
	Vrui::TimePoint latencyTest; // Time at which the most recent latency test was started
	volatile int latencyTestState; // State of the current latency test procedure
	#endif
	
	volatile bool keepRunning; // Flag to shut down the run method
	
	// DEBUGGING
	volatile long nappytime; // Go to sleep...
	
	/* Private methods: */
	void updateHmdConfiguration(bool initial); // Updates the compositor's client-facing and internal HMD configuration by reading from the VR device server
	void hmdConfigurationUpdatedCallback(const Vrui::HMDConfiguration& newHmdConfiguration); // Callback called when the internal HMD configuration changes
	void environmentDefinitionUpdatedCallback(const Vrui::EnvironmentDefinition& newEnvironmentDefinition); // Callback called when the server's environment definition changes
	Vulkan::Image createInputImage(VkFormat inputImageFormat,bool initImage,const unsigned char initColor[4]); // Creates one of the input images that will be shared with VR application clients
	void updateGraphicsPipeline(void); // Updates the graphics pipeline
	void render(const RenderResult& renderResult,const Vrui::TimePoint& exposureTime); // Reprojects and distortion-corrects the input image of the given index into the HMD's framebuffer for the given image exposure time
	
	#if VRCOMPOSITOR_USE_LATENCYTESTER
	void latencySampleCallback(unsigned int timeStamp);
	void latencyButtonEventCallback(unsigned int timeStamp);
	#endif
	
	/* Constructors and destructors: */
	public:
	static Vulkan::CStringList getInstanceExtensions(void); // Returns list of Vulkan extensions required to run a VR compositor
	VRCompositor(Threads::EventDispatcher& sDispatcher,Vrui::VRDeviceClient& sVrDeviceClient,Vulkan::Instance& sInstance,const std::string& hmdName,double targetRefreshRate);
	virtual ~VRCompositor(void);
	
	/* Methods: */
	int getSharedMemoryBlockFd(void); // Returns a file descriptor to access the shared memory block used to communicate with VR application clients
	int getInputImageBlockFd(void); // Returns a file descriptor to access the memory block backing the three input images from another process
	VkDeviceSize getInputImageBlockSize(void); // Returns the total size of the memory block backing the three input images
	VkDeviceSize getInputImageMemSize(unsigned int inputImageIndex) const // Returns the memory size of the given input image
		{
		return inputImages[inputImageIndex].getSize();
		}
	VkDeviceSize getInputImageMemOffset(unsigned int inputImageIndex) const // Returns the offset of the given input image in the memory block backing them
		{
		return inputImages[inputImageIndex].getOffset();
		}
	void run(Threads::EventDispatcher::ListenerKey vsyncSignalKey); // Runs the compositing loop until shut down; sends the given signal to the given dispatcher on each vblank event
	void shutdown(void); // Shuts down the compositor's main loop
	void activate(void); // Tells the compositor that a client is connected
	void deactivate(void); // Tells the compositor that no client is connected
	void toggleReprojection(void); // Switches reprojection between on and off
	void adjustExposeOffset(long step); // Adjusts the post-vblank exposure offset
	
	// DEBUGGING
	void pause(long newNappytime) // Pauses the compositor thread by the given number of microseconds to simulate bad framedrops
		{
		nappytime=newNappytime;
		}
	};

#endif
