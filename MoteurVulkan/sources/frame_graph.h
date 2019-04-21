#pragma once
#include "vk_globals.h"
#include "renderpass.h"

enum eRenderTarget
{
	RT_SCENE_COLOR = 0,
	RT_SCENE_DEPTH,
	RT_SHADOW_MAP,
	RT_COUNT
};


void CreateGraph(VkFormat swapchainFormat, std::vector<RenderPass>* o_renderPasses);
const GfxImage* GetRenderTarget(eRenderTarget render_target_id);
void FG_CleanupResources();
