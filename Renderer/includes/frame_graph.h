#pragma once
#include "vk_globals.h"
#include "gfx_image.h"
#include "material.h"
#include "bindings.h"

namespace FG
{
	enum eDataEntryFlags
	{
		NONE = 1 << 0,
		EXTERNAL = 1 << 1,
	};

	#define EXTERNAL_IMAGE {GfxFormat::UNDEFINED,{0,0}, ( GfxImageUsageFlags )0 }
	#define CREATE_IMAGE_COLOR_SAMPLER( id, format, extent, usage, sampler ) { (uint32_t)id, eDescriptorType::IMAGE_SAMPLER, 1,  FG::eDataEntryFlags::NONE, { format , extent, ( GfxImageUsageFlags )( GfxImageUsageFlagBits::COLOR_ATTACHMENT | usage ) }, sampler }
	#define CREATE_IMAGE_COLOR( id, format, extent, usage, flags ) { (uint32_t)id, eDescriptorType::IMAGE, 1,  flags, { format , extent, ( GfxImageUsageFlags )( GfxImageUsageFlagBits::COLOR_ATTACHMENT | usage ) }, eSamplers::Count }
	#define CREATE_IMAGE_DEPTH( id, format, extent, usage ) { (uint32_t)id, eDescriptorType::IMAGE, 1,  FG::eDataEntryFlags::NONE, { format , extent, ( GfxImageUsageFlags )( GfxImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT | usage ) },  eSamplers::Count }
	#define CREATE_IMAGE_DEPTH_SAMPLER( id, format, extent, usage, sampler ) { static_cast< uint32_t >( id ), eDescriptorType::IMAGE_SAMPLER, 1,  FG::eDataEntryFlags::NONE, { format , extent, ( GfxImageUsageFlags )( GfxImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT | usage ) }, sampler }
	#define CREATE_IMAGE_SAMPLER_EXTERNAL( id, count ){ static_cast< uint32_t >(id), eDescriptorType::IMAGE_SAMPLER, count, FG::eDataEntryFlags::EXTERNAL,	EXTERNAL_IMAGE }

	#define CREATE_BUFFER_IMAGE_INTERNAL( objectSize, objectCount ) { GfxFormat::UNDEFINED, {objectSize, objectCount}, ( GfxImageUsageFlags )0 }
	#define CREATE_BUFFER( id, size ) { (uint32_t)id, eDescriptorType::BUFFER, 1,  FG::eDataEntryFlags::NONE, CREATE_BUFFER_IMAGE_INTERNAL( size, 0 ), eSamplers::Count }
	#define CREATE_BUFFER_DYNAMIC( id, objectSize, objectCount ) { (uint32_t)id, eDescriptorType::BUFFER_DYNAMIC, 1,  FG::eDataEntryFlags::NONE, CREATE_BUFFER_IMAGE_INTERNAL( objectSize, objectCount ), eSamplers::Count }

	constexpr uint32_t MAX_ATTACHMENTS_COUNT = 8;
	constexpr uint32_t MAX_READ_TARGETS = 4;

	typedef uint32_t fg_handle_t;
	typedef uint32_t user_id_t;

	struct RenderTargetRef
	{
		fg_handle_t resourceHandle;
		uint32_t flags;
	};

	struct TaskInputData
	{
		void* userData;
		uint32_t currentFrame;
		VkExtent2D extent;
		const RenderPass* renderpass;
		const Technique * technique;
	};

	struct FrameGraphNode
	{
		void( *RecordDrawCommands )( GfxCommandBuffer graphicsCommandBuffer, const TaskInputData& inputData );
		std::vector<GfxDescriptorTableDesc> descriptorSets;
		std::vector<RenderTargetRef> renderTargetRefs;
		GpuPipelineLayout gpuPipelineLayout;
		GpuPipelineStateDesc gpuPipelineStateDesc;
	};

	struct RenderPassCreationData
	{
		fg_handle_t fgHandleAttachement[MAX_ATTACHMENTS_COUNT];
		AttachementDescription descriptions[MAX_ATTACHMENTS_COUNT];
		uint32_t attachmentCount = 0;

		uint32_t read_targets[MAX_READ_TARGETS];
		uint32_t read_targets_count = 0;

		FrameGraphNode frame_graph_node;
		const char* name;
	};

	struct ResourceDesc
	{
		GfxFormat format;
		VkExtent2D extent;
		GfxImageUsageFlags usage_flags;
	};

	struct DataEntry
	{
		user_id_t user_id;
		eDescriptorType descriptorType;
		uint32_t count;
		uint32_t flags;
		ResourceDesc resourceDesc;
		eSamplers sampler;
	};

	enum RenderTargetRefFlags
	{
		FG_RENDERTARGET_REF_CLEAR_BIT = 1 << 0,
		FG_RENDERTARGET_REF_READ_BIT = 1 << 1,
	};

	class FrameGraph
	{
	public:
		class FrameGraphInternal* imp;
		FrameGraph( class FrameGraphInternal* );
		FrameGraph();
		const RenderPass* GetRenderPass( uint32_t id );
		const GfxImage* GetImageFromId( user_id_t render_target_id );
		void AddExternalImage( fg_handle_t handle, uint32_t frameIndex, const GfxImage& image );
	};

	//Compilation
	FrameGraph CreateGraph( std::vector<RenderPassCreationData> *inRpCreationData, std::vector<DataEntry> *inRtCreationData, fg_handle_t backbuffer_fg_handle );
	void Cleanup( FrameGraph* frameGraph );
	void CreateRenderPasses( FrameGraph* frameGraphExternal );


	//Frame graph stuff
	void RecordDrawCommands( uint32_t currentFrame, void* userData, GfxCommandBuffer graphicsCommandBuffer, VkExtent2D extent, FrameGraph* frameGraphExternal );
}