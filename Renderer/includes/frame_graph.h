#pragma once
#include "vk_globals.h"
#include "gfx_image.h"
#include "renderpass.h"
#include "swapchain.h"
#include "scene_frame_data.h"
#include "material.h"

namespace FG
{
	constexpr VkExtent2D SWAPCHAIN_SIZED = { 0, 0 };
	#define EXTERNAL_IMAGE {(VkFormat)0,{0,0},( VkImageUsageFlagBits )0, (VkImageAspectFlagBits)0,(VkImageLayout)0,false}
	#define CREATE_IMAGE_COLOR( id, format, extent, usage, swapchainSized ) { (uint32_t)id, eDescriptorType::IMAGE, 1,  eTechniqueDataEntryFlags::NONE, { format , extent, ( VkImageUsageFlagBits )( VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | usage ), VK_IMAGE_ASPECT_COLOR_BIT,  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, swapchainSized }, Samplers::Count }
	#define CREATE_IMAGE_DEPTH( id, format, extent, usage, swapchainSized ) { (uint32_t)id, eDescriptorType::IMAGE, 1,  eTechniqueDataEntryFlags::NONE, { format , extent, ( VkImageUsageFlagBits )( VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | usage ), VK_IMAGE_ASPECT_DEPTH_BIT,  VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, swapchainSized },  Samplers::Count }
	#define CREATE_IMAGE_DEPTH_SAMPLER( id, format, extent, usage, swapchainSized, sampler ) { static_cast< uint32_t >( id ), eDescriptorType::IMAGE_SAMPLER, 1,  eTechniqueDataEntryFlags::NONE, { format , extent, ( VkImageUsageFlagBits )( VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | usage ), VK_IMAGE_ASPECT_DEPTH_BIT,  VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, swapchainSized }, sampler }
	#define CREATE_IMAGE_SAMPLER_EXTERNAL( id, count ){ static_cast< uint32_t >(id), eDescriptorType::IMAGE_SAMPLER, count, eTechniqueDataEntryFlags::EXTERNAL,	EXTERNAL_IMAGE }

	#define CREATE_BUFFER_IMAGE_INTERNAL( objectSize, objectCount ) { (VkFormat)0, {objectSize, objectCount}, ( VkImageUsageFlagBits )0, (VkImageAspectFlagBits)0, (VkImageLayout)0, false }
	#define CREATE_BUFFER( id, size ) { (uint32_t)id, eDescriptorType::BUFFER, 1,  eTechniqueDataEntryFlags::NONE, CREATE_BUFFER_IMAGE_INTERNAL( size, 0 ), Samplers::Count }
	#define CREATE_BUFFER_DYNAMIC( id, objectSize, objectCount ) { (uint32_t)id, eDescriptorType::BUFFER_DYNAMIC, 1,  eTechniqueDataEntryFlags::NONE, CREATE_BUFFER_IMAGE_INTERNAL( objectSize, objectCount ), Samplers::Count }

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
	FrameGraph CreateGraph( const Swapchain* swapchain, std::vector<RenderPassCreationData> *inRpCreationData, std::vector<TechniqueDataEntry> *inRtCreationData, uint32_t backbufferId, VkDescriptorPool descriptorPool );
	void Cleanup( FrameGraph* frameGraph );

	//Graph creation
	void RenderColor(RenderPassCreationData& resource, VkFormat format, uint32_t render_target);
	void RenderDepth(RenderPassCreationData& resource, VkFormat format, uint32_t render_target);
	void ReadResource(RenderPassCreationData& resource, uint32_t render_target);
	void ClearLast(RenderPassCreationData& resource);

	//Frame graph stuff
	void RecordDrawCommands( uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent, FrameGraph* frameGraphExternal );
}