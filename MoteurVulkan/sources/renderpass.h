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

struct VICreation
{
	const VkVertexInputBindingDescription * vibDescription;
	uint32_t vibDescriptionsCount;
	const VkVertexInputAttributeDescription* visDescriptions;
	uint32_t visDescriptionsCount;
};

struct ShaderCreation
{
	const uint32_t* code;
	size_t length;
	const char* entryPoint;
	VkShaderStageFlagBits flags;
};

struct RasterizationState
{
	bool depthBiased;
	bool backFaceCulling;
};

struct DepthStencilState
{
	bool depthRead;
	bool depthWrite;
	VkCompareOp depthCompareOp;
};

void CreatePipeline(
	const VICreation& viCreation,
	const std::vector<ShaderCreation>& shaders,
	VkExtent2D framebufferExtent,
	VkRenderPass renderPass,
	VkPipelineLayout pipelineLayout,
	RasterizationState rasterizationState,
	DepthStencilState depthStencilState,
	bool blendEnabled,
	VkPrimitiveTopology primitiveTopology,
	VkPipeline* o_pipeline);

void BeginRenderPass(VkCommandBuffer commandBuffer, const RenderPass& renderpass, VkFramebuffer framebuffer, VkExtent2D extent);
void EndRenderPass(VkCommandBuffer commandBuffer);