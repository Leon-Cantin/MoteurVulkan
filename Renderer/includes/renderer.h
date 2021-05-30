#pragma once

#include "vk_globals.h"
#include "scene_frame_data.h"
#include "frame_graph.h"
#include "gfx_model.h"

#include <vector>

#include <glm/mat4x4.hpp>

namespace RNDR
{
	enum class eRenderError
	{
		SUCCESS,
		NEED_FRAMEBUFFER_RESIZE,
	};

	struct R_State;

	typedef FG::FrameGraph FG_CompileScriptCallback_t( const R_HW::Swapchain*, void* user_params );

	R_State* CreateRenderer( R_HW::DisplaySurface swapchainSurface, uint64_t width, uint64_t height );
	void CompileFrameGraph( R_State* pr_state, FG_CompileScriptCallback_t FGScriptInitialize, void* fg_user_params );
	void recreate_swap_chain( R_State* pr_state, R_HW::DisplaySurface swapchainSurface, uint64_t width, uint64_t height, FG_CompileScriptCallback_t FGScriptInitialize, void* fg_user_params );
	void Destroy( R_State** ppr_state );
	void WaitForFrame( const R_State* pr_state, uint32_t currentFrame );
	eRenderError draw_frame( R_State* pr_state, uint32_t currentFrame, const SceneFrameData* frameData );
	VkExtent2D get_backbuffer_size( const R_State* pr_state );
}


void CmdBindVertexInputs( R_HW::GfxCommandBuffer commandBuffer, const std::vector<R_HW::VIBinding>& gpuPipelineVIBindings, const GfxModel& gfxModel );
void CmdDrawIndexed( R_HW::GfxCommandBuffer commandBuffer, const std::vector<R_HW::VIBinding>& gpuPipelineVIBindings, const GfxModel& gfxModel, uint32_t indexCount );
void CmdDrawIndexed( R_HW::GfxCommandBuffer commandBuffer, const std::vector<R_HW::VIBinding>& gpuPipelineVIBindings, const GfxModel& gfxModel );