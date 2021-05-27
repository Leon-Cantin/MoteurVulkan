#pragma once

#include "vk_globals.h"
#include "scene_frame_data.h"
#include "frame_graph.h"
#include "gfx_model.h"

#include <vector>

#include <glm/mat4x4.hpp>

enum class eRenderError
{
	SUCCESS,
	NEED_FRAMEBUFFER_RESIZE,
};

typedef FG::FrameGraph FG_CompileScriptCallback_t (const Swapchain*, void* user_params );

void InitRenderer( DisplaySurface swapchainSurface, uint64_t width, uint64_t height );
void CompileFrameGraph( FG_CompileScriptCallback_t FGScriptInitialize, void* fg_user_params );
void recreate_swap_chain( DisplaySurface swapchainSurface, uint64_t width, uint64_t height, FG_CompileScriptCallback_t FGScriptInitialize, void* fg_user_params );
void CleanupRenderer();
void WaitForFrame(uint32_t currentFrame);
eRenderError draw_frame(uint32_t currentFrame, const SceneFrameData* frameData);
void CmdBindVertexInputs( GfxCommandBuffer commandBuffer, const std::vector<VIBinding>& gpuPipelineVIBindings, const GfxModel& gfxModel );
void CmdDrawIndexed( GfxCommandBuffer commandBuffer, const std::vector<VIBinding>& gpuPipelineVIBindings, const GfxModel& gfxModel, uint32_t indexCount );
void CmdDrawIndexed( GfxCommandBuffer commandBuffer, const std::vector<VIBinding>& gpuPipelineVIBindings, const GfxModel& gfxModel );
