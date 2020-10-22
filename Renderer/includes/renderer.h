#pragma once

#include "vk_globals.h"
#include "scene_frame_data.h"
#include "frame_graph.h"
#include "gfx_model.h"

#include <vector>

#include <glm/mat4x4.hpp>

void InitRenderer( DisplaySurface swapchainSurface, bool( *needResize )(), void( *getFrameBufferSize )(uint64_t* width, uint64_t* height) );
void CompileFrameGraph( FG::FrameGraph( *FGScriptInitialize )(const Swapchain* swapchain) );
void CleanupFrameGraph();
void CleanupRenderer();
void WaitForFrame(uint32_t currentFrame);
void draw_frame(uint32_t currentFrame, const SceneFrameData* frameData);
void CmdBindVertexInputs( GfxCommandBuffer commandBuffer, const std::vector<VIBinding>& gpuPipelineVIBindings, const GfxModel& gfxModel );
void CmdDrawIndexed( GfxCommandBuffer commandBuffer, const std::vector<VIBinding>& gpuPipelineVIBindings, const GfxModel& gfxModel, uint32_t indexCount );
void CmdDrawIndexed( GfxCommandBuffer commandBuffer, const std::vector<VIBinding>& gpuPipelineVIBindings, const GfxModel& gfxModel );
