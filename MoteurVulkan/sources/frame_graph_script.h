#pragma once

#include "frame_graph.h"

#include "geometry_renderpass.h"
#include "shadow_renderpass.h"
#include "skybox.h"
#include "text_overlay.h"

enum eRenderTarget : uint32_t
{
	RT_SCENE_COLOR = 0,
	RT_SCENE_DEPTH,
	RT_SHADOW_MAP,
	RT_COUNT
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
}

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
}

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
}

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
}

constexpr VkFormat RT_FORMAT_SHADOW_DEPTH = VK_FORMAT_D32_SFLOAT;
constexpr VkExtent2D RT_EXTENT_SHADOW = { 1024, 1024 };

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

	FG::CreateGraph(swapchain, &_rpCreationData, &_rtCreationData, backBuffer);
}
