#pragma once

#include "frame_graph_bindings.h"

#include "geometry_renderpass.h"
#include "text_overlay.h"
#include "descriptors.h"
#include "..\shaders\shadersCommon.h"

#include <unordered_map>

std::array< GpuInputData, SIMULTANEOUS_FRAMES>* _pInputBuffers;
VkDescriptorPool _descriptorPool;

void FG_Script_SetInputBuffers( std::array< GpuInputData, SIMULTANEOUS_FRAMES>* pInputBuffers, VkDescriptorPool descriptorPool )
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

const uint32_t maxModelsCount = 256;
const VkExtent2D screenSize = { 224, 384 };
static FG::DataEntry techniqueDataEntries[static_cast< size_t >(eTechniqueDataEntryImageName::COUNT)] =
{
	//Buffers
	CREATE_BUFFER_DYNAMIC( eTechniqueDataEntryName::INSTANCE_DATA, sizeof( GfxInstanceData ),  maxModelsCount ),
	CREATE_BUFFER( eTechniqueDataEntryName::SCENE_DATA, sizeof( SceneMatricesUniform ) ),

	//images
	CREATE_IMAGE_SAMPLER_EXTERNAL( eTechniqueDataEntryImageName::BINDLESS_TEXTURES, BINDLESS_TEXTURES_MAX ),
	CREATE_IMAGE_SAMPLER_EXTERNAL( eTechniqueDataEntryImageName::TEXT, 1 ),

	CREATE_IMAGE_COLOR_SAMPLER( eTechniqueDataEntryImageName::SCENE_COLOR, VkFormat( 0 ), screenSize, VK_IMAGE_USAGE_SAMPLED_BIT, false, eSamplers::Point ),
	CREATE_IMAGE_DEPTH( eTechniqueDataEntryImageName::SCENE_DEPTH, VK_FORMAT_D32_SFLOAT, screenSize, 0, false ),
	CREATE_IMAGE_COLOR( eTechniqueDataEntryImageName::BACKBUFFER, VkFormat( 0 ), FG::SWAPCHAIN_SIZED, 0, true ),
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
	RENDERPASS_SET,
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryName::SCENE_DATA), 0, eDescriptorAccess::READ, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },

		{ static_cast< uint32_t >(eTechniqueDataEntryImageName::BINDLESS_TEXTURES), 2, eDescriptorAccess::READ, VK_SHADER_STAGE_FRAGMENT_BIT },
	}
};

GfxDescriptorSetDesc geoInstanceSetDesc =
{
	INSTANCE_SET,
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryName::INSTANCE_DATA), 0, eDescriptorAccess::READ, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }
	}
};


static FG::RenderPassCreationData FG_Geometry_CreateGraphNode( const Swapchain* swapchain, eTechniqueDataEntryImageName sceneColor, eTechniqueDataEntryImageName sceneDepth)
{
	FG::RenderPassCreationData renderPassCreationData;
	renderPassCreationData.name = "geometry_pass";

	VkFormat swapchainFormat = swapchain->surfaceFormat.format;

	FG::RenderColor( renderPassCreationData, swapchainFormat, ( uint32_t )sceneColor );
	FG::ClearLast( renderPassCreationData );
	FG::RenderDepth( renderPassCreationData, VK_FORMAT_D32_SFLOAT, ( uint32_t )sceneDepth );
	FG::ClearLast( renderPassCreationData );

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData.frame_graph_node;
	frameGraphNode->RecordDrawCommands = GeometryRecordDrawCommandsBuffer;

	frameGraphNode->gpuPipelineLayout = GetGeoPipelineLayout();
	frameGraphNode->gpuPipelineState = GetGeoPipelineState();
	frameGraphNode->descriptorSets.push_back( geoPassSetDesc );
	frameGraphNode->descriptorSets.push_back( geoInstanceSetDesc );

	return renderPassCreationData;
}

GfxDescriptorSetDesc textPassSet =
{
	RENDERPASS_SET,
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryImageName::TEXT), 0, eDescriptorAccess::READ, VK_SHADER_STAGE_FRAGMENT_BIT }
	}
};

static FG::RenderPassCreationData FG_TextOverlay_CreateGraphNode( const Swapchain* swapchain, eTechniqueDataEntryImageName sceneColor )
{
	FG::RenderPassCreationData renderPassCreationData;
	renderPassCreationData.name = "text_pass";

	VkFormat swapchainFormat = swapchain->surfaceFormat.format;

	FG::RenderColor( renderPassCreationData, swapchainFormat, ( uint32_t )sceneColor );

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData.frame_graph_node;
	frameGraphNode->RecordDrawCommands = TextRecordDrawCommandsBuffer;

	frameGraphNode->gpuPipelineLayout = GetTextPipelineLayout();
	frameGraphNode->gpuPipelineState = GetTextPipelineState();
	//TODO: we don't need one set per frame for this one
	frameGraphNode->descriptorSets.push_back( textPassSet );

	return renderPassCreationData;
}

GfxDescriptorSetDesc copyPassSet =
{
	RENDERPASS_SET,
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryImageName::SCENE_COLOR), 0, eDescriptorAccess::READ, VK_SHADER_STAGE_FRAGMENT_BIT }
	}
};

#include "vk_debug.h"
#include "file_system.h"
void CopyRecordDrawCommandsBuffer( uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent, const RenderPass * renderpass, const Technique * technique )
{
	//TODO: we don't need extent, it's implicit by the framebuffer
	//TODO: don't let this code choose with "currentFrame" it doesn't need to know that.
	CmdBeginVkLabel( graphicsCommandBuffer, "Copy Renderpass", glm::vec4( 0.8f, 0.8f, 0.8f, 1.0f ) );
	const FrameBuffer& frameBuffer = renderpass->outputFrameBuffer[currentFrame];
	BeginRenderPass( graphicsCommandBuffer, *renderpass, frameBuffer.frameBuffer, frameBuffer.extent );

	vkCmdBindPipeline( graphicsCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, technique->pipeline );
	vkCmdBindDescriptorSets( graphicsCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, technique->pipelineLayout, 0, 1, &technique->descriptor_sets[RENDERPASS_SET].hw_descriptorSets[currentFrame], 0, nullptr );

	vkCmdDraw( graphicsCommandBuffer, 4, 1, 0, 0 );

	EndRenderPass( graphicsCommandBuffer );
	CmdEndVkLabel( graphicsCommandBuffer );
}

GpuPipelineLayout GetCopyPipelineLayout()
{
	return GpuPipelineLayout();
}

GpuPipelineState GetCopyPipelineState()
{
	GpuPipelineState gpuPipelineState = {};
	GetBindingDescription( VIBindings_PosColUV, &gpuPipelineState.viState );//unused

	gpuPipelineState.shaders = {
		{ FS::readFile( "shaders/copy.vert.spv" ), "main", VK_SHADER_STAGE_VERTEX_BIT },
		{ FS::readFile( "shaders/copy.frag.spv" ), "main", VK_SHADER_STAGE_FRAGMENT_BIT } };

	gpuPipelineState.rasterizationState.backFaceCulling = false;
	gpuPipelineState.rasterizationState.depthBiased = false;

	gpuPipelineState.depthStencilState.depthRead = false;
	gpuPipelineState.depthStencilState.depthWrite = false;

	gpuPipelineState.blendEnabled = false;
	gpuPipelineState.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	return gpuPipelineState;
}

static FG::RenderPassCreationData FG_Copy_CreateGraphNode( const Swapchain* swapchain, eTechniqueDataEntryImageName dst, eTechniqueDataEntryImageName src )
{
	FG::RenderPassCreationData renderPassCreationData;
	renderPassCreationData.name = "copy_pass";

	VkFormat swapchainFormat = swapchain->surfaceFormat.format;

	FG::RenderColor( renderPassCreationData, swapchainFormat, ( uint32_t )dst );
	FG::ReadResource( renderPassCreationData, ( uint32_t )src );

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData.frame_graph_node;
	frameGraphNode->RecordDrawCommands = CopyRecordDrawCommandsBuffer;

	frameGraphNode->gpuPipelineLayout = GpuPipelineLayout();
	frameGraphNode->gpuPipelineState = GetCopyPipelineState();
	//TODO: generalize instance and pass set into an array of binding point and set
	frameGraphNode->descriptorSets.push_back( copyPassSet );

	return renderPassCreationData;
}

FG::FrameGraph InitializeScript( const Swapchain* swapchain )
{
	//Setup resources
	VkFormat swapchainFormat = swapchain->surfaceFormat.format;
	VkExtent2D swapchainExtent = swapchain->extent;

	uint32_t backBufferId = (uint32_t) eTechniqueDataEntryImageName::BACKBUFFER;

	std::vector<FG::DataEntry> dataEntries ( techniqueDataEntries, techniqueDataEntries + sizeof( techniqueDataEntries ) / sizeof( techniqueDataEntries[0] ) );
	//TODO: get rid of this, when format is 0, just set it to swapchain format in frame graph
	dataEntries[( uint32_t )eTechniqueDataEntryImageName::SCENE_COLOR].resourceDesc.format = swapchainFormat;
	//dataEntries[( uint32_t )eTechniqueDataEntryImageName::BACKBUFFER].resourceDesc.format = swapchainFormat;
	//dataEntries[( uint32_t )eTechniqueDataEntryImageName::BACKBUFFER].resourceDesc.extent = swapchainExtent; // maybe not needed because FG does it

	//Setup passes
	//TODO: get rid of passing the swapchain
	std::vector<FG::RenderPassCreationData> rpCreationData;
	rpCreationData.push_back( FG_Geometry_CreateGraphNode( swapchain, eTechniqueDataEntryImageName::SCENE_COLOR, eTechniqueDataEntryImageName::SCENE_DEPTH ) );
	rpCreationData.push_back( FG_TextOverlay_CreateGraphNode( swapchain, eTechniqueDataEntryImageName::SCENE_COLOR ) );
	rpCreationData.push_back( FG_Copy_CreateGraphNode( swapchain, eTechniqueDataEntryImageName::BACKBUFFER, eTechniqueDataEntryImageName::SCENE_COLOR ) );

	FG::FrameGraph fg = FG::CreateGraph( swapchain, &rpCreationData, &dataEntries, backBufferId );

	FG::SetupInputBuffers( &fg, *_pInputBuffers );
	FG::CreateTechniques( &fg, _descriptorPool );
	FG::UpdateTechniqueDescriptorSets( &fg, *_pInputBuffers );

	return fg;
}
