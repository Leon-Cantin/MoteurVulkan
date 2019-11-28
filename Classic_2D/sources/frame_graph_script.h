#pragma once

#include "frame_graph_bindings.h"

#include "geometry_renderpass.h"
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

	COUNT
};

// TODO: find a way to handle double buffering, I could increase the number of buffers created but then it will write it as an array of buffers?
// TODO: frame graph doesn't need to know about external resources... But I need them to build the descriptor sets layout. And I want one definition for all data.
// Render pass and techniques are as one here, use that idea to define them.
// Have a list of descriptor sets instead of instance and pass to keep things generic. Check the binding point to know to which (instance or pass) it belongs.

const uint32_t maxModelsCount = 64;
static FG::DataEntry techniqueDataEntries[static_cast< size_t >(eTechniqueDataEntryImageName::COUNT)] =
{
	//Buffers
	CREATE_BUFFER_DYNAMIC( eTechniqueDataEntryName::INSTANCE_DATA, sizeof( GfxInstanceData ),  maxModelsCount ),
	CREATE_BUFFER( eTechniqueDataEntryName::SCENE_DATA, sizeof( SceneMatricesUniform ) ),

	//images
	CREATE_IMAGE_SAMPLER_EXTERNAL( eTechniqueDataEntryImageName::BINDLESS_TEXTURES, 5 ),
	CREATE_IMAGE_SAMPLER_EXTERNAL( eTechniqueDataEntryImageName::TEXT, 1 ),

	CREATE_IMAGE_COLOR( eTechniqueDataEntryImageName::SCENE_COLOR, VkFormat( 0 ), FG::SWAPCHAIN_SIZED, 0, true ),
	CREATE_IMAGE_DEPTH( eTechniqueDataEntryImageName::SCENE_DEPTH, VK_FORMAT_D32_SFLOAT, FG::SWAPCHAIN_SIZED, 0, true ),
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

GfxDescriptorSetDesc geoPassSetDesc =
{
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryName::SCENE_DATA), 0, eDescriptorAccess::READ, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },

		{ static_cast< uint32_t >(eTechniqueDataEntryImageName::BINDLESS_TEXTURES), 2, eDescriptorAccess::READ, VK_SHADER_STAGE_FRAGMENT_BIT },
	}
};

GfxDescriptorSetDesc geoInstanceSetDesc =
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
	FG::ClearLast( renderPassCreationData );
	FG::RenderDepth( renderPassCreationData, VK_FORMAT_D32_SFLOAT, ( uint32_t )eTechniqueDataEntryImageName::SCENE_DEPTH );
	FG::ClearLast( renderPassCreationData );

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData.frame_graph_node;
	frameGraphNode->RecordDrawCommands = GeometryRecordDrawCommandsBuffer;

	frameGraphNode->gpuPipelineLayout = GetGeoPipelineLayout();
	frameGraphNode->gpuPipelineState = GetGeoPipelineState();
	frameGraphNode->instanceSet = &geoInstanceSetDesc;
	frameGraphNode->passSet = &geoPassSetDesc;

	return renderPassCreationData;
}

GfxDescriptorSetDesc textPassSet =
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

	std::vector<FG::DataEntry> dataEntries ( techniqueDataEntries, techniqueDataEntries + sizeof( techniqueDataEntries ) / sizeof( techniqueDataEntries[0] ) );
	dataEntries[( uint32_t )eTechniqueDataEntryImageName::SCENE_COLOR].resourceDesc.format = swapchainFormat;
	dataEntries[( uint32_t )eTechniqueDataEntryImageName::SCENE_COLOR].resourceDesc.extent = swapchainExtent; // maybe not needed because FG does it
	dataEntries[( uint32_t )eTechniqueDataEntryImageName::SCENE_DEPTH].resourceDesc.extent = swapchainExtent;

	//Setup passes	
	std::vector<FG::RenderPassCreationData> rpCreationData;
	rpCreationData.push_back( FG_Geometry_CreateGraphNode( swapchain ) );
	rpCreationData.push_back( FG_TextOverlay_CreateGraphNode( swapchain ) );

	FG::FrameGraph fg = FG::CreateGraph( swapchain, &rpCreationData, &dataEntries, backBufferId, _descriptorPool );

	FG::SetupInputBuffers( &fg, *_pInputBuffers );
	FG::CreateTechniques( &fg, _descriptorPool );
	FG::UpdateTechniqueDescriptorSets( &fg, *_pInputBuffers );

	return fg;
}
