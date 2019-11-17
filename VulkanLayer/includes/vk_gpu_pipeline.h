#pragma once

#include "vk_globals.h"
#include "vk_vertex_input.h"
#include <vector>

constexpr uint32_t VI_STATE_MAX_DESCRIPTIONS = 5;
struct VIState
{
	VkVertexInputBindingDescription vibDescription[VI_STATE_MAX_DESCRIPTIONS];
	uint32_t vibDescriptionsCount;
	VkVertexInputAttributeDescription visDescriptions[VI_STATE_MAX_DESCRIPTIONS];
	uint32_t visDescriptionsCount;
};

struct ShaderCreation
{
	std::vector<char> code;
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

struct GpuPipelineLayout
{
	std::vector<VkPushConstantRange> pushConstantRanges;
};

struct GpuPipelineState
{
	VIState viState;
	std::vector<ShaderCreation> shaders;
	VkExtent2D framebufferExtent;
	RasterizationState rasterizationState;
	DepthStencilState depthStencilState;
	bool blendEnabled;
	VkPrimitiveTopology primitiveTopology;
};

uint32_t GetBindingDescription( const std::vector<VIBinding>& VIBindings, VIState* o_viState );
void CreatePipeline( const GpuPipelineState& gpuPipeline, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkPipeline* o_pipeline );
