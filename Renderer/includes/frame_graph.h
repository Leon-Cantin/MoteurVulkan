#pragma once
#include "vk_globals.h"
#include "gfx_image.h"
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

	struct ResourceDesc
	{
		VkFormat format;
		VkExtent2D extent;
		VkImageUsageFlagBits usage_flags;
		VkImageAspectFlagBits aspect_flags;
		VkImageLayout image_layout;
		bool swapChainSized = false;
	};

	struct TechniqueDataEntry
	{
		uint32_t id;
		eDescriptorType descriptorType;
		uint32_t count;
		uint32_t flags;
		ResourceDesc resourceDesc;
		Samplers sampler;
	};

	class FrameGraph
	{
	public:
		class FrameGraphInternal* imp;
		FrameGraph( class FrameGraphInternal* );
		FrameGraph();
		const RenderPass* GetRenderPass( uint32_t id );
		const GfxImage* GetImage( uint32_t render_target_id );
	};

	//Compilation
	FrameGraph CreateGraph( const Swapchain* swapchain, std::vector<RenderPassCreationData> *inRpCreationData, std::vector<TechniqueDataEntry> *inRtCreationData, uint32_t backbufferId, VkDescriptorPool descriptorPool,
		void( *createTechniqueCallback )(const RenderPass*, const RenderPassCreationData*, Technique*, FrameGraph*) );
	void Cleanup( FrameGraph* frameGraph );

	//Graph creation
	void RenderColor(RenderPassCreationData& resource, VkFormat format, uint32_t render_target);
	void RenderDepth(RenderPassCreationData& resource, VkFormat format, uint32_t render_target);
	void ReadResource(RenderPassCreationData& resource, uint32_t render_target);
	void ClearLast(RenderPassCreationData& resource);

	//Frame graph stuff
	void RecordDrawCommands( uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent, FrameGraph* frameGraphExternal );
}