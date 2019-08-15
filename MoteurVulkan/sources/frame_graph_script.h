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
		{ eTechniqueDataEntryName::SCENE_DATA, 0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },
		{ eTechniqueDataEntryName::LIGHT_DATA, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },
	},
	2,
	{
		{ eTechniqueDataEntryImageName::ALBEDOS, 2, VK_SHADER_STAGE_FRAGMENT_BIT },
		{ eTechniqueDataEntryImageName::NORMALS, 3, VK_SHADER_STAGE_FRAGMENT_BIT },
		{ eTechniqueDataEntryImageName::SHADOWS, 4, VK_SHADER_STAGE_FRAGMENT_BIT },
	},
	3
};

TechniqueDescriptorSetDesc geoInstanceSetDesc =
{
	{
		{ eTechniqueDataEntryName::INSTANCE_DATA, 0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }
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

	frameGraphNode->Initialize = InitializeGeometryRenderPass;
	frameGraphNode->RecreateAfterSwapchain = RecreateGeometryAfterSwapChain;
	frameGraphNode->CleanupAfterSwapchain = CleanupGeometryRenderpassAfterSwapchain;
	frameGraphNode->Cleanup = CleanupGeometryRenderpass;
	frameGraphNode->RecordDrawCommands = GeometryRecordDrawCommandsBuffer;
	frameGraphNode->instanceSet = &geoInstanceSetDesc;
	frameGraphNode->passSet = &geoPassSetDesc;
}

TechniqueDescriptorSetDesc shadowPassSet =
{
	{
		{ eTechniqueDataEntryName::SHADOW_DATA, 0, VK_SHADER_STAGE_VERTEX_BIT }
	},
	1
};

TechniqueDescriptorSetDesc shadowInstanceSet =
{
	{
		{ eTechniqueDataEntryName::INSTANCE_DATA, 0, VK_SHADER_STAGE_VERTEX_BIT }
	},
	1
};

static void FG_Shadow_CreateGraphNode(FG::RenderPassCreationData* renderPassCreationData, const Swapchain* swapchain)
{
	renderPassCreationData->name = "shadow_pass";

	FG::CreateDepth(*renderPassCreationData, VK_FORMAT_D32_SFLOAT, RT_SHADOW_MAP);
	FG::ClearLast(*renderPassCreationData);

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData->frame_graph_node;

	frameGraphNode->Initialize = InitializeShadowPass;
	frameGraphNode->RecreateAfterSwapchain = nullptr;
	frameGraphNode->CleanupAfterSwapchain = nullptr;
	frameGraphNode->Cleanup = CleanupShadowPass;
	frameGraphNode->RecordDrawCommands = ShadowRecordDrawCommandsBuffer;
	frameGraphNode->instanceSet = &shadowInstanceSet;
	frameGraphNode->passSet = &shadowPassSet;
}

TechniqueDescriptorSetDesc skyboxPassSetDesc =
{
	{
		{ eTechniqueDataEntryName::SKYBOX_DATA, 0, VK_SHADER_STAGE_VERTEX_BIT },
	},
	1,
	{
		{ eTechniqueDataEntryImageName::SKYBOX, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
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

	frameGraphNode->Initialize = InitializeSkyboxRenderPass;
	frameGraphNode->RecreateAfterSwapchain = RecreateSkyboxAfterSwapchain;
	frameGraphNode->CleanupAfterSwapchain = CleanupSkyboxAfterSwapchain;
	frameGraphNode->Cleanup = CleanupSkybox;
	frameGraphNode->RecordDrawCommands = SkyboxRecordDrawCommandsBuffer;
	frameGraphNode->instanceSet = nullptr;
	frameGraphNode->passSet = &skyboxPassSetDesc;
}

TechniqueDescriptorSetDesc textPassSet =
{
	{},
	0,
	{
		{ eTechniqueDataEntryImageName::TEXT, 0, VK_SHADER_STAGE_FRAGMENT_BIT }
	},
	1
};

static void FG_TextOverlay_CreateGraphNode(FG::RenderPassCreationData* renderPassCreationData, const Swapchain* swapchain)
{
	renderPassCreationData->name = "skybox_pass";

	VkFormat swapchainFormat = swapchain->surfaceFormat.format;

	FG::CreateColor(*renderPassCreationData, swapchainFormat, RT_SCENE_COLOR);

	FG::FrameGraphNode* frameGraphNode = &renderPassCreationData->frame_graph_node;

	frameGraphNode->Initialize = InitializeTextRenderPass;
	frameGraphNode->RecreateAfterSwapchain = RecreateTextRenderPassAfterSwapchain;
	frameGraphNode->CleanupAfterSwapchain = CleanupTextRenderPassAfterSwapchain;
	frameGraphNode->Cleanup = CleanupTextRenderPass;
	frameGraphNode->RecordDrawCommands = TextRecordDrawCommandsBuffer;
	frameGraphNode->instanceSet = nullptr;
	//TODO: we don't need one set per frame for this one
	frameGraphNode->passSet = &textPassSet;
}

constexpr VkFormat RT_FORMAT_SHADOW_DEPTH = VK_FORMAT_D32_SFLOAT;
constexpr VkExtent2D RT_EXTENT_SHADOW = { 1024, 1024 };

void CreateTechniqueCallback (const FG::RenderPassCreationData* passCreationData, Technique* technique)
{
	std::array< InputBuffers, SIMULTANEOUS_FRAMES>& inputBuffers = *_pInputBuffers;

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
