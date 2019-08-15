#include "renderer.h"

#include "vk_buffer.h"
#include "vk_debug.h"
#include "scene_instance.h"
#include "vk_commands.h"
#include "profile.h"
#include "vk_framework.h"
#include "console_command.h"
#include "gpu_synchronization.h"
#include "window_handler.h"
#include "frame_graph.h"

#include <array>
#include <iostream>

extern bool framebuffer_resized;

Swapchain g_swapchain;
std::array<VkCommandBuffer, SIMULTANEOUS_FRAMES> g_graphicsCommandBuffers;
std::array<VkCommandBuffer, SIMULTANEOUS_FRAMES> g_transferCommandBuffers;
std::array<VkCommandBuffer, SIMULTANEOUS_FRAMES> g_computeCommandBuffers;

std::array<VkSemaphore, SIMULTANEOUS_FRAMES> graphicPassFinishedSemaphores;
std::array<VkSemaphore, SIMULTANEOUS_FRAMES> imageAvailableSemaphores;
std::array<VkSemaphore, SIMULTANEOUS_FRAMES> renderFinishedSemaphores;
std::array<VkFence, SIMULTANEOUS_FRAMES> inFlightFences;

void CreateCommandBuffer()
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

void CreateTransferCommandBuffer()
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

void create_sync_objects()
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

VkFormat findDepthFormat() {
	return findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

void cleanup_swap_chain()
{
	vkFreeCommandBuffers(g_vk.device, g_vk.graphicsCommandPool, static_cast<uint32_t>(g_graphicsCommandBuffers.size()), g_graphicsCommandBuffers.data());
	vkFreeCommandBuffers(g_vk.device, g_vk.transferCommandPool, static_cast<uint32_t>(g_transferCommandBuffers.size()), g_transferCommandBuffers.data());
	vkFreeCommandBuffers(g_vk.device, g_vk.computeCommandPool, static_cast<uint32_t>(g_computeCommandBuffers.size()), g_computeCommandBuffers.data());

	FG::CleanupAfterSwapchain();

	for (auto image : g_swapchain.images)
		vkDestroyImageView(g_vk.device, image.imageView, nullptr);

	vkDestroySwapchainKHR(g_vk.device, g_swapchain.vkSwapchain, nullptr);
}

void recreate_swap_chain()
{
	//TODO: find a better way of handling window minimization
	uint64_t width = 0, height = 0;
	while (width == 0 || height == 0) {
		WH::GetFramebufferSize( &width, &height );
		//TODO
		//glfwWaitEvents();
	}

	framebuffer_resized = false;

	vkDeviceWaitIdle(g_vk.device);

	cleanup_swap_chain();

	//TODO: try to use the "oldSwapchain" parameter to optimize when recreating swap chains
	WH::GetFramebufferSize(&width, &height);
	createSwapChain(g_vk.windowSurface, width, height, g_swapchain);

	FG::RecreateAfterSwapchain(&g_swapchain);

	CreateCommandBuffer();
	CreateTransferCommandBuffer();
}

void InitRenderer()
{
	uint64_t width, height;
	WH::GetFramebufferSize(&width, &height);
	createSwapChain(g_vk.windowSurface, width, height, g_swapchain);

	QueueFamilyIndices queue_family_indices = find_queue_families(g_vk.physicalDevice);
	CreateCommandPool(queue_family_indices.graphics_family.value(), &g_vk.graphicsCommandPool);
	CreateCommandPool(queue_family_indices.transfer_family.value(), &g_vk.transferCommandPool);
	CreateSingleUseCommandPool(queue_family_indices.graphics_family.value(), &g_vk.graphicsSingleUseCommandPool);
	CreateSingleUseCommandPool(queue_family_indices.compute_family.value(), &g_vk.computeCommandPool);

	InitSamplers();

	CreateCommandBuffer();
	create_sync_objects();

	CreateTimeStampsQueryPool(SIMULTANEOUS_FRAMES);
}

void CompileFrameGraph( void( *FGScriptInitialize )(const Swapchain* swapchain) )
{
	FGScriptInitialize( &g_swapchain );
}

bool verify_swap_chain(VkResult result)
{
	if (result != VK_SUCCESS)
		throw std::runtime_error("failed to acquire swap chain image");

	return result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized ? false : true;
}

void RecordCommandBuffer(uint32_t currentFrame, const SceneFrameData* frameData)
{
	VkCommandBuffer graphicsCommandBuffer = g_graphicsCommandBuffers[currentFrame];
	BeginCommandBufferRecording(graphicsCommandBuffer);

	CmdResetTimeStampSet(graphicsCommandBuffer, currentFrame);

	CmdWriteTimestamp(graphicsCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, Timestamp::COMMAND_BUFFER_START, currentFrame);

	FG::RecordDrawCommands(currentFrame, frameData, graphicsCommandBuffer, g_swapchain.extent);

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
	if (!verify_swap_chain(result)) {
		unsignalSemaphore(imageAvailableSemaphores[currentFrame]);
		recreate_swap_chain();
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
		recreate_swap_chain();
	}
}

void CleanupRenderer() {
	cleanup_swap_chain();

	DestroySamplers();

	FG::CleanupResources();

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
