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

	#define EXTERNAL_IMAGE { R_HW::GfxFormat::UNDEFINED,{0,0}, ( R_HW::GfxImageUsageFlags )0 }
	#define CREATE_IMAGE_COLOR_SAMPLER( id, format, extent, usage, sampler ) { (uint32_t)id, R_HW::eDescriptorType::IMAGE_SAMPLER, 1,  FG::eDataEntryFlags::NONE, { format , extent, ( R_HW::GfxImageUsageFlags )( R_HW::GfxImageUsageFlagBits::COLOR_ATTACHMENT | usage ) }, sampler }
	#define CREATE_IMAGE_COLOR( id, format, extent, usage, flags ) { (uint32_t)id, R_HW::eDescriptorType::IMAGE, 1,  flags, { format , extent, ( R_HW::GfxImageUsageFlags )( R_HW::GfxImageUsageFlagBits::COLOR_ATTACHMENT | usage ) }, eSamplers::Count }
	#define CREATE_IMAGE_DEPTH( id, format, extent, usage ) { (uint32_t)id, R_HW::eDescriptorType::IMAGE, 1,  FG::eDataEntryFlags::NONE, { format , extent, ( R_HW::GfxImageUsageFlags )( R_HW::GfxImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT | usage ) },  eSamplers::Count }
	#define CREATE_IMAGE_DEPTH_SAMPLER( id, format, extent, usage, sampler ) { static_cast< uint32_t >( id ), R_HW::eDescriptorType::IMAGE_SAMPLER, 1,  FG::eDataEntryFlags::NONE, { format , extent, ( R_HW::GfxImageUsageFlags )( R_HW::GfxImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT | usage ) }, sampler }
	#define CREATE_IMAGE_SAMPLER_EXTERNAL( id, count ){ static_cast< uint32_t >(id), R_HW::eDescriptorType::IMAGE_SAMPLER, count, FG::eDataEntryFlags::EXTERNAL,	EXTERNAL_IMAGE }
	#define CREATE_SAMPLER_EXTERNAL( id, count ){ static_cast< uint32_t >(id), R_HW::eDescriptorType::SAMPLER, count, FG::eDataEntryFlags::EXTERNAL,	EXTERNAL_IMAGE }
	#define CREATE_IMAGE_EXTERNAL( id, count ){ static_cast< uint32_t >(id), R_HW::eDescriptorType::IMAGE, count, FG::eDataEntryFlags::EXTERNAL,	EXTERNAL_IMAGE }

	#define CREATE_BUFFER_IMAGE_INTERNAL( objectSize, objectCount ) { R_HW::GfxFormat::UNDEFINED, {objectSize, objectCount}, ( R_HW::GfxImageUsageFlags )0 }
	#define CREATE_BUFFER( id, size ) { (uint32_t)id, R_HW::eDescriptorType::BUFFER, 1,  FG::eDataEntryFlags::NONE, CREATE_BUFFER_IMAGE_INTERNAL( size, 0 ), eSamplers::Count }
	#define CREATE_BUFFER_DYNAMIC( id, objectSize, objectCount ) { (uint32_t)id, R_HW::eDescriptorType::BUFFER_DYNAMIC, 1,  FG::eDataEntryFlags::NONE, CREATE_BUFFER_IMAGE_INTERNAL( objectSize, objectCount ), eSamplers::Count }

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
		const R_HW::RenderPass* renderpass;
		const Technique * technique;
	};

	struct DataBinding
	{
		fg_handle_t resourceHandle;
		R_HW::GfxDataBinding desc;
	};

	struct DescriptorTableDesc
	{
		uint32_t binding;
		std::vector<DataBinding> dataBindings;
	};

	struct FrameGraphNode
	{
		void( *RecordDrawCommands )(R_HW::GfxCommandBuffer graphicsCommandBuffer, const TaskInputData& inputData );
		std::vector<DescriptorTableDesc> descriptorSets;
		std::vector<RenderTargetRef> renderTargetRefs;
		R_HW::GpuPipelineLayout gpuPipelineLayout;
		R_HW::GpuPipelineStateDesc gpuPipelineStateDesc;
	};

	struct RenderPassCreationData
	{
		fg_handle_t fgHandleAttachement[MAX_ATTACHMENTS_COUNT];
		R_HW::AttachementDescription descriptions[MAX_ATTACHMENTS_COUNT];
		uint32_t attachmentCount = 0;

		FrameGraphNode frame_graph_node;
		const char* name;
	};

	struct ResourceDesc
	{
		R_HW::GfxFormat format;
		VkExtent2D extent;
		R_HW::GfxImageUsageFlags usage_flags;
	};

	struct DataEntry
	{
		user_id_t user_id;
		R_HW::eDescriptorType descriptorType;
		uint32_t count;
		uint32_t flags;
		ResourceDesc resourceDesc;
		eSamplers sampler;
	};

	enum RenderTargetRefFlags
	{
		FG_RENDERTARGET_REF_CLEAR_BIT = 1 << 0,
		FG_RENDERTARGET_REF_READ_BIT = 1 << 1,
		FG_RENDERTARGET_REF_DEPTH_READ = 1 << 2,
	};

	class FrameGraph
	{
	public:
		class FrameGraphInternal* imp;
		R_HW::GfxImage dummyImage;
		FrameGraph( class FrameGraphInternal* );
		FrameGraph();
		const R_HW::RenderPass* GetRenderPass( uint32_t id );
		const R_HW::GfxImage* GetImageFromId( user_id_t render_target_id );
		void AddExternalImage( fg_handle_t handle, uint32_t frameIndex, const R_HW::GfxImage& image );
	};

	//Compilation
	FrameGraph CreateGraph( std::vector<RenderPassCreationData> *inRpCreationData, std::vector<DataEntry> *inRtCreationData );
	void Cleanup( FrameGraph* frameGraph );
	void CreateRenderPasses( FrameGraph* frameGraphExternal );


	//Frame graph stuff
	void RecordDrawCommands( uint32_t currentFrame, void* userData, R_HW::GfxCommandBuffer graphicsCommandBuffer, VkExtent2D extent, FrameGraph* frameGraphExternal );
}