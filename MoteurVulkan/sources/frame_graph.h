#pragma once
#include "vk_globals.h"
#include "renderpass.h"

void CreateGraph(VkFormat swapchainFormat, std::vector<RenderPass>* o_renderPasses);
