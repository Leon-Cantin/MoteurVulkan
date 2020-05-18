#include "renderer.h"

#include "vk_buffer.h"
#include "vk_debug.h"
#include "vk_commands.h"
#include "profile.h"
#include "vk_framework.h"
#include "gpu_synchronization.h"
#include "frame_graph.h"
#include "gfx_heaps_batched_allocator.h"

#include <array>
#include <iostream>

Swapchain g_swapchain;
std::array<VkCommandBuffer, SIMULTANEOUS_FRAMES> g_graphicsCommandBuffers;
std::array<VkCommandBuffer, SIMULTANEOUS_FRAMES> g_transferCommandBuffers;
std::array<VkCommandBuffer, SIMULTANEOUS_FRAMES> g_computeCommandBuffers;

std::array<VkSemaphore, SIMULTANEOUS_FRAMES> graphicPassFinishedSemaphores;
std::array<VkSemaphore, SIMULTANEOUS_FRAMES> imageAvailableSemaphores;
std::array<VkSemaphore, SIMULTANEOUS_FRAMES> renderFinishedSemaphores;
std::array<VkFence, SIMULTANEOUS_FRAMES> inFlightFences;

FG::FrameGraph( *_fFGScriptInitialize )(const Swapchain*);

FG::FrameGraph _frameGraph;

bool( *_needResize )();
void( *_getFrameBufferSize )(uint64_t* width, uint64_t* height);

VkSurfaceKHR _swapchainSurface;

GfxImage dummyImage;

static void CreateCommandBuffer()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = g_vk.graphicsCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(g_graphicsCommandBuffers.size());

	if (vkAllocateCommandBuffers(g_vk.device, &allocInfo, g_graphicsCommandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

static void CreateTransferCommandBuffer()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = g_vk.transferCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(g_transferCommandBuffers.size());

	if (vkAllocateCommandBuffers(g_vk.device, &allocInfo, g_transferCommandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

static void create_sync_objects()
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
	{
		if (vkCreateSemaphore(g_vk.device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(g_vk.device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(g_vk.device, &semaphoreInfo, nullptr, &graphicPassFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(g_vk.device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create semaphores!");
		}

		MarkVkObject((uint64_t)imageAvailableSemaphores[i], VK_OBJECT_TYPE_SEMAPHORE, "image available semaphore");
		MarkVkObject((uint64_t)renderFinishedSemaphores[i], VK_OBJECT_TYPE_SEMAPHORE, "render finished semaphore");
		MarkVkObject((uint64_t)graphicPassFinishedSemaphores[i], VK_OBJECT_TYPE_SEMAPHORE, "graphic pass finished semaphore");
		MarkVkObject((uint64_t)inFlightFences[i], VK_OBJECT_TYPE_FENCE, "In flight fence");
	}
}

static void cleanup_swap_chain()
{
	vkFreeCommandBuffers(g_vk.device, g_vk.graphicsCommandPool, static_cast<uint32_t>(g_graphicsCommandBuffers.size()), g_graphicsCommandBuffers.data());
	vkFreeCommandBuffers(g_vk.device, g_vk.transferCommandPool, static_cast<uint32_t>(g_transferCommandBuffers.size()), g_transferCommandBuffers.data());
	vkFreeCommandBuffers(g_vk.device, g_vk.computeCommandPool, static_cast<uint32_t>(g_computeCommandBuffers.size()), g_computeCommandBuffers.data());

	FG::Cleanup( &_frameGraph );

	for (auto image : g_swapchain.images)
		vkDestroyImageView(g_vk.device, image.imageView, nullptr);

	vkDestroySwapchainKHR(g_vk.device, g_swapchain.vkSwapchain, nullptr);
}

static void CreateDummyImage()
{
	GfxHeaps_CommitedResourceAllocator allocator = {};
	allocator.Prepare();
	CreateSolidColorImage( glm::vec4( 0, 0, 0, 0 ), &dummyImage, &allocator );
	allocator.Commit();
}

void InitRenderer( VkSurfaceKHR swapchainSurface, bool( *needResize )(), void( *getFrameBufferSize )(uint64_t* width, uint64_t* height) )
{
	//TODO: do better than this shit
	_getFrameBufferSize = getFrameBufferSize;
	_needResize = needResize;

	uint64_t width, height;
	_getFrameBufferSize( &width, &height );
	_swapchainSurface = swapchainSurface;
	createSwapChain( swapchainSurface, width, height, g_swapchain );

	VK::QueueFamilyIndices queue_family_indices = VK::find_queue_families( g_vk.physicalDevice, swapchainSurface );
	CreateCommandPool( queue_family_indices.graphics_family.value(), &g_vk.graphicsCommandPool );
	CreateCommandPool( queue_family_indices.transfer_family.value(), &g_vk.transferCommandPool );
	CreateSingleUseCommandPool( queue_family_indices.graphics_family.value(), &g_vk.graphicsSingleUseCommandPool );
	CreateSingleUseCommandPool( queue_family_indices.compute_family.value(), &g_vk.computeCommandPool );

	InitSamplers();

	CreateCommandBuffer();
	create_sync_objects();

	CreateTimeStampsQueryPool( SIMULTANEOUS_FRAMES );

	CreateDummyImage();
}

static void CompileFrameGraph()
{
	_frameGraph = _fFGScriptInitialize( &g_swapchain );
}

void recreate_swap_chain( VkSurfaceKHR swapchainSurface )
{
	//TODO: find a better way of handling window minimization
	uint64_t width = 0, height = 0;
	while (width == 0 || height == 0) {
		_getFrameBufferSize( &width, &height );
		//TODO
		//glfwWaitEvents();
	}

	vkDeviceWaitIdle(g_vk.device);

	cleanup_swap_chain();

	//TODO: try to use the "oldSwapchain" parameter to optimize when recreating swap chains
	_getFrameBufferSize(&width, &height);
	createSwapChain( swapchainSurface, width, height, g_swapchain );

	CompileFrameGraph();

	CreateCommandBuffer();
	CreateTransferCommandBuffer();
}

void CleanupFrameGraph()
{
	FG::Cleanup( &_frameGraph );
}

void CompileFrameGraph( FG::FrameGraph( *FGScriptInitialize )( const Swapchain* ) )
{
	_fFGScriptInitialize = FGScriptInitialize;
	CompileFrameGraph();
}

bool verify_swap_chain(VkResult result)
{
	//if (result != VK_SUCCESS)
		//throw std::runtime_error("failed to acquire swap chain image");

	return result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _needResize() ? false : true;
}

void RecordCommandBuffer(uint32_t currentFrame, const SceneFrameData* frameData)
{
	VkCommandBuffer graphicsCommandBuffer = g_graphicsCommandBuffers[currentFrame];
	BeginCommandBufferRecording(graphicsCommandBuffer);

	CmdResetTimeStampSet(graphicsCommandBuffer, currentFrame);

	CmdWriteTimestamp(graphicsCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, Timestamp::COMMAND_BUFFER_START, currentFrame);

	FG::RecordDrawCommands(currentFrame, frameData, graphicsCommandBuffer, g_swapchain.extent, &_frameGraph );

	CmdWriteTimestamp(graphicsCommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, Timestamp::COMMAND_BUFFER_END, currentFrame);

	EndCommandBufferRecording(graphicsCommandBuffer);
}

void WaitForFrame(uint32_t currentFrame)
{
	vkWaitForFences(g_vk.device, 1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
}

void draw_frame(uint32_t currentFrame, const SceneFrameData* frameData)
{
	RecordCommandBuffer(currentFrame, frameData);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(g_vk.device, g_swapchain.vkSwapchain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
	//TODO: Do I really need 2 check for recreate swap chain in this function?
	if (!verify_swap_chain(result)) {
		unsignalSemaphore(imageAvailableSemaphores[currentFrame]);
		recreate_swap_chain( _swapchainSurface );
		return;
	}

	//Graphics
	VkSubmitInfo graphicsSubmitInfo = {};
	graphicsSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	graphicsSubmitInfo.waitSemaphoreCount = 1;
	graphicsSubmitInfo.pWaitSemaphores = waitSemaphores;
	graphicsSubmitInfo.pWaitDstStageMask = waitStages;
	graphicsSubmitInfo.commandBufferCount = 1;
	graphicsSubmitInfo.pCommandBuffers = &g_graphicsCommandBuffers[currentFrame];

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] }; //TODO make a system that keeps the semaphores ordered
	graphicsSubmitInfo.signalSemaphoreCount = 1;
	graphicsSubmitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(g_vk.device, 1, &inFlightFences[currentFrame]);
	if (vkQueueSubmit(g_vk.graphics_queue, 1, &graphicsSubmitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	//Present
	VkSemaphore presentWaitSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = presentWaitSemaphores;

	VkSwapchainKHR swapChains[] = { g_swapchain.vkSwapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional, for multiple swapchains

	result = vkQueuePresentKHR(g_vk.present_queue, &presentInfo);
	if (!verify_swap_chain(result))
	{
		std::cout << "recreating swap chain " << imageIndex << currentFrame << std::endl;
		recreate_swap_chain( _swapchainSurface );
	}
}

void CleanupRenderer() {
	DestroyImage( &dummyImage );

	cleanup_swap_chain();

	DestroySamplers();

	FG::Cleanup( &_frameGraph );

	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
	{
		vkDestroySemaphore(g_vk.device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(g_vk.device, imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(g_vk.device, graphicPassFinishedSemaphores[i], nullptr);
		vkDestroyFence(g_vk.device, inFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(g_vk.device, g_vk.graphicsCommandPool, nullptr);
	vkDestroyCommandPool(g_vk.device, g_vk.graphicsSingleUseCommandPool, nullptr);
	vkDestroyCommandPool(g_vk.device, g_vk.computeCommandPool, nullptr);
	vkDestroyCommandPool(g_vk.device, g_vk.transferCommandPool, nullptr);

	DestroyTimeStampsPool();
}

void CmdBindVertexInputs( VkCommandBuffer commandBuffer, const std::vector<VIBinding>& gpuPipelineVIBindings, const GfxModel& gfxModel )
{
	const uint32_t maxVertexInputBinding = 16;
	const uint32_t gpuPipelineVIBingindCount = gpuPipelineVIBindings.size();
	assert( gpuPipelineVIBingindCount <= maxVertexInputBinding );
	VkBuffer vertexBuffers[maxVertexInputBinding];
	VkDeviceSize offsets[maxVertexInputBinding];

	for( uint32_t i = 0; i < gpuPipelineVIBingindCount; ++i )
	{
		const GfxModelVertexInput* modelVI = GetVertexInput( gfxModel, gpuPipelineVIBindings[i].desc.dataType );
		assert( gpuPipelineVIBindings[i].desc == modelVI->desc );
		//assert( modelVI.vertAttribBuffers != VK_NULL_HANDLE );
		vertexBuffers[i] = modelVI->buffer.buffer;
		offsets[i] = 0;
	}

	vkCmdBindVertexBuffers( commandBuffer, 0, gpuPipelineVIBingindCount, vertexBuffers, offsets );
}

void CmdDrawIndexed( VkCommandBuffer commandBuffer, const std::vector<VIBinding>& gpuPipelineVIBindings, const GfxModel& gfxModel )
{
	CmdBindVertexInputs( commandBuffer, gpuPipelineVIBindings, gfxModel );
	vkCmdBindIndexBuffer( commandBuffer, gfxModel.indexBuffer.buffer, 0, gfxModel.indexType );
	vkCmdDrawIndexed( commandBuffer, gfxModel.indexCount, 1, 0, 0, 0 );
}
