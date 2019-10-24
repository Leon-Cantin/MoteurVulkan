#pragma once

#include "frame_graph_bindings.h"

#include "geometry_renderpass.h"
#include "shadow_renderpass.h"
#include "skybox.h"
#include "text_overlay.h"
#include "descriptors.h"

#include <unordered_map>

std::array< GpuInputData, SIMULTANEOUS_FRAMES>* _pInputBuffers;
VkDescriptorPool _descriptorPool;

void SetInputBuffers( std::array< GpuInputData, SIMULTANEOUS_FRAMES>* pInputBuffers, VkDescriptorPool descriptorPool )
{
	_pInputBuffers = pInputBuffers;
	_descriptorPool = descriptorPool;
}

enum class eTechniqueDataEntryName
{
	FIRST = 0,

	INSTANCE_DATA = FIRST,
	SHADOW_DATA,
	SCENE_DATA,
	LIGHT_DATA,
	SKYBOX_DATA,

	COUNT
};

enum class eTechniqueDataEntryImageName
{
	FIRST = (uint32_t)eTechniqueDataEntryName::COUNT,

	BINDLESS_TEXTURES = FIRST,
	TEXT,
	SKYBOX,

	SCENE_COLOR,
	SCENE_DEPTH,
	SHADOW_MAP,

	COUNT
};

// TODO: find a way to handle double buffering, I could increase the number of buffers created but then it will write it as an array of buffers?
// TODO: frame graph doesn't need to know about external resources... But I need them to build the descriptor sets layout. And I want one definition for all data.
// Render pass and techniques are as one here, use that idea to define them.
// Have a list of descriptor sets instead of instance and pass to keep things generic. Check the binding point to know to which (instance or pass) it belongs.

const uint32_t maxModelsCount = 5;
constexpr VkFormat RT_FORMAT_SHADOW_DEPTH = VK_FORMAT_D32_SFLOAT;
constexpr VkExtent2D RT_EXTENT_SHADOW = { 1024, 1024 };
static FG::TechniqueDataEntry techniqueDataEntries[static_cast< size_t >(eTechniqueDataEntryImageName::COUNT)] =
{
	//Buffers
	CREATE_BUFFER_DYNAMIC( eTechniqueDataEntryName::INSTANCE_DATA, sizeof( GfxInstanceData ),  maxModelsCount ),
	CREATE_BUFFER( eTechniqueDataEntryName::SHADOW_DATA, sizeof( SceneMatricesUniform ) ),
	CREATE_BUFFER( eTechniqueDataEntryName::SCENE_DATA, sizeof( SceneMatricesUniform ) ),
	CREATE_BUFFER( eTechniqueDataEntryName::LIGHT_DATA, sizeof( LightUniform ) ),
	CREATE_BUFFER( eTechniqueDataEntryName::SKYBOX_DATA, sizeof( SkyboxUniformBufferObject ) ),

	//images
	CREATE_IMAGE_SAMPLER_EXTERNAL( eTechniqueDataEntryImageName::BINDLESS_TEXTURES, 5 ),
	CREATE_IMAGE_SAMPLER_EXTERNAL( eTechniqueDataEntryImageName::TEXT, 1 ),
	CREATE_IMAGE_SAMPLER_EXTERNAL( eTechniqueDataEntryImageName::SKYBOX, 1 ),

	CREATE_IMAGE_COLOR( eTechniqueDataEntryImageName::SCENE_COLOR, VkFormat( 0 ), FG::SWAPCHAIN_SIZED, 0, true ),
	CREATE_IMAGE_DEPTH( eTechniqueDataEntryImageName::SCENE_DEPTH, VK_FORMAT_D32_SFLOAT, FG::SWAPCHAIN_SIZED, 0, true ),
	CREATE_IMAGE_DEPTH_SAMPLER( eTechniqueDataEntryImageName::SHADOW_MAP, RT_FORMAT_SHADOW_DEPTH, RT_EXTENT_SHADOW, VK_IMAGE_USAGE_SAMPLED_BIT, false, Samplers::Shadow ),
};

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

TechniqueDescriptorSetDesc geoPassSetDesc =
{
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryName::SCENE_DATA), 0, eDescriptorAccess::READ, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },
		{ static_cast< uint32_t >(eTechniqueDataEntryName::LIGHT_DATA), 1, eDescriptorAccess::READ, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },

		{ static_cast< uint32_t >(eTechniqueDataEntryImageName::BINDLESS_TEXTURES), 2, eDescriptorAccess::READ, VK_SHADER_STAGE_FRAGMENT_BIT },
		{ static_cast< uint32_t >(eTechniqueDataEntryImageName::SHADOW_MAP), 4, eDescriptorAccess::READ, VK_SHADER_STAGE_FRAGMENT_BIT },
	}
};

TechniqueDescriptorSetDesc geoInstanceSetDesc =
{
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryName::INSTANCE_DATA), 0, eDescriptorAccess::READ, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }
	}
};


static FG::RenderPassCreationData FG_Geometry_CreateGraphNode( const Swapchain* swapchain )
{
	FG::RenderPassCreationData renderPassCreationData;
	renderPassCreationData.name = "geometry_pass";

	VkFormat swapchainFormat = swapchain->surfaceFormat.format;

	FG::RenderColor( renderPassCreationData, swapchainFormat, ( uint32_t )eTechniqueDataEntryImageName::SCENE_COLOR );
	FG::RenderDepth( renderPassCreationData, VK_FORMAT_D32_SFLOAT, ( uint32_t )eTechniqueDataEntryImageName::SCENE_DEPTH );
	FG::ClearLast( renderPassCreationData );
	FG::ReadResource( renderPassCreationData, ( uint32_t )eTechniqueDataEntryImageName::SHADOW_MAP );

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData.frame_graph_node;
	frameGraphNode->RecordDrawCommands = GeometryRecordDrawCommandsBuffer;

	frameGraphNode->gpuPipelineLayout = GetGeoPipelineLayout();
	frameGraphNode->gpuPipelineState = GetGeoPipelineState();
	frameGraphNode->instanceSet = &geoInstanceSetDesc;
	frameGraphNode->passSet = &geoPassSetDesc;

	return renderPassCreationData;
}

TechniqueDescriptorSetDesc shadowPassSet =
{
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryName::SHADOW_DATA), 0, eDescriptorAccess::READ, VK_SHADER_STAGE_VERTEX_BIT }
	}
};

TechniqueDescriptorSetDesc shadowInstanceSet =
{
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryName::INSTANCE_DATA), 0, eDescriptorAccess::READ, VK_SHADER_STAGE_VERTEX_BIT }
	}
};

static FG::RenderPassCreationData FG_Shadow_CreateGraphNode( const Swapchain* swapchain )
{
	FG::RenderPassCreationData renderPassCreationData;
	renderPassCreationData.name = "shadow_pass";

	FG::RenderDepth(renderPassCreationData, VK_FORMAT_D32_SFLOAT, ( uint32_t )eTechniqueDataEntryImageName::SHADOW_MAP );
	FG::ClearLast(renderPassCreationData);

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData.frame_graph_node;
	frameGraphNode->RecordDrawCommands = ShadowRecordDrawCommandsBuffer;

	frameGraphNode->gpuPipelineLayout = GetShadowPipelineLayout();
	frameGraphNode->gpuPipelineState = GetShadowPipelineState();
	frameGraphNode->instanceSet = &shadowInstanceSet;
	frameGraphNode->passSet = &shadowPassSet;

	return renderPassCreationData;
}

TechniqueDescriptorSetDesc skyboxPassSetDesc =
{
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryName::SKYBOX_DATA), 0, eDescriptorAccess::READ, VK_SHADER_STAGE_VERTEX_BIT },

		{ static_cast< uint32_t >(eTechniqueDataEntryImageName::SKYBOX), 1, eDescriptorAccess::READ, VK_SHADER_STAGE_FRAGMENT_BIT },
	}
};

static FG::RenderPassCreationData FG_Skybox_CreateGraphNode( const Swapchain* swapchain )
{
	FG::RenderPassCreationData renderPassCreationData;
	renderPassCreationData.name = "skybox_pass";

	VkFormat swapchainFormat = swapchain->surfaceFormat.format;

	FG::RenderColor( renderPassCreationData, swapchainFormat, ( uint32_t )eTechniqueDataEntryImageName::SCENE_COLOR );
	FG::RenderDepth( renderPassCreationData, VK_FORMAT_D32_SFLOAT, ( uint32_t )eTechniqueDataEntryImageName::SCENE_DEPTH );

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData.frame_graph_node;
	frameGraphNode->RecordDrawCommands = SkyboxRecordDrawCommandsBuffer;

	frameGraphNode->gpuPipelineLayout = GetSkyboxPipelineLayout();
	frameGraphNode->gpuPipelineState = GetSkyboxPipelineState();
	frameGraphNode->instanceSet = nullptr;
	frameGraphNode->passSet = &skyboxPassSetDesc;

	return renderPassCreationData;
}

TechniqueDescriptorSetDesc textPassSet =
{
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryImageName::TEXT), 0, eDescriptorAccess::READ, VK_SHADER_STAGE_FRAGMENT_BIT }
	}
};

static FG::RenderPassCreationData FG_TextOverlay_CreateGraphNode( const Swapchain* swapchain )
{
	FG::RenderPassCreationData renderPassCreationData;
	renderPassCreationData.name = "skybox_pass";

	VkFormat swapchainFormat = swapchain->surfaceFormat.format;

	FG::RenderColor( renderPassCreationData, swapchainFormat, ( uint32_t )eTechniqueDataEntryImageName::SCENE_COLOR );

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData.frame_graph_node;
	frameGraphNode->RecordDrawCommands = TextRecordDrawCommandsBuffer;

	frameGraphNode->gpuPipelineLayout = GetTextPipelineLayout();
	frameGraphNode->gpuPipelineState = GetTextPipelineState();
	frameGraphNode->instanceSet = nullptr;
	//TODO: we don't need one set per frame for this one
	frameGraphNode->passSet = &textPassSet;

	return renderPassCreationData;
}

FG::FrameGraph InitializeScript( const Swapchain* swapchain )
{
	//Setup resources
	VkFormat swapchainFormat = swapchain->surfaceFormat.format;
	VkExtent2D swapchainExtent = swapchain->extent;

	uint32_t backBufferId = (uint32_t) eTechniqueDataEntryImageName::SCENE_COLOR;

	std::vector<FG::TechniqueDataEntry> dataEntries ( techniqueDataEntries, techniqueDataEntries + sizeof( techniqueDataEntries ) / sizeof( techniqueDataEntries[0] ) );
	dataEntries[( uint32_t )eTechniqueDataEntryImageName::SCENE_COLOR].resourceDesc.format = swapchainFormat;
	dataEntries[( uint32_t )eTechniqueDataEntryImageName::SCENE_COLOR].resourceDesc.extent = swapchainExtent; // maybe not needed because FG does it
	dataEntries[( uint32_t )eTechniqueDataEntryImageName::SCENE_DEPTH].resourceDesc.extent = swapchainExtent;

	//Setup passes	
	std::vector<FG::RenderPassCreationData> rpCreationData;
	rpCreationData.push_back( FG_Shadow_CreateGraphNode( swapchain ) );
	rpCreationData.push_back( FG_Geometry_CreateGraphNode( swapchain ) );
	rpCreationData.push_back( FG_Skybox_CreateGraphNode( swapchain ) );
	rpCreationData.push_back( FG_TextOverlay_CreateGraphNode( swapchain ) );

	FG::FrameGraph fg = FG::CreateGraph( swapchain, &rpCreationData, &dataEntries, backBufferId, _descriptorPool );
	FG::CreateTechniques( &fg, _descriptorPool, *_pInputBuffers );
	FG::UpdateTechniqueDescriptorSets( &fg, *_pInputBuffers );

	return fg;
}
