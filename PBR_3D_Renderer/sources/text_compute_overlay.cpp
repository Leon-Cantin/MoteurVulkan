/*
#include "text_compute_overlay.h"

#include "descriptors.h"
#include "file_system.h"
#include "vk_shader.h"

#include <cassert>
#include <array>

VkPipeline textComputePipeline;
VkPipelineLayout textComputePipelineLayout;
VkDescriptorSetLayout textComputeDescriptorSetLayout;
std::vector<VkDescriptorSet> textComputeDescriptorSets;

void createTextComputeDescriptorSetLayout()
{
	const VkDescriptorSetLayoutBinding outputImageBinding = { 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };
	const VkDescriptorSetLayoutBinding samplerLayoutBinding = { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };

	const std::array<VkDescriptorSetLayoutBinding, 2> bindings = { outputImageBinding, samplerLayoutBinding };
	CreateDesciptorSetLayout(bindings.data(), static_cast<uint32_t>(bindings.size()), &textComputeDescriptorSetLayout);
}

void CreateTextComputeDescriptorSet(VkDescriptorPool descriptorPool, const Swapchain& swapchain, VkSampler trilinearSampler)
{
	std::vector<DescriptorSet> descriptorSets;
	descriptorSets.reserve(swapchain.imageCount);
	for (size_t i = 0; i < swapchain.imageCount; ++i)
	{
		DescriptorSet textComputeDescriptorSet = {};
		//textComputeDescriptorSet.storageImageDescriptors.push_back({ swapchain.images[i].imageView, trilinearSampler, 0 });
		assert(false);
		//textComputeDescriptorSet.imageSamplerDescriptors.push_back({ g_fontImage.imageView, trilinearSampler, 1 });
		textComputeDescriptorSet.layout = textComputeDescriptorSetLayout;
		descriptorSets.push_back(textComputeDescriptorSet);
	}
	createDescriptorSets(descriptorPool, descriptorSets.size(), descriptorSets.data());
	textComputeDescriptorSets.resize(swapchain.imageCount);
	textComputeDescriptorSets[0] = descriptorSets[0].set;
	textComputeDescriptorSets[1] = descriptorSets[1].set;
}

static void CreateComputePipeline(std::vector<char>& shaderCode, VkDescriptorSetLayout descriptorSetLayout, VkPipelineLayout* o_pipelineLayout, VkPipeline* o_pipeline)
{
	//Pipeline layout
	//Describe complete set of resources available (image, sampler, ubo, constants, ...)
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1; // Optional
	pipeline_layout_info.pSetLayouts = &descriptorSetLayout; // Optional
	pipeline_layout_info.pushConstantRangeCount = 0; // Optional
	pipeline_layout_info.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(g_vk.device, &pipeline_layout_info, nullptr, o_pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}
	//TODO: dangerous cast due to alligment
	VkShaderModule shaderModule = create_shader_module(reinterpret_cast<uint32_t*>(shaderCode.data()), shaderCode.size());

	VkPipelineShaderStageCreateInfo stageCreateInfo = {};
	stageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stageCreateInfo.pSpecializationInfo = VK_NULL_HANDLE;
	stageCreateInfo.module = shaderModule;
	stageCreateInfo.flags = 0;
	stageCreateInfo.pName = "main";

	VkComputePipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	createInfo.flags = 0;
	createInfo.layout = *o_pipelineLayout;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;
	createInfo.basePipelineIndex = 0;
	createInfo.stage = stageCreateInfo;

	if (vkCreateComputePipelines(g_vk.device, VK_NULL_HANDLE, 1, &createInfo, VK_NULL_HANDLE, o_pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline!");
	}

	vkDestroyShaderModule(g_vk.device, shaderModule, nullptr);
}

void CreateTextComputePipeline()
{
	std::vector<char> shaderCode = FS::readFile("shaders/textCompute.comp.spv");
	CreateComputePipeline(shaderCode, textComputeDescriptorSetLayout, &textComputePipelineLayout, &textComputePipeline);
}

void RecreateTextComputeAfterSwapChain(VkDescriptorPool descriptorPool,const Swapchain& swapchain, VkSampler trilinearSampler)
{
	CreateTextComputePipeline();
	CreateTextComputeDescriptorSet(descriptorPool, swapchain, trilinearSampler);
}

void InitTextCompute(VkDescriptorPool descriptorPool, const Swapchain& swapchain, VkSampler trilinearSampler)
{
	createTextComputeDescriptorSetLayout();
	CreateTextComputePipeline();
	CreateTextComputeDescriptorSet(descriptorPool, swapchain, trilinearSampler);
}

void CleanupTextComputeAfterSwapChain(VkDescriptorPool descriptorPool)
{
	//TODO: remember the pool used for the descriptor
	vkFreeDescriptorSets(g_vk.device, descriptorPool, static_cast<uint32_t>(textComputeDescriptorSets.size()), textComputeDescriptorSets.data());
	vkDestroyPipeline(g_vk.device, textComputePipeline, nullptr);
	vkDestroyPipelineLayout(g_vk.device, textComputePipelineLayout, nullptr);
}

void CleanupTextCompute()
{
	vkDestroyDescriptorSetLayout(g_vk.device, textComputeDescriptorSetLayout, nullptr);
}


void CmdDispatchTextOverlay()
{
	g_computeCommandBuffers.resize(g_outputFramebuffer.size());
	allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = g_vk.computeCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(g_computeCommandBuffers.size());
	if (vkAllocateCommandBuffers(g_vk.device, &allocInfo, g_computeCommandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	for (size_t i = 0; i < g_computeCommandBuffers.size(); i++) {
		VkCommandBuffer computeCommandBuffer = g_computeCommandBuffers[i];
		BeginCommandBufferRecording(computeCommandBuffer);

		vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, textComputePipeline);
		vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, textComputePipelineLayout, 0, 1, &textComputeDescriptorSets[i], 0, nullptr);
		vkCmdDispatch(computeCommandBuffer, 32, 32, 1);

		transitionImageLayout(computeCommandBuffer, g_swapchain.images[i].image, g_swapchain.images[i].format, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1, 1);

		EndCommandBufferRecording(computeCommandBuffer);
	}
}

//Compute
VkSubmitInfo computeSubmitInfo = {};
computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

VkSemaphore computeWaitSemaphores[] = { graphicPassFinishedSemaphores[current_frame] };
VkPipelineStageFlags computeWaitStages[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
computeSubmitInfo.waitSemaphoreCount = 1;
computeSubmitInfo.pWaitSemaphores = computeWaitSemaphores;
computeSubmitInfo.pWaitDstStageMask = computeWaitStages;
computeSubmitInfo.commandBufferCount = 1;
computeSubmitInfo.pCommandBuffers = &g_computeCommandBuffers[current_frame];

VkSemaphore computeSignalSemaphores[] = { renderFinishedSemaphores[current_frame] };
computeSubmitInfo.signalSemaphoreCount = 1;
computeSubmitInfo.pSignalSemaphores = computeSignalSemaphores;
if( vkQueueSubmit(g_vk.compute_queue, 1, &computeSubmitInfo, inFlightFences[current_frame]) != VK_SUCCESS ){//TODO: keep inFlight for the last one
	throw std::runtime_error("failed to submit draw command buffer!");
}*/
