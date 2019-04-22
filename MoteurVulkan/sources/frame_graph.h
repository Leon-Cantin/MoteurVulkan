#pragma once
#include "vk_globals.h"
#include "renderpass.h"
#include "swapchain.h"

enum eRenderTarget
{
	RT_SCENE_COLOR = 0,
	RT_SCENE_DEPTH,
	RT_SHADOW_MAP,
	RT_COUNT
};


void CreateGraph(const Swapchain* swapchain);
const RenderPass* GetRenderPass(uint32_t id);
const GfxImage* GetRenderTarget(eRenderTarget render_target_id);
void FG_RecreateAfterSwapchain(const Swapchain* swapchain);
void FG_CleanupAfterSwapchain();
void FG_CleanupResources();
