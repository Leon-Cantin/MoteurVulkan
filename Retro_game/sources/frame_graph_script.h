#pragma once

#include "frame_graph_bindings.h"

#include "geometry_renderpass.h"
#include "shadow_renderpass.h"
#include "skybox.h"
#include "text_overlay.h"
#include "bullet_debug_draw_pass.h"

#include "../shaders/shadersCommon.h"

#include <unordered_map>

enum class eTechniqueDataEntryName
{
	FIRST = 0,

	INSTANCE_DATA = FIRST,
	SHADOW_DATA,
	SCENE_DATA,
	LIGHT_DATA,
	SKYBOX_DATA,

	SAMPLERS,

	COUNT
};

enum class eTechniqueDataEntryImageName
{
	FIRST = ( uint32_t )eTechniqueDataEntryName::COUNT,

	BINDLESS_TEXTURES = FIRST,
	TEXT,
	SKYBOX,

	SCENE_COLOR,
	SCENE_DEPTH,
	SHADOW_MAP,

	LAST = SHADOW_MAP,
	COUNT = LAST - FIRST,
};

// TODO: find a way to handle double buffering, I could increase the number of buffers created but then it will write it as an array of buffers?
// TODO: frame graph doesn't need to know about external resources... But I need them to build the descriptor sets layout. And I want one definition for all data.
// Render pass and techniques are as one here, use that idea to define them.
// Have a list of descriptor sets instead of instance and pass to keep things generic. Check the binding point to know to which (instance or pass) it belongs.

const uint32_t maxModelsCount = 32;
constexpr VkExtent2D RT_EXTENT_SHADOW = { 1024, 1024 };

inline void SetBuffers( GpuInputData* buffers, eTechniqueDataEntryName id, GpuBuffer* input, uint32_t count )
{
	SetBuffers( buffers, static_cast< uint32_t >(id), input, count );
}

inline void SetImages( GpuInputData* buffers, eTechniqueDataEntryImageName id, GfxImageSamplerCombined* input, uint32_t count )
{
	SetImages( buffers, static_cast< uint32_t >(id), input, count );
}

inline void SetSamplers( GpuInputData* buffers, eTechniqueDataEntryName id, GfxApiSampler* input, uint32_t count )
{
	SetSamplers( buffers, static_cast< uint32_t >(id), input, count );
}

inline GpuBuffer* GetBuffer( const GpuInputData* buffers, eTechniqueDataEntryName id )
{
	return GetBuffer( buffers, static_cast< uint32_t >(id) );
}

inline GfxImageSamplerCombined* GetImage( const GpuInputData* buffers, eTechniqueDataEntryImageName id )
{
	return GetImage( buffers, static_cast< uint32_t >(id) );
}


static FG::RenderPassCreationData FG_Opaque_CreateGraphNode( FG::fg_handle_t sceneColor, FG::fg_handle_t sceneDepth, FG::fg_handle_t bindlessTextures, FG::fg_handle_t shadowMap, FG::fg_handle_t shadowData,
	FG::fg_handle_t instanceData, FG::fg_handle_t lightData, FG::fg_handle_t sceneData, FG::fg_handle_t samplers )
{
	FG::DescriptorTableDesc geoPassSetDesc =
	{
		RENDERPASS_SET,
		{
			{ sceneData, 0, eDescriptorAccess::READ, GFX_SHADER_STAGE_VERTEX_BIT | GFX_SHADER_STAGE_FRAGMENT_BIT },
			{ lightData, 1, eDescriptorAccess::READ, GFX_SHADER_STAGE_VERTEX_BIT | GFX_SHADER_STAGE_FRAGMENT_BIT },
			{ bindlessTextures, 2, eDescriptorAccess::READ, GFX_SHADER_STAGE_FRAGMENT_BIT },
			{ samplers, 3, eDescriptorAccess::READ, GFX_SHADER_STAGE_FRAGMENT_BIT | GFX_SHADER_STAGE_VERTEX_BIT },
			{ shadowMap, 4, eDescriptorAccess::READ, GFX_SHADER_STAGE_FRAGMENT_BIT },
		}
	};

	FG::DescriptorTableDesc geoInstanceSetDesc =
	{
		INSTANCE_SET,
		{
			{ instanceData, 0, eDescriptorAccess::READ, GFX_SHADER_STAGE_VERTEX_BIT | GFX_SHADER_STAGE_FRAGMENT_BIT }
		}
	};

	FG::RenderPassCreationData renderPassCreationData;
	renderPassCreationData.name = "opaque_pass";

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData.frame_graph_node;
	frameGraphNode->RecordDrawCommands = GeometryRecordDrawCommandsBuffer;

	frameGraphNode->gpuPipelineLayout = GetGeoPipelineLayout();
	frameGraphNode->gpuPipelineStateDesc = GetGeoPipelineState();
	frameGraphNode->descriptorSets.push_back( geoPassSetDesc );
	frameGraphNode->descriptorSets.push_back( geoInstanceSetDesc );
	frameGraphNode->renderTargetRefs.push_back( { sceneColor, FG::FG_RENDERTARGET_REF_CLEAR_BIT } );
	frameGraphNode->renderTargetRefs.push_back( { sceneDepth, FG::FG_RENDERTARGET_REF_CLEAR_BIT } );
	frameGraphNode->renderTargetRefs.push_back( { shadowMap, FG::FG_RENDERTARGET_REF_READ_BIT } );

	return renderPassCreationData;
}


static FG::RenderPassCreationData FG_Shadow_CreateGraphNode( FG::fg_handle_t shadowMap, FG::fg_handle_t shadowData, FG::fg_handle_t instanceData )
{
	FG::DescriptorTableDesc shadowPassSet =
	{
		RENDERPASS_SET,
		{
			{ shadowData, 0, eDescriptorAccess::READ, GFX_SHADER_STAGE_VERTEX_BIT }
		}
	};

	FG::DescriptorTableDesc shadowInstanceSet =
	{
		INSTANCE_SET,
		{
			{ instanceData, 0, eDescriptorAccess::READ, GFX_SHADER_STAGE_VERTEX_BIT }
		}
	};
	FG::RenderPassCreationData renderPassCreationData;
	renderPassCreationData.name = "shadow_pass";

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData.frame_graph_node;
	frameGraphNode->RecordDrawCommands = ShadowRecordDrawCommandsBuffer;

	frameGraphNode->gpuPipelineLayout = GetShadowPipelineLayout();
	frameGraphNode->gpuPipelineStateDesc = GetShadowPipelineState();
	frameGraphNode->descriptorSets.push_back( shadowPassSet );
	frameGraphNode->descriptorSets.push_back( shadowInstanceSet );
	frameGraphNode->renderTargetRefs.push_back( { shadowMap, FG::FG_RENDERTARGET_REF_CLEAR_BIT } );

	return renderPassCreationData;
}

static FG::RenderPassCreationData FG_Skybox_CreateGraphNode( FG::fg_handle_t sceneColor, FG::fg_handle_t sceneDepth, FG::fg_handle_t skyboxTexture, FG::fg_handle_t skyboxData )
{
	FG::DescriptorTableDesc skyboxPassSetDesc =
	{
		RENDERPASS_SET,
		{
			{ skyboxData, 0, eDescriptorAccess::READ, GFX_SHADER_STAGE_VERTEX_BIT },
			{ skyboxTexture, 1, eDescriptorAccess::READ, GFX_SHADER_STAGE_FRAGMENT_BIT },
		}
	};

	FG::RenderPassCreationData renderPassCreationData;
	renderPassCreationData.name = "skybox_pass";

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData.frame_graph_node;
	frameGraphNode->RecordDrawCommands = SkyboxRecordDrawCommandsBuffer;

	frameGraphNode->gpuPipelineLayout = GetSkyboxPipelineLayout();
	frameGraphNode->gpuPipelineStateDesc = GetSkyboxPipelineState();
	frameGraphNode->descriptorSets.push_back( skyboxPassSetDesc );
	frameGraphNode->renderTargetRefs.push_back( { sceneColor, 0 } );
	frameGraphNode->renderTargetRefs.push_back( { sceneDepth, FG::FG_RENDERTARGET_REF_DEPTH_READ } );

	return renderPassCreationData;
}

static FG::RenderPassCreationData FG_TextOverlay_CreateGraphNode( FG::fg_handle_t sceneColor, FG::fg_handle_t textTexture )
{
	FG::DescriptorTableDesc textPassSet =
	{
		RENDERPASS_SET,
		{
			{ textTexture, 0, eDescriptorAccess::READ, GFX_SHADER_STAGE_FRAGMENT_BIT }
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

static FG::RenderPassCreationData FG_BtDebug_CreateGraphNode( FG::fg_handle_t sceneColor, FG::fg_handle_t sceneData )
{
	FG::DescriptorTableDesc renderpass_set =
	{
		RENDERPASS_SET,
		{
			{ sceneData, 0, eDescriptorAccess::READ, GFX_SHADER_STAGE_VERTEX_BIT | GFX_SHADER_STAGE_FRAGMENT_BIT },
		}
	};

	FG::RenderPassCreationData renderPassCreationData;
	renderPassCreationData.name = "bullet_debug_pass";

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData.frame_graph_node;
	frameGraphNode->RecordDrawCommands = BtDebugRecordDrawCommandsBuffer;

	frameGraphNode->gpuPipelineLayout = GetBtDebugPipelineLayout();
	frameGraphNode->gpuPipelineStateDesc = GetBtDebugPipelineState();
	//TODO: we don't need one set per frame for this one
	frameGraphNode->descriptorSets.push_back( renderpass_set );
	frameGraphNode->renderTargetRefs.push_back( { sceneColor, 0 } );

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

struct RetroFrameGraphParams
{
	std::array< GpuInputData, SIMULTANEOUS_FRAMES>* _pInputBuffers;
	VkDescriptorPool _descriptorPool;

	bool d_btDrawDebug;
};

FG::FrameGraph InitializeScript( const Swapchain* swapchain, void* user_params )
{
	RetroFrameGraphParams* params = reinterpret_cast< RetroFrameGraphParams* >(user_params);
	//Setup resources
	GfxFormat swapchainFormat = GetFormat( swapchain->surfaceFormat );
	VkExtent2D swapchainExtent = swapchain->extent;

	ResourceGatherer resourceGatherer;
	FG::fg_handle_t instance_data_h = resourceGatherer.AddResource( CREATE_BUFFER_DYNAMIC( eTechniqueDataEntryName::INSTANCE_DATA, sizeof( GfxInstanceData ), maxModelsCount ) );
	FG::fg_handle_t scene_data_h = resourceGatherer.AddResource( CREATE_BUFFER( eTechniqueDataEntryName::SCENE_DATA, sizeof( SceneMatricesUniform ) ) );
	FG::fg_handle_t shadow_data_h = resourceGatherer.AddResource( CREATE_BUFFER( eTechniqueDataEntryName::SHADOW_DATA, sizeof( SceneMatricesUniform ) ) );
	FG::fg_handle_t light_data_h = resourceGatherer.AddResource( CREATE_BUFFER( eTechniqueDataEntryName::LIGHT_DATA, sizeof( LightUniform ) ) );
	FG::fg_handle_t skybox_data_h = resourceGatherer.AddResource( CREATE_BUFFER( eTechniqueDataEntryName::SKYBOX_DATA, sizeof( SkyboxUniformBufferObject ) ) );

	//TODO: Remove external resources that don't need to be managed
	FG::fg_handle_t bindless_textures_h = resourceGatherer.AddResource( CREATE_IMAGE_EXTERNAL( eTechniqueDataEntryImageName::BINDLESS_TEXTURES, BINDLESS_TEXTURES_MAX ) );
	FG::fg_handle_t text_texture_h = resourceGatherer.AddResource( CREATE_IMAGE_SAMPLER_EXTERNAL( eTechniqueDataEntryImageName::TEXT, 1 ) );
	FG::fg_handle_t skybox_texture_h = resourceGatherer.AddResource( CREATE_IMAGE_SAMPLER_EXTERNAL( eTechniqueDataEntryImageName::SKYBOX, 1 ) );
	FG::fg_handle_t samplers_h = resourceGatherer.AddResource( CREATE_SAMPLER_EXTERNAL( eTechniqueDataEntryName::SAMPLERS, SAMPLERS_MAX ) );

	//TODO: I shouldn't have to specify usage such as "sampled" should be implicit
	FG::fg_handle_t scene_depth_h = resourceGatherer.AddResource( CREATE_IMAGE_DEPTH( eTechniqueDataEntryImageName::SCENE_DEPTH, GfxFormat::D32_SFLOAT, swapchainExtent, 0 ) );
	FG::fg_handle_t shadow_map_h = resourceGatherer.AddResource( CREATE_IMAGE_DEPTH_SAMPLER( eTechniqueDataEntryImageName::SHADOW_MAP, GfxFormat::D32_SFLOAT, RT_EXTENT_SHADOW, GfxImageUsageFlagBits::SAMPLED, eSamplers::Shadow ) );
	FG::fg_handle_t scene_color_h = resourceGatherer.AddResource( CREATE_IMAGE_COLOR( eTechniqueDataEntryImageName::SCENE_COLOR, swapchainFormat, swapchainExtent, 0, FG::eDataEntryFlags::EXTERNAL ) );

	//Setup passes
	std::vector<FG::RenderPassCreationData> rpCreationData;
	rpCreationData.push_back( FG_Shadow_CreateGraphNode( shadow_map_h, shadow_data_h, instance_data_h ) );
	rpCreationData.push_back( FG_Opaque_CreateGraphNode( scene_color_h, scene_depth_h, bindless_textures_h, shadow_map_h, shadow_data_h, instance_data_h, light_data_h, scene_data_h, samplers_h ) );
	rpCreationData.push_back( FG_Skybox_CreateGraphNode( scene_color_h, scene_depth_h, skybox_texture_h, skybox_data_h ) );
	if( params->d_btDrawDebug )
		rpCreationData.push_back( FG_BtDebug_CreateGraphNode( scene_color_h, scene_data_h ) );
	rpCreationData.push_back( FG_TextOverlay_CreateGraphNode( scene_color_h, text_texture_h ) );

	FG::FrameGraph fg = FG::CreateGraph( &rpCreationData, &resourceGatherer.m_resources );
	for( uint32_t frameIndex = 0; frameIndex < SIMULTANEOUS_FRAMES; ++frameIndex )
		fg.AddExternalImage( scene_color_h, frameIndex, swapchain->images[frameIndex] );
	FG::CreateRenderPasses( &fg );
	FG::AddResourcesToInputBuffer( &fg, *params->_pInputBuffers );
	FG::CreateTechniques( &fg, params->_descriptorPool );
	fg.dummyImage = FG::CreateDummyImage();
	FG::UpdateTechniqueDescriptorSets( &fg, *params->_pInputBuffers, fg.dummyImage );

	return fg;
}
