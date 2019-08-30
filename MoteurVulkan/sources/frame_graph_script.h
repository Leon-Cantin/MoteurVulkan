#pragma once

#include "frame_graph.h"

#include "geometry_renderpass.h"
#include "shadow_renderpass.h"
#include "skybox.h"
#include "text_overlay.h"
#include "descriptors.h"

std::array< InputBuffers, SIMULTANEOUS_FRAMES>* _pInputBuffers;
VkDescriptorPool _descriptorPool;

void SetInputBuffers( std::array< InputBuffers, SIMULTANEOUS_FRAMES>* pInputBuffers, VkDescriptorPool descriptorPool )
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
	ALBEDOS = 0,
	NORMALS,
	SHADOWS,
	TEXT,
	SKYBOX,
	COUNT
};

inline void SetBuffers( InputBuffers* buffers, eTechniqueDataEntryName id, GpuBuffer* input )
{
	SetBuffers( buffers, static_cast< uint32_t >(id), input );
}

inline void SetImages( InputBuffers* buffers, eTechniqueDataEntryImageName id, VkDescriptorImageInfo* input )
{
	SetImages( buffers, static_cast< uint32_t >(id), input );
}

inline GpuBuffer* GetBuffer( const InputBuffers* buffers, eTechniqueDataEntryName id )
{
	return GetBuffer( buffers, static_cast< uint32_t >(id) );
}

inline VkDescriptorImageInfo* GetImage( const InputBuffers* buffers, eTechniqueDataEntryImageName id )
{
	return GetImage( buffers, static_cast< uint32_t >(id) );
}

const uint32_t maxModelsCount = 5;
static const TechniqueDataEntry techniqueDataEntries[static_cast< size_t >(eTechniqueDataEntryName::COUNT)] =
{
	{ static_cast< uint32_t >(eTechniqueDataEntryName::INSTANCE_DATA), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, eTechniqueDataEntryFlags::NONE, sizeof( InstanceMatrices ) * maxModelsCount},
	{ static_cast< uint32_t >(eTechniqueDataEntryName::SHADOW_DATA),	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, eTechniqueDataEntryFlags::NONE, sizeof( SceneMatricesUniform )},
	{ static_cast< uint32_t >(eTechniqueDataEntryName::SCENE_DATA),	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, eTechniqueDataEntryFlags::NONE, sizeof( SceneMatricesUniform )},
	{ static_cast< uint32_t >(eTechniqueDataEntryName::LIGHT_DATA),	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, eTechniqueDataEntryFlags::NONE, sizeof( LightUniform )},
	{ static_cast< uint32_t >(eTechniqueDataEntryName::SKYBOX_DATA),	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, eTechniqueDataEntryFlags::NONE, sizeof( SkyboxUniformBufferObject )},
};

static const TechniqueDataEntryImage techniqueDataEntryImages[static_cast< size_t >(eTechniqueDataEntryImageName::COUNT)] =
{
	{ static_cast< uint32_t >(eTechniqueDataEntryImageName::ALBEDOS), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5 },
	{ static_cast< uint32_t >(eTechniqueDataEntryImageName::NORMALS), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
	{ static_cast< uint32_t >(eTechniqueDataEntryImageName::SHADOWS), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
	{ static_cast< uint32_t >(eTechniqueDataEntryImageName::TEXT), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
	{ static_cast< uint32_t >(eTechniqueDataEntryImageName::SKYBOX), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
};

std::array<PerFrameBuffer, 16> _allbuffers;

void HACKCleanUpFrameGraphScriptResources()
{
	for( uint32_t i = 0; i < _allbuffers.size(); ++i )
	{
		if( _allbuffers[i].memory )
			DestroyPerFrameBuffer( &_allbuffers[i] );
	}
}

enum eRenderTarget : uint32_t
{
	RT_SCENE_COLOR = 0,
	RT_SCENE_DEPTH,
	RT_SHADOW_MAP,
	RT_COUNT
};

TechniqueDescriptorSetDesc geoPassSetDesc =
{
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryName::SCENE_DATA), 0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },
		{ static_cast< uint32_t >(eTechniqueDataEntryName::LIGHT_DATA), 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },
	},
	2,
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryImageName::ALBEDOS), 2, VK_SHADER_STAGE_FRAGMENT_BIT },
		{ static_cast< uint32_t >(eTechniqueDataEntryImageName::NORMALS), 3, VK_SHADER_STAGE_FRAGMENT_BIT },
		{ static_cast< uint32_t >(eTechniqueDataEntryImageName::SHADOWS), 4, VK_SHADER_STAGE_FRAGMENT_BIT },
	},
	3
};

TechniqueDescriptorSetDesc geoInstanceSetDesc =
{
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryName::INSTANCE_DATA), 0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }
	},
	1
};


static void FG_Geometry_CreateGraphNode(FG::RenderPassCreationData* renderPassCreationData, const Swapchain* swapchain)
{
	renderPassCreationData->name = "geometry_pass";

	VkFormat swapchainFormat = swapchain->surfaceFormat.format;

	FG::CreateColor(*renderPassCreationData, swapchainFormat, RT_SCENE_COLOR);
	FG::CreateDepth(*renderPassCreationData, VK_FORMAT_D32_SFLOAT, RT_SCENE_DEPTH);
	FG::ClearLast(*renderPassCreationData);
	FG::ReadResource(*renderPassCreationData, RT_SHADOW_MAP);

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData->frame_graph_node;

	frameGraphNode->gpuPipelineLayout = GetGeoPipelineLayout();
	frameGraphNode->gpuPipelineState = GetGeoPipelineState();

	frameGraphNode->RecordDrawCommands = GeometryRecordDrawCommandsBuffer;
	frameGraphNode->instanceSet = &geoInstanceSetDesc;
	frameGraphNode->passSet = &geoPassSetDesc;
}

TechniqueDescriptorSetDesc shadowPassSet =
{
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryName::SHADOW_DATA), 0, VK_SHADER_STAGE_VERTEX_BIT }
	},
	1
};

TechniqueDescriptorSetDesc shadowInstanceSet =
{
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryName::INSTANCE_DATA), 0, VK_SHADER_STAGE_VERTEX_BIT }
	},
	1
};

static void FG_Shadow_CreateGraphNode(FG::RenderPassCreationData* renderPassCreationData, const Swapchain* swapchain)
{
	renderPassCreationData->name = "shadow_pass";

	FG::CreateDepth(*renderPassCreationData, VK_FORMAT_D32_SFLOAT, RT_SHADOW_MAP);
	FG::ClearLast(*renderPassCreationData);

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData->frame_graph_node;

	frameGraphNode->gpuPipelineLayout = GetShadowPipelineLayout();
	frameGraphNode->gpuPipelineState = GetShadowPipelineState();
	frameGraphNode->RecordDrawCommands = ShadowRecordDrawCommandsBuffer;
	frameGraphNode->instanceSet = &shadowInstanceSet;
	frameGraphNode->passSet = &shadowPassSet;
}

TechniqueDescriptorSetDesc skyboxPassSetDesc =
{
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryName::SKYBOX_DATA), 0, VK_SHADER_STAGE_VERTEX_BIT },
	},
	1,
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryImageName::SKYBOX), 1, VK_SHADER_STAGE_FRAGMENT_BIT },
	},
	1
};

static void FG_Skybox_CreateGraphNode(FG::RenderPassCreationData* renderPassCreationData, const Swapchain* swapchain)
{
	renderPassCreationData->name = "skybox_pass";

	VkFormat swapchainFormat = swapchain->surfaceFormat.format;

	FG::CreateColor(*renderPassCreationData, swapchainFormat, RT_SCENE_COLOR);
	FG::CreateDepth(*renderPassCreationData, VK_FORMAT_D32_SFLOAT, RT_SCENE_DEPTH);

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData->frame_graph_node;

	frameGraphNode->gpuPipelineLayout = GetSkyboxPipelineLayout();
	frameGraphNode->gpuPipelineState = GetSkyboxPipelineState();
	frameGraphNode->RecordDrawCommands = SkyboxRecordDrawCommandsBuffer;
	frameGraphNode->instanceSet = nullptr;
	frameGraphNode->passSet = &skyboxPassSetDesc;
}

TechniqueDescriptorSetDesc textPassSet =
{
	{},
	0,
	{
		{ static_cast< uint32_t >(eTechniqueDataEntryImageName::TEXT), 0, VK_SHADER_STAGE_FRAGMENT_BIT }
	},
	1
};

static void FG_TextOverlay_CreateGraphNode(FG::RenderPassCreationData* renderPassCreationData, const Swapchain* swapchain)
{
	renderPassCreationData->name = "skybox_pass";

	VkFormat swapchainFormat = swapchain->surfaceFormat.format;

	FG::CreateColor(*renderPassCreationData, swapchainFormat, RT_SCENE_COLOR);

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData->frame_graph_node;

	frameGraphNode->gpuPipelineLayout = GetTextPipelineLayout();
	frameGraphNode->gpuPipelineState = GetTextPipelineState();
	frameGraphNode->RecordDrawCommands = TextRecordDrawCommandsBuffer;
	frameGraphNode->instanceSet = nullptr;
	//TODO: we don't need one set per frame for this one
	frameGraphNode->passSet = &textPassSet;
}

static void CreateDescriptorSetLayout( const TechniqueDescriptorSetDesc * desc, VkDescriptorSetLayout * o_setLayout )
{
	std::array<VkDescriptorSetLayoutBinding, 8> tempBindings;
	uint32_t count = 0;

	for( uint32_t i = 0; i < desc->buffersCount; ++i, ++count )
	{
		const TechniqueDataBinding* dataBinding = &desc->buffersBindings[i];
		const TechniqueDataEntry* dataEntry = &techniqueDataEntries[dataBinding->id];

		tempBindings[count].binding = dataBinding->binding;
		tempBindings[count].descriptorCount = dataEntry->count;
		tempBindings[count].descriptorType = dataEntry->descriptorType;
		tempBindings[count].stageFlags = dataBinding->stageFlags;
		tempBindings[count].pImmutableSamplers = nullptr;
	}

	for( uint32_t i = 0; i < desc->imagesCount; ++i, ++count )
	{
		const TechniqueDataBinding* dataBinding = &desc->imagesBindings[i];
		const TechniqueDataEntryImage* dataEntry = &techniqueDataEntryImages[dataBinding->id];

		tempBindings[count].binding = dataBinding->binding;
		tempBindings[count].descriptorCount = dataEntry->count;
		tempBindings[count].descriptorType = dataEntry->descriptorType;
		tempBindings[count].stageFlags = dataBinding->stageFlags;
		tempBindings[count].pImmutableSamplers = nullptr;
	}

	CreateDesciptorSetLayout( tempBindings.data(), count, o_setLayout );
}

static void CreateDescriptorSet( const InputBuffers* inputData, const TechniqueDescriptorSetDesc* descriptorSetDesc, VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool, VkDescriptorSet* o_descriptorSet )
{
	CreateDescriptorSets( descriptorPool, 1, &descriptorSetLayout, o_descriptorSet );

	assert( descriptorSetDesc->buffersCount + descriptorSetDesc->imagesCount <= 8 );
	WriteDescriptor writeDescriptors[8];
	uint32_t writeDescriptorsCount = 0;
	WriteDescriptorSet writeDescriptorSet = { writeDescriptors, descriptorSetDesc->buffersCount + descriptorSetDesc->imagesCount };

	//Fill in buffer
	VkDescriptorBufferInfo descriptorBuffersInfos[16];
	uint32_t descriptorBuffersInfosCount = 0;
	for( uint32_t i = 0; i < descriptorSetDesc->buffersCount; ++i )
	{
		const TechniqueDataBinding* bufferBinding = &descriptorSetDesc->buffersBindings[i];
		const TechniqueDataEntry* techniqueDataEntry = &techniqueDataEntries[bufferBinding->id];
		GpuBuffer* buffers = GetBuffer( inputData, bufferBinding->id );
		uint32_t bufferStart = descriptorBuffersInfosCount;
		for( uint32_t descriptorIndex = 0; descriptorIndex < techniqueDataEntry->count; ++descriptorIndex )
		{
			assert( descriptorBuffersInfosCount < 16 );
			descriptorBuffersInfos[descriptorBuffersInfosCount++] = { buffers[descriptorIndex].buffer, 0, VK_WHOLE_SIZE };
		}
		writeDescriptors[writeDescriptorsCount++] = { bufferBinding->binding, techniqueDataEntry->count, techniqueDataEntry->descriptorType, &descriptorBuffersInfos[bufferStart], nullptr };
	}

	//Fill in images
	VkDescriptorImageInfo descriptorImagesInfos[16];
	uint32_t descriptorImagesInfosCount = 0;
	for( uint32_t i = 0; i < descriptorSetDesc->imagesCount; ++i )
	{
		const TechniqueDataBinding* imageBinding = &descriptorSetDesc->imagesBindings[i];
		const TechniqueDataEntryImage* techniqueDataEntry = &techniqueDataEntryImages[imageBinding->id];
		VkDescriptorImageInfo* images = GetImage( inputData, imageBinding->id );
		uint32_t bufferStart = descriptorImagesInfosCount;
		for( uint32_t descriptorIndex = 0; descriptorIndex < techniqueDataEntry->count; ++descriptorIndex )
		{
			assert( descriptorImagesInfosCount < 16 );
			//TODO: HACK shouldn't pass in this struct in the input buffer, should be some wrapper or something
			descriptorImagesInfos[descriptorImagesInfosCount++] = images[descriptorIndex];
		}
		writeDescriptors[writeDescriptorsCount++] = { imageBinding->binding, techniqueDataEntry->count, techniqueDataEntry->descriptorType, nullptr, &descriptorImagesInfos[bufferStart] };
	}


	UpdateDescriptorSets( 1, &writeDescriptorSet, o_descriptorSet );
}

constexpr VkFormat RT_FORMAT_SHADOW_DEPTH = VK_FORMAT_D32_SFLOAT;
constexpr VkExtent2D RT_EXTENT_SHADOW = { 1024, 1024 };

static void CreateBuffersIfNotCreated( std::array< InputBuffers, SIMULTANEOUS_FRAMES>* inputBuffers, const TechniqueDescriptorSetDesc* descriptorSetDesc )
{
	for( uint32_t i = 0; i < descriptorSetDesc->buffersCount; ++i )
	{
		const TechniqueDataEntry* dataEntry = &techniqueDataEntries[descriptorSetDesc->buffersBindings[i].id];
		PerFrameBuffer* buffer = &_allbuffers[dataEntry->id];
		if( buffer->memory == VK_NULL_HANDLE )
		{
			CreatePerFrameBuffer( dataEntry->size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer );
			for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
				SetBuffers( &(*inputBuffers)[i], dataEntry->id, &buffer->buffers[i] );
		}
	}
}

void CreateTechniqueCallback (const RenderPass* renderpass, const FG::RenderPassCreationData* passCreationData, Technique* technique)
{
	std::array< InputBuffers, SIMULTANEOUS_FRAMES>& inputBuffers = *_pInputBuffers;

	//Create buffers if required
	if( passCreationData->frame_graph_node.passSet )
		CreateBuffersIfNotCreated( &inputBuffers, passCreationData->frame_graph_node.passSet );
	if( passCreationData->frame_graph_node.instanceSet )
		CreateBuffersIfNotCreated( &inputBuffers, passCreationData->frame_graph_node.instanceSet );

	//TODO: images? don't create stuff if it's external?

	//Create descriptors
	const GfxImage *shadowImages = FG::GetRenderTarget( RT_SHADOW_MAP );
	VkDescriptorImageInfo shadowTextures[] = { { GetSampler( Samplers::Shadow ), shadowImages->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } };
	for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
	{
		inputBuffers[i].dataImages[static_cast< size_t >(eTechniqueDataEntryImageName::SHADOWS)] = shadowTextures;
	}

	const TechniqueDescriptorSetDesc* passSet = passCreationData->frame_graph_node.passSet;
	const TechniqueDescriptorSetDesc* instanceSet = passCreationData->frame_graph_node.instanceSet;

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

void InitializeScript(const Swapchain* swapchain)
{
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
