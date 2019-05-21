#pragma once
#include "vk_globals.h"
#include "renderpass.h"
#include "swapchain.h"
#include "scene_frame_data.h"

enum eRenderTarget
{
	RT_SCENE_COLOR = 0,
	RT_SCENE_DEPTH,
	RT_SHADOW_MAP,
	RT_COUNT
};

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
	eRenderTarget e_render_targets[MAX_ATTACHMENTS_COUNT];
	VkAttachmentDescription descriptions[MAX_ATTACHMENTS_COUNT];
	VkAttachmentReference references[MAX_ATTACHMENTS_COUNT];
	uint32_t attachmentCount = 0;

	eRenderTarget read_targets[MAX_READ_TARGETS];
	uint32_t read_targets_count = 0;

	FrameGraphNode frame_graph_node;
	const char* name;
};

void FG_CreateGraph(const Swapchain* swapchain, std::vector<FG_RenderPassCreationData> *inRpCreationData);
const RenderPass* GetRenderPass(uint32_t id);
const GfxImage* GetRenderTarget(eRenderTarget render_target_id);
void FG_RecreateAfterSwapchain(const Swapchain* swapchain);
void FG_CleanupAfterSwapchain();
void FG_CleanupResources();

void FG_CreateColor(FG_RenderPassCreationData& resource, VkFormat format, eRenderTarget render_target);
void FG_CreateDepth(FG_RenderPassCreationData& resource, VkFormat format, eRenderTarget render_target);
void FG_ReadResource(FG_RenderPassCreationData& resource, eRenderTarget render_target);
void FG_ClearLast(FG_RenderPassCreationData& resource);

void FG_RecordDrawCommands(uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent);
