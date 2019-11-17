#pragma once

#include "vk_globals.h"
#include "scene_instance.h"
#include "scene_frame_data.h"
#include "swapchain.h"
#include "frame_graph.h"

#include <vector>

#include <glm/mat4x4.hpp>

void InitRenderer( VkSurfaceKHR swapchainSurface, bool( *needResize )(), void( *getFrameBufferSize )(uint64_t* width, uint64_t* height) );
void CompileFrameGraph( FG::FrameGraph( *FGScriptInitialize )(const Swapchain* swapchain) );
void CleanupRenderer();
void WaitForFrame(uint32_t currentFrame);
void draw_frame(uint32_t currentFrame, const SceneFrameData* frameData);
void CmdDrawIndexed( VkCommandBuffer commandBuffer, const std::vector<VIBinding>& gpuPipelineVIBindings, const GfxModel& gfxModel );
