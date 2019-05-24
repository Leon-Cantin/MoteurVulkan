#pragma once
#include "vk_globals.h"
#include "renderpass.h"
#include "swapchain.h"
#include "scene_frame_data.h"

constexpr uint32_t MAX_ATTACHMENTS_COUNT = 8;
constexpr uint32_t MAX_READ_TARGETS = 4;

struct FrameGraphNode
{
	void(*Initialize)(const RenderPass*, const Swapchain* swapchain);
	void(*CleanupAfterSwapchain)();
	void(*RecreateAfterSwapchain)(const Swapchain* swapchain);
	void(*Cleanup)();
	void(*RecordDrawCommands)(uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent);
};

struct FG_RenderPassCreationData
{
	uint32_t e_render_targets[MAX_ATTACHMENTS_COUNT];
	VkAttachmentDescription descriptions[MAX_ATTACHMENTS_COUNT];
	VkAttachmentReference references[MAX_ATTACHMENTS_COUNT];
	uint32_t attachmentCount = 0;

	uint32_t read_targets[MAX_READ_TARGETS];
	uint32_t read_targets_count = 0;

	FrameGraphNode frame_graph_node;
	const char* name;
};

struct FG_RenderTargetCreationData
{
	uint32_t id;
	VkFormat format;
	VkExtent2D extent;
	VkImageUsageFlagBits usage_flags;
	VkImageAspectFlagBits aspect_flags;
	VkImageLayout image_layout;
	bool swapChainSized = false;
};

void FG_CreateGraph(const Swapchain* swapchain, std::vector<FG_RenderPassCreationData> *inRpCreationData, std::vector<FG_RenderTargetCreationData> *inRtCreationData, uint32_t backbufferId);
const RenderPass* GetRenderPass(uint32_t id);
const GfxImage* GetRenderTarget(uint32_t render_target_id);
void FG_RecreateAfterSwapchain(const Swapchain* swapchain);
void FG_CleanupAfterSwapchain();
void FG_CleanupResources();

void FG_CreateColor(FG_RenderPassCreationData& resource, VkFormat format, uint32_t render_target);
void FG_CreateDepth(FG_RenderPassCreationData& resource, VkFormat format, uint32_t render_target);
void FG_ReadResource(FG_RenderPassCreationData& resource, uint32_t render_target);
void FG_ClearLast(FG_RenderPassCreationData& resource);

void FG_RecordDrawCommands(uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent);
