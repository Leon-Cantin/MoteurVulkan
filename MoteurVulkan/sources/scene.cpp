#include "scene.h"

#include "swapchain.h"
#include "framebuffer.h"
#include "vk_buffer.h"
#include "vk_debug.h"
#include "scene_instance.h"
#include "shadow_renderpass.h"
#include "geometry_renderpass.h"
#include "text_overlay.h"
#include "skybox.h"
#include "descriptors.h"
#include "vk_commands.h"
#include "profile.h"
#include "vk_framework.h"
#include "console_command.h"
#include "gpu_synchronization.h"
#include "frame_graph.h"

#include <array>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>


extern bool framebuffer_resized;

Swapchain g_swapchain;
std::array<FrameBuffer, SIMULTANEOUS_FRAMES> g_outputFramebuffer;
std::array<VkCommandBuffer, SIMULTANEOUS_FRAMES> g_graphicsCommandBuffers;
std::array<VkCommandBuffer, SIMULTANEOUS_FRAMES> g_transferCommandBuffers;
std::array<VkCommandBuffer, SIMULTANEOUS_FRAMES> g_computeCommandBuffers;
GfxImage depthImage;

VkDescriptorPool descriptorPool;

std::array<VkSemaphore, SIMULTANEOUS_FRAMES> graphicPassFinishedSemaphores;
std::array<VkSemaphore, SIMULTANEOUS_FRAMES> imageAvailableSemaphores;
std::array<VkSemaphore, SIMULTANEOUS_FRAMES> renderFinishedSemaphores;
std::array<VkFence, SIMULTANEOUS_FRAMES> inFlightFences;

PerFrameBuffer sceneUniformBuffer;
PerFrameBuffer lightUniformBuffer;

PerFrameBuffer instanceMatricesBuffer;

void CreateInstanceMatricesBuffers()
{
	uint32_t modelsCount = 3;
	CreatePerFrameBuffer(sizeof(InstanceMatrices)*modelsCount, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &instanceMatricesBuffer);
}

void createOutputFrameBuffer()
{
	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i) {
		//Pick any renderpass that matches
		createFrameBuffer(&g_swapchain.images[i], 1, &depthImage, g_swapchain.extent, GetGeometryRenderPass(), &g_outputFramebuffer[i]);
	}
}

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

void createDepthResources()
{
	depthImage.extent = g_swapchain.extent;
	depthImage.format = VK_FORMAT_D32_SFLOAT;
	create_image(depthImage.extent.width, depthImage.extent.height, 1, depthImage.format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage.image, depthImage.memory);
	depthImage.imageView = createImageView(depthImage.image, depthImage.format, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
	transitionImageLayout(depthImage.image, depthImage.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 1);
}

void CreateGeometryUniformBuffer()
{
	CreatePerFrameBuffer(sizeof(SceneMatricesUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &sceneUniformBuffer);
	CreatePerFrameBuffer(sizeof(LightUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &lightUniformBuffer);
}

void CreateGeometryRenderpassDescriptorSet(const GfxImage* albedoImage, const GfxImage* normalImage)
{
	//TODO review how to pass shadowmaps
	const GfxImage *shadowImages = GetShadowDepthImage();

	CreateGeometryDescriptorSet(descriptorPool, sceneUniformBuffer.buffers.data(), instanceMatricesBuffer.buffers.data(), lightUniformBuffer.buffers.data(), albedoImage->imageView, normalImage->imageView, GetSampler(Samplers::Trilinear),
		shadowImages->imageView, GetSampler(Samplers::Shadow));

	CreateShadowDescriptorSet(descriptorPool, instanceMatricesBuffer.buffers.data());
}

void CreateGeometryInstanceDescriptorSet( SceneInstanceSet* sceneInstanceDescriptorSet, uint32_t hackIndex)
{
	CreateSceneInstanceDescriptorSet( sceneInstanceDescriptorSet, hackIndex);
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
	DestroyImage(depthImage);

	for (auto framebuffer : g_outputFramebuffer)
		vkDestroyFramebuffer(g_vk.device, framebuffer.frameBuffer, nullptr);

	vkFreeCommandBuffers(g_vk.device, g_vk.graphicsCommandPool, static_cast<uint32_t>(g_graphicsCommandBuffers.size()), g_graphicsCommandBuffers.data());
	vkFreeCommandBuffers(g_vk.device, g_vk.transferCommandPool, static_cast<uint32_t>(g_transferCommandBuffers.size()), g_transferCommandBuffers.data());
	vkFreeCommandBuffers(g_vk.device, g_vk.computeCommandPool, static_cast<uint32_t>(g_computeCommandBuffers.size()), g_computeCommandBuffers.data());

	CleanupGeometryRenderpassAfterSwapchain();

	CleanupSkyboxAfterSwapchain();

	CleanupTextRenderPassAfterSwapchain();

	for (auto image : g_swapchain.images)
		vkDestroyImageView(g_vk.device, image.imageView, nullptr);

	vkDestroySwapchainKHR(g_vk.device, g_swapchain.vkSwapchain, nullptr);
}

void recreate_swap_chain()
{
	//TODO: find a better way of handling window minimization
	int width = 0, height = 0;
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(g_window, &width, &height);
		glfwWaitEvents();
	}

	framebuffer_resized = false;

	vkDeviceWaitIdle(g_vk.device);

	cleanup_swap_chain();

	//TODO: try to use the "oldSwapchain" parameter to optimize when recreating swap chains
	glfwGetFramebufferSize(g_window, &width, &height);
	createSwapChain(g_windowSurface, width, height, g_swapchain);

	create_skybox_graphics_pipeline(g_swapchain.extent);

	createGeoGraphicPipeline(g_swapchain.extent);//Could be avoided with dynamic state, we only need to redo scissor and viewport

	RecreateTextRenderPass(g_swapchain);

	createDepthResources();
	createOutputFrameBuffer();
	CreateCommandBuffer();
	CreateTransferCommandBuffer();
}

void InitSkybox(const GfxImage* skyboxImage)
{
	createSkyboxDescriptorSetLayout();
	createSkyboxUniformBuffers();
	CreateSkyboxDescriptorSet(descriptorPool, skyboxImage->imageView, GetSampler(Samplers::Trilinear));
	create_skybox_graphics_pipeline(g_swapchain.extent);
}

void InitScene()
{
	int width, height;
	glfwGetFramebufferSize(g_window, &width, &height);
	createSwapChain(g_windowSurface, width, height, g_swapchain);

	std::vector<RenderPass> renderPasses;
	CreateGraph(g_swapchain.surfaceFormat.format, &renderPasses);

	createGeoDescriptorSetLayout();
	AddGeometryRenderPass(renderPasses[1]);
	createGeoGraphicPipeline(g_swapchain.extent);
	CreateInstanceMatricesBuffers();

	AddSkyboxRenderPass(renderPasses[2]);

	QueueFamilyIndices queue_family_indices = find_queue_families(g_vk.physicalDevice);
	CreateCommandPool(queue_family_indices.graphics_family.value(), &g_vk.graphicsCommandPool);
	CreateCommandPool(queue_family_indices.transfer_family.value(), &g_vk.transferCommandPool);
	CreateSingleUseCommandPool(queue_family_indices.graphics_family.value(), &g_vk.graphicsSingleUseCommandPool);
	CreateSingleUseCommandPool(queue_family_indices.compute_family.value(), &g_vk.computeCommandPool);

	createDepthResources();
	createOutputFrameBuffer();

	InitSamplers();

	CreateGeometryUniformBuffer();
	const uint32_t geometryDescriptorSets = 2 * SIMULTANEOUS_FRAMES;
	const uint32_t geometryBuffersCount =  2 * SIMULTANEOUS_FRAMES;
	const uint32_t geometryImageCount = 3 * SIMULTANEOUS_FRAMES;
	const uint32_t geomtryDynamicBuffersCount = 1 * SIMULTANEOUS_FRAMES;

	const uint32_t shadowDescriptorSets = 2 * SIMULTANEOUS_FRAMES;
	const uint32_t shadowBuffersCount = 2 * SIMULTANEOUS_FRAMES;
	const uint32_t shadowDynamicBuffersCount = 1 * SIMULTANEOUS_FRAMES;

	const uint32_t skyboxDescriptorSetsCount = 1 * SIMULTANEOUS_FRAMES;
	const uint32_t skyboxBuffersCount = 1 * SIMULTANEOUS_FRAMES;
	const uint32_t skyboxImageCount = 1 * SIMULTANEOUS_FRAMES;

	const uint32_t textDescriptorSetsCount = 1;
	const uint32_t textImageCount = 1;

	const uint32_t uniformBuffersCount = geometryBuffersCount + shadowBuffersCount + skyboxBuffersCount;
	const uint32_t imageSamplersCount = geometryImageCount + skyboxImageCount + textImageCount;
	const uint32_t storageImageCount = 1;
	const uint32_t uniformBuffersDynamicCount = geomtryDynamicBuffersCount + shadowDynamicBuffersCount;

	const uint32_t maxSets = (geometryDescriptorSets + shadowDescriptorSets + skyboxDescriptorSetsCount + textDescriptorSetsCount);
	createDescriptorPool(uniformBuffersCount, uniformBuffersDynamicCount, imageSamplersCount, storageImageCount, maxSets, &descriptorPool);

	AddShadowRenderPass(renderPasses[0]);
	CreateShadowPass();

	LoadFontTexture();
	AddTextRenderPass(renderPasses[3]);
	InitTextRenderPass(g_swapchain);
	CreateTextVertexBuffer(256);
	CreateTextDescriptorSet(descriptorPool, GetSampler(Samplers::Trilinear));

	CreateCommandBuffer();
	create_sync_objects();

	CreateTimeStampsQueryPool(SIMULTANEOUS_FRAMES);
}

bool verify_swap_chain(VkResult result)
{
	if (result != VK_SUCCESS)
		throw std::runtime_error("failed to acquire swap chain image");

	return result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized ? false : true;
}

void UpdateGeometryUniformBuffer(const SceneInstance* sceneInstance, const SceneInstanceSet* sceneInstanceDescriptorSet, uint32_t currentFrame)
{
	InstanceMatrices instanceMatrices = {};
	instanceMatrices.model = ComputeSceneInstanceModelMatrix(*sceneInstance);

	VkDeviceSize frameMemoryOffset = GetMemoryOffsetForFrame(&instanceMatricesBuffer, currentFrame);
	void* data;
	vkMapMemory(g_vk.device, instanceMatricesBuffer.memory, sceneInstanceDescriptorSet->geometryBufferOffsets[currentFrame] + frameMemoryOffset, sizeof(InstanceMatrices), 0, &data);
	memcpy(data, &instanceMatrices, sizeof(InstanceMatrices));
	vkUnmapMemory(g_vk.device, instanceMatricesBuffer.memory);

}

void UpdateLightUniformBuffer( const SceneMatricesUniform* shadowSceneMatrices, LightUniform* light, uint32_t currentFrame)
{
	const glm::mat4 biasMat = {
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.5, 0.5, 0.0, 1.0 };
	light->shadowMatrix = biasMat * shadowSceneMatrices->proj * shadowSceneMatrices->view;
	UpdatePerFrameBuffer(&lightUniformBuffer, light, sizeof(LightUniform), currentFrame);

	UpdateShadowUniformBuffers( currentFrame, shadowSceneMatrices);
}

void UpdateSceneUniformBuffer(const glm::mat4& world_view_matrix, VkExtent2D extent, uint32_t currentFrame)
{
	SceneMatricesUniform sceneMatrices = {};
	sceneMatrices.view = world_view_matrix;
	sceneMatrices.proj = glm::perspective(glm::radians(45.0f), extent.width / (float)extent.height, 0.1f, 10.0f);
	sceneMatrices.proj[1][1] *= -1;//Compensate for OpenGL Y coordinate being inverted
	UpdatePerFrameBuffer(&sceneUniformBuffer, &sceneMatrices, sizeof(sceneMatrices), currentFrame);
}

void RecordCommandBuffer(uint32_t currentFrame, const SceneFrameData* frameData)
{
	VkCommandBuffer graphicsCommandBuffer = g_graphicsCommandBuffers[currentFrame];
	BeginCommandBufferRecording(graphicsCommandBuffer);

	CmdResetTimeStampSet(graphicsCommandBuffer, currentFrame);

	CmdWriteTimestamp(graphicsCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, Timestamp::COMMAND_BUFFER_START, currentFrame);

	CmdBeginShadowPass(graphicsCommandBuffer, currentFrame);
	for (size_t i = 0; i < frameData->renderableAssets.size(); ++i)
	{
		const SceneRenderableAsset* renderable = frameData->renderableAssets[i];
		CmdDrawShadowPass(graphicsCommandBuffer, renderable->descriptorSet, renderable->modelAsset, currentFrame);
	}
	CmdEndShadowPass(graphicsCommandBuffer);

	CmdBeginGeometryRenderPass(graphicsCommandBuffer, g_outputFramebuffer[currentFrame].frameBuffer, g_swapchain.extent, currentFrame);
	for (size_t i = 0; i < frameData->renderableAssets.size(); ++i)
	{
		const SceneRenderableAsset* renderable = frameData->renderableAssets[i];
		CmdDrawModelAsset(graphicsCommandBuffer, renderable->descriptorSet, *renderable->modelAsset, currentFrame);
	}
	CmdEndGeometryRenderPass(graphicsCommandBuffer);

	CmdDrawSkybox(graphicsCommandBuffer, g_outputFramebuffer[currentFrame].frameBuffer, g_swapchain.extent, currentFrame);

	CmdDrawText(graphicsCommandBuffer, g_outputFramebuffer[currentFrame].frameBuffer, g_swapchain.extent, currentFrame);

	CmdWriteTimestamp(graphicsCommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, Timestamp::COMMAND_BUFFER_END, currentFrame);

	EndCommandBufferRecording(graphicsCommandBuffer);
}

void ReloadSceneShaders()
{
	vkDeviceWaitIdle(g_vk.device);
	VkExtent2D extent = g_outputFramebuffer[0].extent;
	ReloadSkyboxShaders(extent);
	ReloadGeometryShaders(extent);
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

void CleanupScene() {
	cleanup_swap_chain();

	DestroyTimeStampsPool();
	vkDestroyDescriptorPool(g_vk.device, descriptorPool, nullptr);

	CleanupGeometryRenderpass();

	DestroyPerFrameBuffer(&lightUniformBuffer);
	DestroyPerFrameBuffer(&sceneUniformBuffer);
	DestroyPerFrameBuffer(&instanceMatricesBuffer);

	CleanupSkybox();

	DestroySamplers();

	CleanupTextRenderPass();

	CleanupShadowPass();

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
}
