#pragma once

#include "frame_graph_bindings.h"

#include "geometry_renderpass.h"
#include "text_overlay.h"
#include "vk_globals.h"
#include "../shaders/shadersCommon.h"

#include <unordered_map>

std::array< GpuInputData, SIMULTANEOUS_FRAMES>* _pInputBuffers;
GfxDescriptorPool _descriptorPool;

const uint32_t maxModelsCount = 256;
const VkExtent2D screenSize = { 224, 384 };

void FG_Script_SetInputBuffers( std::array< GpuInputData, SIMULTANEOUS_FRAMES>* pInputBuffers, GfxDescriptorPool descriptorPool )
{
	_pInputBuffers = pInputBuffers;
	_descriptorPool = descriptorPool;
}

enum class eTechniqueDataEntryName
{
	FIRST = 0,

	INSTANCE_DATA = FIRST,
	SCENE_DATA,

	COUNT
};

enum class eTechniqueDataEntryImageName
{
	FIRST = (uint32_t)eTechniqueDataEntryName::COUNT,

	BINDLESS_TEXTURES = FIRST,
	TEXT,

	SCENE_COLOR,
	SCENE_DEPTH,
	BACKBUFFER,

	COUNT
};

// TODO: find a way to handle double buffering, I could increase the number of buffers created but then it will write it as an array of buffers?
// TODO: frame graph doesn't need to know about external resources... But I need them to build the descriptor sets layout. And I want one definition for all data.
// Render pass and techniques are as one here, use that idea to define them.
// Have a list of descriptor sets instead of instance and pass to keep things generic. Check the binding point to know to which (instance or pass) it belongs.

inline void SetBuffers( GpuInputData* buffers, eTechniqueDataEntryName id, GpuBuffer* input, uint32_t count )
{
	SetBuffers( buffers, static_cast< uint32_t >(id), input, count );
}

inline void SetImages( GpuInputData* buffers, eTechniqueDataEntryImageName id, GfxImageSamplerCombined* input, uint32_t count )
{
	SetImages( buffers, static_cast< uint32_t >(id), input, count );
}

inline GpuBuffer* GetBuffer( const GpuInputData* buffers, eTechniqueDataEntryName id )
{
	return GetBuffer( buffers, static_cast< uint32_t >(id) );
}

inline GfxImageSamplerCombined* GetImage( const GpuInputData* buffers, eTechniqueDataEntryImageName id )
{
	return GetImage( buffers, static_cast< uint32_t >(id) );
}

static FG::RenderPassCreationData FG_Geometry_CreateGraphNode( FG::fg_handle_t sceneColor, FG::fg_handle_t sceneDepth, FG::fg_handle_t sceneData, FG::fg_handle_t bindlessTextures, FG::fg_handle_t instanceData )
{
	FG::DescriptorTableDesc geoPassSetDesc =
	{
		RENDERPASS_SET,
		{
			{ sceneData, { 0, eDescriptorAccess::READ, GFX_SHADER_STAGE_VERTEX_BIT | GFX_SHADER_STAGE_FRAGMENT_BIT } },
			{ bindlessTextures, { 2, eDescriptorAccess::READ, GFX_SHADER_STAGE_FRAGMENT_BIT } },
		}
	};

	FG::DescriptorTableDesc geoInstanceSetDesc =
	{
		INSTANCE_SET,
		{
			{ instanceData, { 0, eDescriptorAccess::READ, GFX_SHADER_STAGE_VERTEX_BIT | GFX_SHADER_STAGE_FRAGMENT_BIT } }
		}
	};

	FG::RenderPassCreationData renderPassCreationData;
	renderPassCreationData.name = "geometry_pass";

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData.frame_graph_node;
	frameGraphNode->RecordDrawCommands = GeometryRecordDrawCommandsBuffer;

	frameGraphNode->gpuPipelineLayout = GetGeoPipelineLayout();
	frameGraphNode->gpuPipelineStateDesc = GetGeoPipelineState();
	frameGraphNode->descriptorSets.push_back( geoPassSetDesc );
	frameGraphNode->descriptorSets.push_back( geoInstanceSetDesc );
	frameGraphNode->renderTargetRefs.push_back( { sceneColor, FG::FG_RENDERTARGET_REF_CLEAR_BIT } );
	frameGraphNode->renderTargetRefs.push_back( { sceneDepth, FG::FG_RENDERTARGET_REF_CLEAR_BIT } );

	return renderPassCreationData;
}

static FG::RenderPassCreationData FG_TextOverlay_CreateGraphNode( FG::fg_handle_t sceneColor, FG::fg_handle_t textTexture )
{
	FG::DescriptorTableDesc textPassSet =
	{
		RENDERPASS_SET,
		{
			{ textTexture, { 0, eDescriptorAccess::READ, GFX_SHADER_STAGE_FRAGMENT_BIT } }
		}
	};


	FG::RenderPassCreationData renderPassCreationData;
	renderPassCreationData.name = "text_pass";

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData.frame_graph_node;
	frameGraphNode->RecordDrawCommands = TextRecordDrawCommandsBuffer;

	frameGraphNode->gpuPipelineLayout = GetTextPipelineLayout();
	frameGraphNode->gpuPipelineStateDesc = GetTextPipelineState();
	//TODO: we don't need one set per frame for this one
	frameGraphNode->descriptorSets.push_back( textPassSet );
	frameGraphNode->renderTargetRefs.push_back( { sceneColor, 0 } );

	return renderPassCreationData;
}

#include "file_system.h"
void CopyRecordDrawCommandsBuffer( GfxCommandBuffer graphicsCommandBuffer, const FG::TaskInputData& inputData )
{
	//TODO: we don't need extent, it's implicit by the framebuffer
	//TODO: don't let this code choose with "currentFrame" it doesn't need to know that.
	CmdBeginLabel( graphicsCommandBuffer, "Copy Renderpass", glm::vec4( 0.8f, 0.8f, 0.8f, 1.0f ) );
	const FrameBuffer& frameBuffer = inputData.renderpass->outputFrameBuffer[inputData.currentFrame];
	BeginRenderPass( graphicsCommandBuffer, *inputData.renderpass, frameBuffer );

	vkCmdBindPipeline( graphicsCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, inputData.technique->pipeline );
	vkCmdBindDescriptorSets( graphicsCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, inputData.technique->pipelineLayout, 0, 1, &inputData.technique->descriptor_sets[RENDERPASS_SET].hw_descriptorSets[inputData.currentFrame], 0, nullptr );

	vkCmdDraw( graphicsCommandBuffer, 4, 1, 0, 0 );

	EndRenderPass( graphicsCommandBuffer );
	CmdEndLabel( graphicsCommandBuffer );
}

GpuPipelineLayout GetCopyPipelineLayout()
{
	return GpuPipelineLayout();
}

GpuPipelineStateDesc GetCopyPipelineState()
{
	GpuPipelineStateDesc gpuPipelineState = {};
	GetBindingDescription( VIBindings_PosColUV, &gpuPipelineState.viState );//unused

	gpuPipelineState.shaders = {
		{ FS::readFile( "shaders/copy.vert.spv" ), "main", GFX_SHADER_STAGE_VERTEX_BIT },
		{ FS::readFile( "shaders/copy.frag.spv" ), "main", GFX_SHADER_STAGE_FRAGMENT_BIT } };

	gpuPipelineState.rasterizationState.backFaceCulling = false;
	gpuPipelineState.rasterizationState.depthBiased = false;

	gpuPipelineState.depthStencilState.depthRead = false;
	gpuPipelineState.depthStencilState.depthWrite = false;

	gpuPipelineState.blendEnabled = false;
	gpuPipelineState.primitiveTopology = GfxPrimitiveTopology::TRIANGLE_STRIP;
	return gpuPipelineState;
}

static FG::RenderPassCreationData FG_Copy_CreateGraphNode( FG::fg_handle_t dst, FG::fg_handle_t src )
{
	FG::DescriptorTableDesc copyPassSet =
	{
		RENDERPASS_SET,
		{
			{ src, { 0, eDescriptorAccess::READ, GFX_SHADER_STAGE_FRAGMENT_BIT } }
		}
	};

	FG::RenderPassCreationData renderPassCreationData;
	renderPassCreationData.name = "copy_pass";

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData.frame_graph_node;
	frameGraphNode->RecordDrawCommands = CopyRecordDrawCommandsBuffer;

	frameGraphNode->gpuPipelineLayout = GpuPipelineLayout();
	frameGraphNode->gpuPipelineStateDesc = GetCopyPipelineState();
	frameGraphNode->descriptorSets.push_back( copyPassSet );
	frameGraphNode->renderTargetRefs.push_back( { dst, 0 } );
	frameGraphNode->renderTargetRefs.push_back( { src, FG::FG_RENDERTARGET_REF_READ_BIT } );

	return renderPassCreationData;
}

class ResourceGatherer
{
public:
	std::vector<FG::DataEntry> m_resources;

	FG::fg_handle_t AddResource( const FG::DataEntry& resourceDesc )
	{
		FG::fg_handle_t resourceHandle = m_resources.size();
		m_resources.push_back( resourceDesc );
		return resourceHandle;
	}
};

FG::FrameGraph InitializeScript( const Swapchain* swapchain )
{
	//Setup resources
	GfxFormat swapchainFormat = GetFormat( swapchain->surfaceFormat );
	VkExtent2D swapchainExtent = swapchain->extent;

	ResourceGatherer resourceGatherer;
	FG::fg_handle_t scene_data_h = resourceGatherer.AddResource( CREATE_BUFFER( eTechniqueDataEntryName::SCENE_DATA, sizeof( SceneMatricesUniform ) ) );
	FG::fg_handle_t instance_data_h = resourceGatherer.AddResource( CREATE_BUFFER_DYNAMIC( eTechniqueDataEntryName::INSTANCE_DATA, sizeof( GfxInstanceData ), maxModelsCount ) );
	//TODO: Remove external resources that don't need to be managed
	FG::fg_handle_t bindless_textures_h = resourceGatherer.AddResource( CREATE_IMAGE_SAMPLER_EXTERNAL( eTechniqueDataEntryImageName::BINDLESS_TEXTURES, BINDLESS_TEXTURES_MAX ) );
	FG::fg_handle_t text_texture_h = resourceGatherer.AddResource( CREATE_IMAGE_SAMPLER_EXTERNAL( eTechniqueDataEntryImageName::TEXT, 1 ) );
	FG::fg_handle_t scene_depth_h = resourceGatherer.AddResource( CREATE_IMAGE_DEPTH( eTechniqueDataEntryImageName::SCENE_DEPTH, GfxFormat::D32_SFLOAT, screenSize, 0 ) );
	FG::fg_handle_t scene_color_h = resourceGatherer.AddResource( CREATE_IMAGE_COLOR_SAMPLER( eTechniqueDataEntryImageName::SCENE_COLOR, swapchainFormat, screenSize, GfxImageUsageFlagBits::SAMPLED, eSamplers::Point ) );
	//TODO: also if something is external I shouldn't have to state the count, format and size. Format is used for renderpasses only
	FG::fg_handle_t backbuffer_h = resourceGatherer.AddResource( CREATE_IMAGE_COLOR( eTechniqueDataEntryImageName::BACKBUFFER, swapchainFormat, swapchainExtent, 0, FG::eDataEntryFlags::EXTERNAL ) );

	//Setup passes
	std::vector<FG::RenderPassCreationData> rpCreationData;
	rpCreationData.push_back( FG_Geometry_CreateGraphNode( scene_color_h, scene_depth_h, scene_data_h, bindless_textures_h, instance_data_h ) );
	rpCreationData.push_back( FG_TextOverlay_CreateGraphNode( scene_color_h, text_texture_h ) );
	rpCreationData.push_back( FG_Copy_CreateGraphNode( backbuffer_h, scene_color_h ) );

	FG::FrameGraph fg = FG::CreateGraph( &rpCreationData, &resourceGatherer.m_resources );
	for( uint32_t frameIndex = 0; frameIndex < SIMULTANEOUS_FRAMES; ++frameIndex )
		fg.AddExternalImage( backbuffer_h, frameIndex, swapchain->images[frameIndex] );
	FG::CreateRenderPasses( &fg );
	FG::SetupInputBuffers( &fg, *_pInputBuffers );
	FG::CreateTechniques( &fg, _descriptorPool );
	FG::UpdateTechniqueDescriptorSets( &fg, *_pInputBuffers );

	return fg;
}
