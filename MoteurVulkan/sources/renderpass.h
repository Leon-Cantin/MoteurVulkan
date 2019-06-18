#pragma once
#include "vk_globals.h"

#include "framebuffer.h"

#include <vector>

struct RenderPass {
	VkRenderPass vk_renderpass;
	std::vector<VkFormat> colorFormats;
	VkFormat depthFormat;
	FrameBuffer frameBuffer;
	FrameBuffer outputFrameBuffer[SIMULTANEOUS_FRAMES];
};

void CreatePipeline(
	const VkVertexInputBindingDescription * vibDescription,
	uint32_t vibDescriptionsCount,
	const VkVertexInputAttributeDescription* visDescriptions,
	uint32_t visDescriptionsCount,
	std::vector<char>& vertShaderCode, std::vector<char>& fragShaderCode,
	VkExtent2D framebufferExtent,
	VkRenderPass renderPass,
	VkPipelineLayout pipelineLayout,
	bool depthBiased,
	bool depthRead,
	bool depthWrite,
	bool blendEnabled,
	bool backFaceCulling,
	VkPrimitiveTopology primitiveTopology,
	VkCompareOp depthCompareOp,
	VkPipeline* o_pipeline);

void BeginRenderPass(VkCommandBuffer commandBuffer, const RenderPass& renderpass, VkFramebuffer framebuffer, VkExtent2D extent);
void EndRenderPass(VkCommandBuffer commandBuffer);