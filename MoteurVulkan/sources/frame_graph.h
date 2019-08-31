#pragma once
#include "vk_globals.h"
#include "renderpass.h"
#include "swapchain.h"
#include "scene_frame_data.h"
#include "material.h"

namespace FG
{
	constexpr uint32_t MAX_ATTACHMENTS_COUNT = 8;
	constexpr uint32_t MAX_READ_TARGETS = 4;

	struct FrameGraphNode
	{
		void( *RecordDrawCommands )(uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent, const RenderPass * renderpass, const Technique * technique);
		TechniqueDescriptorSetDesc* passSet;
		TechniqueDescriptorSetDesc* instanceSet;
		GpuPipelineLayout gpuPipelineLayout;
		GpuPipelineState gpuPipelineState;
	};

	struct RenderPassCreationData
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

	struct RenderTargetCreationData
	{
		uint32_t id;
		VkFormat format;
		VkExtent2D extent;
		VkImageUsageFlagBits usage_flags;
		VkImageAspectFlagBits aspect_flags;
		VkImageLayout image_layout;
		bool swapChainSized = false;
	};

	void CreateGraph( const Swapchain* swapchain, std::vector<RenderPassCreationData> *inRpCreationData, std::vector<RenderTargetCreationData> *inRtCreationData, uint32_t backbufferId, VkDescriptorPool descriptorPool,
		void( *createTechniqueCallback )(const RenderPass*, const RenderPassCreationData*, Technique*) );
	const RenderPass* GetRenderPass(uint32_t id);
	const GfxImage* GetRenderTarget(uint32_t render_target_id);
	void Cleanup();

	void CreateColor(RenderPassCreationData& resource, VkFormat format, uint32_t render_target);
	void CreateDepth(RenderPassCreationData& resource, VkFormat format, uint32_t render_target);
	void ReadResource(RenderPassCreationData& resource, uint32_t render_target);
	void ClearLast(RenderPassCreationData& resource);

	void RecordDrawCommands(uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent);
}