#pragma once

#include "frame_graph.h"

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
	INSTANCE_DATA = 0,
	SHADOW_DATA,
	SCENE_DATA,
	LIGHT_DATA,
	SKYBOX_DATA,
	COUNT
};

enum class eTechniqueDataEntryImageName
{
	ALBEDOS = static_cast<uint32_t>(eTechniqueDataEntryName::COUNT),
	NORMALS,
	SHADOWS,
	TEXT,
	SKYBOX,
	COUNT
};

const uint32_t maxModelsCount = 5;
static const TechniqueDataEntry techniqueDataEntries[static_cast< size_t >(eTechniqueDataEntryImageName::COUNT)] =
{
	//Buffers
	{ static_cast< uint32_t >(eTechniqueDataEntryName::INSTANCE_DATA), BUFFER_DYNAMIC, 1, eTechniqueDataEntryFlags::NONE, sizeof( InstanceMatrices ) * maxModelsCount},
	{ static_cast< uint32_t >(eTechniqueDataEntryName::SHADOW_DATA), BUFFER, 1, eTechniqueDataEntryFlags::NONE, sizeof( SceneMatricesUniform )},
	{ static_cast< uint32_t >(eTechniqueDataEntryName::SCENE_DATA),	BUFFER, 1, eTechniqueDataEntryFlags::NONE, sizeof( SceneMatricesUniform )},
	{ static_cast< uint32_t >(eTechniqueDataEntryName::LIGHT_DATA),	BUFFER, 1, eTechniqueDataEntryFlags::NONE, sizeof( LightUniform )},
	{ static_cast< uint32_t >(eTechniqueDataEntryName::SKYBOX_DATA), BUFFER, 1, eTechniqueDataEntryFlags::NONE, sizeof( SkyboxUniformBufferObject )},

	//images
	{ static_cast< uint32_t >(eTechniqueDataEntryImageName::ALBEDOS), eDescriptorType::IMAGE_SAMPLER, 5, eTechniqueDataEntryFlags::EXTERNAL, 0 },
	{ static_cast< uint32_t >(eTechniqueDataEntryImageName::NORMALS), eDescriptorType::IMAGE_SAMPLER, 1, eTechniqueDataEntryFlags::EXTERNAL, 0 },
	{ static_cast< uint32_t >(eTechniqueDataEntryImageName::SHADOWS), eDescriptorType::IMAGE_SAMPLER, 1, eTechniqueDataEntryFlags::NONE, 0 },
	{ static_cast< uint32_t >(eTechniqueDataEntryImageName::TEXT), eDescriptorType::IMAGE_SAMPLER, 1, eTechniqueDataEntryFlags::EXTERNAL, 0 },
	{ static_cast< uint32_t >(eTechniqueDataEntryImageName::SKYBOX), eDescriptorType::IMAGE_SAMPLER, 1, eTechniqueDataEntryFlags::EXTERNAL, 0 },
};

const TechniqueDataEntry* GetDataEntry( uint32_t entryId )
{
	const TechniqueDataEntry* dataEntry = &techniqueDataEntries[entryId];
	assert( dataEntry->id == entryId );
	return dataEntry;
}

std::array<PerFrameBuffer, static_cast< size_t >(eTechniqueDataEntryImageName::COUNT)> _allbuffers;
std::array<VkDescriptorImageInfo, static_cast< size_t >(eTechniqueDataEntryImageName::COUNT)> _allImages;

inline void SetBuffers( GpuInputData* buffers, eTechniqueDataEntryName id, GpuBuffer* input )
{
	SetBuffers( buffers, static_cast< uint32_t >(id), input );
}

inline void SetImages( GpuInputData* buffers, eTechniqueDataEntryImageName id, VkDescriptorImageInfo* input )
{
	SetImages( buffers, static_cast< uint32_t >(id), input );
}

inline GpuBuffer* GetBuffer( const GpuInputData* buffers, eTechniqueDataEntryName id )
{
	return GetBuffer( buffers, static_cast< uint32_t >(id) );
}

inline VkDescriptorImageInfo* GetImage( const GpuInputData* buffers, eTechniqueDataEntryImageName id )
{
	return GetImage( buffers, static_cast< uint32_t >(id) );
}

void HACKCleanUpFrameGraphScriptResources()
{
	for( uint32_t i = 0; i < _allbuffers.size(); ++i )
	{
		if( _allbuffers[i].memory )
			DestroyPerFrameBuffer( &_allbuffers[i] );
	}

	_allImages = {};
}

enum eRenderTarget : uint32_t
{
	RT_SCENE_COLOR = 0,
	RT_SCENE_DEPTH,
	RT_SHADOW_MAP,
	RT_COUNT
};

struct ImageDataEntrySomething
{
	eRenderTarget renderTarget;
	Samplers sampler;
};
std::unordered_map< eTechniqueDataEntryImageName, ImageDataEntrySomething > dataEntryToFGImage = { {eTechniqueDataEntryImageName::SHADOWS, { RT_SHADOW_MAP, Samplers::Shadow } } };

TechniqueDescriptorSetDesc geoPassSetDesc =
{
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryName::SCENE_DATA), 0, eDescriptorAccess::READ, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },
		{ static_cast< uint32_t >(eTechniqueDataEntryName::LIGHT_DATA), 1, eDescriptorAccess::READ, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },

		{ static_cast< uint32_t >(eTechniqueDataEntryImageName::ALBEDOS), 2, eDescriptorAccess::READ, VK_SHADER_STAGE_FRAGMENT_BIT },
		{ static_cast< uint32_t >(eTechniqueDataEntryImageName::NORMALS), 3, eDescriptorAccess::READ, VK_SHADER_STAGE_FRAGMENT_BIT },
		{ static_cast< uint32_t >(eTechniqueDataEntryImageName::SHADOWS), 4, eDescriptorAccess::READ, VK_SHADER_STAGE_FRAGMENT_BIT },
	},
	5
};

TechniqueDescriptorSetDesc geoInstanceSetDesc =
{
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryName::INSTANCE_DATA), 0, eDescriptorAccess::READ, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }
	},
	1
};


static void FG_Geometry_CreateGraphNode(FG::RenderPassCreationData* renderPassCreationData, const Swapchain* swapchain)
{
	renderPassCreationData->name = "geometry_pass";

	VkFormat swapchainFormat = swapchain->surfaceFormat.format;

	FG::RenderColor(*renderPassCreationData, swapchainFormat, RT_SCENE_COLOR);
	FG::RenderDepth(*renderPassCreationData, VK_FORMAT_D32_SFLOAT, RT_SCENE_DEPTH);
	FG::ClearLast(*renderPassCreationData);
	FG::ReadResource(*renderPassCreationData, RT_SHADOW_MAP);

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData->frame_graph_node;
	frameGraphNode->RecordDrawCommands = GeometryRecordDrawCommandsBuffer;

	frameGraphNode->gpuPipelineLayout = GetGeoPipelineLayout();
	frameGraphNode->gpuPipelineState = GetGeoPipelineState();
	frameGraphNode->instanceSet = &geoInstanceSetDesc;
	frameGraphNode->passSet = &geoPassSetDesc;
}

TechniqueDescriptorSetDesc shadowPassSet =
{
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryName::SHADOW_DATA), 0, eDescriptorAccess::READ, VK_SHADER_STAGE_VERTEX_BIT }
	},
	1
};

TechniqueDescriptorSetDesc shadowInstanceSet =
{
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryName::INSTANCE_DATA), 0, eDescriptorAccess::READ, VK_SHADER_STAGE_VERTEX_BIT }
	},
	1
};

static void FG_Shadow_CreateGraphNode(FG::RenderPassCreationData* renderPassCreationData, const Swapchain* swapchain)
{
	renderPassCreationData->name = "shadow_pass";

	FG::RenderDepth(*renderPassCreationData, VK_FORMAT_D32_SFLOAT, RT_SHADOW_MAP);
	FG::ClearLast(*renderPassCreationData);

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData->frame_graph_node;
	frameGraphNode->RecordDrawCommands = ShadowRecordDrawCommandsBuffer;

	frameGraphNode->gpuPipelineLayout = GetShadowPipelineLayout();
	frameGraphNode->gpuPipelineState = GetShadowPipelineState();
	frameGraphNode->instanceSet = &shadowInstanceSet;
	frameGraphNode->passSet = &shadowPassSet;
}

TechniqueDescriptorSetDesc skyboxPassSetDesc =
{
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryName::SKYBOX_DATA), 0, eDescriptorAccess::READ, VK_SHADER_STAGE_VERTEX_BIT },

		{ static_cast< uint32_t >(eTechniqueDataEntryImageName::SKYBOX), 1, eDescriptorAccess::READ, VK_SHADER_STAGE_FRAGMENT_BIT },
	},
	2
};

static void FG_Skybox_CreateGraphNode(FG::RenderPassCreationData* renderPassCreationData, const Swapchain* swapchain)
{
	renderPassCreationData->name = "skybox_pass";

	VkFormat swapchainFormat = swapchain->surfaceFormat.format;

	FG::RenderColor(*renderPassCreationData, swapchainFormat, RT_SCENE_COLOR);
	FG::RenderDepth(*renderPassCreationData, VK_FORMAT_D32_SFLOAT, RT_SCENE_DEPTH);

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData->frame_graph_node;
	frameGraphNode->RecordDrawCommands = SkyboxRecordDrawCommandsBuffer;

	frameGraphNode->gpuPipelineLayout = GetSkyboxPipelineLayout();
	frameGraphNode->gpuPipelineState = GetSkyboxPipelineState();
	frameGraphNode->instanceSet = nullptr;
	frameGraphNode->passSet = &skyboxPassSetDesc;
}

TechniqueDescriptorSetDesc textPassSet =
{
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryImageName::TEXT), 0, eDescriptorAccess::READ, VK_SHADER_STAGE_FRAGMENT_BIT }
	},
	1
};

static void FG_TextOverlay_CreateGraphNode(FG::RenderPassCreationData* renderPassCreationData, const Swapchain* swapchain)
{
	renderPassCreationData->name = "skybox_pass";

	VkFormat swapchainFormat = swapchain->surfaceFormat.format;

	FG::RenderColor(*renderPassCreationData, swapchainFormat, RT_SCENE_COLOR);

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData->frame_graph_node;
	frameGraphNode->RecordDrawCommands = TextRecordDrawCommandsBuffer;

	frameGraphNode->gpuPipelineLayout = GetTextPipelineLayout();
	frameGraphNode->gpuPipelineState = GetTextPipelineState();
	frameGraphNode->instanceSet = nullptr;
	//TODO: we don't need one set per frame for this one
	frameGraphNode->passSet = &textPassSet;
}

static VkDescriptorType DescriptorTypeToVkType( eDescriptorType type, eDescriptorAccess access )
{
	switch( type )
	{
		case eDescriptorType::BUFFER :
			return access ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case eDescriptorType::BUFFER_DYNAMIC:
			return access ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		case eDescriptorType::IMAGE:
			return access ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case eDescriptorType::SAMPLER:
			assert( access == eDescriptorAccess::READ );
			return VK_DESCRIPTOR_TYPE_SAMPLER;
		case eDescriptorType::IMAGE_SAMPLER:
			assert( access == eDescriptorAccess::READ );
			return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	}
}

static VkDescriptorSetLayoutBinding CreateSetLayoutBinding( const TechniqueDataBinding* dataBinding, const TechniqueDataEntry* dataEntry )
{
	VkDescriptorSetLayoutBinding layoutBinding;
	layoutBinding.binding = dataBinding->binding;
	layoutBinding.descriptorCount = dataEntry->count;
	layoutBinding.descriptorType = DescriptorTypeToVkType( dataEntry->descriptorType, dataBinding->descriptorAccess );
	layoutBinding.stageFlags = dataBinding->stageFlags;
	layoutBinding.pImmutableSamplers = nullptr;
	return layoutBinding;
}

static void CreateDescriptorSetLayout( const TechniqueDescriptorSetDesc * desc, VkDescriptorSetLayout * o_setLayout )
{
	std::array<VkDescriptorSetLayoutBinding, 8> tempBindings;
	uint32_t count = 0;

	for( uint32_t i = 0; i < desc->dataCount; ++i, ++count )
	{
		const TechniqueDataBinding* dataBinding = &desc->dataBindings[i];
		const TechniqueDataEntry* dataEntry = GetDataEntry(dataBinding->id);

		tempBindings[count] = CreateSetLayoutBinding( dataBinding, dataEntry );
	}

	CreateDesciptorSetLayout( tempBindings.data(), count, o_setLayout );
}

static void CreateDescriptorSet( const GpuInputData* inputData, const TechniqueDescriptorSetDesc* descriptorSetDesc, VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool, VkDescriptorSet* o_descriptorSet )
{
	CreateDescriptorSets( descriptorPool, 1, &descriptorSetLayout, o_descriptorSet );

	assert( descriptorSetDesc->dataCount <= MAX_DATA_ENTRIES );
	WriteDescriptor writeDescriptors[8];
	uint32_t writeDescriptorsCount = 0;
	WriteDescriptorSet writeDescriptorSet = { writeDescriptors, descriptorSetDesc->dataCount };

	VkDescriptorBufferInfo descriptorBuffersInfos[16];
	uint32_t descriptorBuffersInfosCount = 0;
	VkDescriptorImageInfo descriptorImagesInfos[16];
	uint32_t descriptorImagesInfosCount = 0;

	//Fill in buffer
	for( uint32_t i = 0; i < descriptorSetDesc->dataCount; ++i )
	{
		const TechniqueDataBinding* dataBinding = &descriptorSetDesc->dataBindings[i];
		const TechniqueDataEntry* techniqueDataEntry = GetDataEntry( dataBinding->id );

		if( IsBufferType( techniqueDataEntry->descriptorType ) )//Buffers
		{
			uint32_t bufferStart = descriptorBuffersInfosCount;
			GpuBuffer* buffers = GetBuffer( inputData, dataBinding->id );
			for( uint32_t descriptorIndex = 0; descriptorIndex < techniqueDataEntry->count; ++descriptorIndex )
			{
				assert( descriptorBuffersInfosCount < 16 );
				descriptorBuffersInfos[descriptorBuffersInfosCount++] = { buffers[descriptorIndex].buffer, 0, VK_WHOLE_SIZE };
			}
			writeDescriptors[writeDescriptorsCount++] = { dataBinding->binding, techniqueDataEntry->count, DescriptorTypeToVkType( techniqueDataEntry->descriptorType, dataBinding->descriptorAccess ), &descriptorBuffersInfos[bufferStart], nullptr };
		}
		else if ( techniqueDataEntry->descriptorType == eDescriptorType::IMAGE_SAMPLER ) // Combined image samplers
		{
			VkDescriptorImageInfo* images = GetImage( inputData, dataBinding->id );
			uint32_t bufferStart = descriptorImagesInfosCount;

			for( uint32_t descriptorIndex = 0; descriptorIndex < techniqueDataEntry->count; ++descriptorIndex )
			{
				assert( descriptorImagesInfosCount < 16 );
				//TODO: HACK shouldn't pass in this struct in the input buffer, should be some wrapper or something
				descriptorImagesInfos[descriptorImagesInfosCount++] = images[descriptorIndex];
			}
			writeDescriptors[writeDescriptorsCount++] = { dataBinding->binding, techniqueDataEntry->count, DescriptorTypeToVkType( techniqueDataEntry->descriptorType, dataBinding->descriptorAccess ), nullptr, &descriptorImagesInfos[bufferStart] };
		}
		else
		{
			//TODO: Other image types not yet implemented
			assert( true );
		}
	}

	UpdateDescriptorSets( 1, &writeDescriptorSet, o_descriptorSet );
}

static void SetOrCreateDataIfNeeded( std::array< GpuInputData, SIMULTANEOUS_FRAMES>* inputBuffers, const TechniqueDescriptorSetDesc* descriptorSetDesc )
{
	for( uint32_t i = 0; i < descriptorSetDesc->dataCount; ++i )
	{
		const TechniqueDataBinding* dataBinding = &descriptorSetDesc->dataBindings[i];
		const TechniqueDataEntry* dataEntry = GetDataEntry( dataBinding->id );

		if( dataEntry->flags & eTechniqueDataEntryFlags::EXTERNAL )
			continue;

		if( IsBufferType( dataEntry->descriptorType ) )
		{
			PerFrameBuffer* buffer = &_allbuffers[dataEntry->id];
			if( buffer->memory == VK_NULL_HANDLE )
			{
				//TODO: could have to change VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT if we write (store).
				CreatePerFrameBuffer( dataEntry->size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer );
				for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
					SetBuffers( &(*inputBuffers)[i], dataEntry->id, &buffer->buffers[i] );
			}
		}
		else
		{
			const ImageDataEntrySomething& imageDataEntrySomething = dataEntryToFGImage.at( static_cast< eTechniqueDataEntryImageName >(dataEntry->id) );
			const GfxImage* image = FG::GetRenderTarget( imageDataEntrySomething.renderTarget );
			VkDescriptorImageInfo* imageInfo = &_allImages[dataEntry->id];
			if( imageInfo->imageView == VK_NULL_HANDLE )
			{
				//TODO: layout might be different for different shaders, probably just need write, works so far.
				*imageInfo = { GetSampler( imageDataEntrySomething.sampler ), image->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
				for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
					SetImages( &(*inputBuffers)[i], dataEntry->id, imageInfo );
			}
		}
	}
}

void CreateTechniqueCallback (const RenderPass* renderpass, const FG::RenderPassCreationData* passCreationData, Technique* technique)
{
	std::array< GpuInputData, SIMULTANEOUS_FRAMES>& inputBuffers = *_pInputBuffers;

	const TechniqueDescriptorSetDesc* passSet = passCreationData->frame_graph_node.passSet;
	const TechniqueDescriptorSetDesc* instanceSet = passCreationData->frame_graph_node.instanceSet;

	//Create buffers if required
	if( passSet )
		SetOrCreateDataIfNeeded( &inputBuffers, passSet );
	if( instanceSet )
		SetOrCreateDataIfNeeded( &inputBuffers, instanceSet );

	//Create descriptors
	if( passSet )
	{
		CreateDescriptorSetLayout( passSet, &technique->renderpass_descriptor_layout );
		//TODO: not all of them need one for each simultaneous frames
		for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
			CreateDescriptorSet( &inputBuffers[i], passSet, technique->renderpass_descriptor_layout, _descriptorPool, &technique->renderPass_descriptor[i] );
	}
	if( instanceSet )
	{
		CreateDescriptorSetLayout( instanceSet, &technique->instance_descriptor_layout );
		//TODO: not all of them need one for each simultaneous frames
		for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
			CreateDescriptorSet( &inputBuffers[i], instanceSet, technique->instance_descriptor_layout, _descriptorPool, &technique->instance_descriptor[i] );
	}
	technique->parentDescriptorPool = _descriptorPool;

	//Create pipeline layout
	uint32_t dscriptorSetLayoutsCount = 0;
	std::array< VkDescriptorSetLayout, 2 > descriptorSetLayouts;
	if( passSet )
		descriptorSetLayouts[dscriptorSetLayoutsCount++] = technique->renderpass_descriptor_layout;
	if( instanceSet )
		descriptorSetLayouts[dscriptorSetLayoutsCount++] = technique->instance_descriptor_layout;

	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = dscriptorSetLayoutsCount;
	pipeline_layout_info.pSetLayouts = descriptorSetLayouts.data();
	pipeline_layout_info.pushConstantRangeCount = passCreationData->frame_graph_node.gpuPipelineLayout.pushConstantRanges.size();
	pipeline_layout_info.pPushConstantRanges = passCreationData->frame_graph_node.gpuPipelineLayout.pushConstantRanges.data();

	if( vkCreatePipelineLayout( g_vk.device, &pipeline_layout_info, nullptr, &technique->pipelineLayout ) != VK_SUCCESS ) {
		throw std::runtime_error( "failed to create pipeline layout!" );
	}

	//TODO: temp hack to get a relative size;
	GpuPipelineState pipelineState = passCreationData->frame_graph_node.gpuPipelineState;
	if( pipelineState.framebufferExtent.width == 0 && pipelineState.framebufferExtent.height == 0 )
		pipelineState.framebufferExtent = renderpass->outputFrameBuffer[0].extent;

	CreatePipeline( pipelineState,
		renderpass->vk_renderpass,
		technique->pipelineLayout,
		&technique->pipeline );
}

constexpr VkFormat RT_FORMAT_SHADOW_DEPTH = VK_FORMAT_D32_SFLOAT;
constexpr VkExtent2D RT_EXTENT_SHADOW = { 1024, 1024 };

void InitializeScript(const Swapchain* swapchain)
{
	HACKCleanUpFrameGraphScriptResources();

	//Setup resources
	VkFormat swapchainFormat = swapchain->surfaceFormat.format;
	VkExtent2D swapchainExtent = swapchain->extent;
	std::vector<FG::RenderTargetCreationData> _rtCreationData;
	_rtCreationData.resize(RT_COUNT);

	eRenderTarget backBuffer = RT_SCENE_COLOR;

	_rtCreationData[RT_SCENE_COLOR] = { RT_SCENE_COLOR, swapchainFormat , swapchainExtent, (VkImageUsageFlagBits)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT), VK_IMAGE_ASPECT_COLOR_BIT,  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, true };
	_rtCreationData[RT_SCENE_DEPTH] = { RT_SCENE_DEPTH, VK_FORMAT_D32_SFLOAT , swapchainExtent, (VkImageUsageFlagBits)(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT), VK_IMAGE_ASPECT_DEPTH_BIT,  VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, true };
	_rtCreationData[RT_SHADOW_MAP] = { RT_SHADOW_MAP, RT_FORMAT_SHADOW_DEPTH , RT_EXTENT_SHADOW, (VkImageUsageFlagBits)(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT), VK_IMAGE_ASPECT_DEPTH_BIT,  VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	std::vector<FG::RenderPassCreationData> _rpCreationData;

	//Setup passes	
	FG::RenderPassCreationData shadowPass;
	FG_Shadow_CreateGraphNode(&shadowPass, swapchain);
	_rpCreationData.push_back(shadowPass);

	FG::RenderPassCreationData geoPass;
	FG_Geometry_CreateGraphNode(&geoPass, swapchain);
	_rpCreationData.push_back(geoPass);

	FG::RenderPassCreationData skyPass;
	FG_Skybox_CreateGraphNode(&skyPass, swapchain);
	_rpCreationData.push_back(skyPass);

	FG::RenderPassCreationData textPass;
	FG_TextOverlay_CreateGraphNode(&textPass, swapchain);
	_rpCreationData.push_back(textPass);

	FG::CreateGraph(swapchain, &_rpCreationData, &_rtCreationData, backBuffer, _descriptorPool, &CreateTechniqueCallback );
}
