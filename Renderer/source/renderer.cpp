#include "renderer.h"

#include "profile.h"
#include "frame_graph.h"
#include "gfx_heaps_batched_allocator.h"

#include <array>
#include <iostream>

namespace RNDR
{
	struct R_State
	{
		GfxCommandPool g_graphicsCommandPool;

		Swapchain g_swapchain;
		std::array<GfxCommandBuffer, SIMULTANEOUS_FRAMES> g_graphicsCommandBuffers;

		std::array<GfxSemaphore, SIMULTANEOUS_FRAMES> imageAvailableSemaphores;
		std::array<GfxSemaphore, SIMULTANEOUS_FRAMES> renderFinishedSemaphores;
		std::array<GfxFence, SIMULTANEOUS_FRAMES> end_of_frame_fences;

		FG::FrameGraph _frameGraph;
	};

	static void create_sync_objects( R_State* pr_state )
	{
		for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
		{
			if( !CreateGfxSemaphore( &pr_state->imageAvailableSemaphores[i] ) ||
				!CreateGfxSemaphore( &pr_state->renderFinishedSemaphores[i] ) ||
				!CreateGfxFence( &pr_state->end_of_frame_fences[i] ) )
			{
				throw std::runtime_error( "failed to create semaphores!" );
			}

			MarkGfxObject( pr_state->imageAvailableSemaphores[i], "image available semaphore" );
			MarkGfxObject( pr_state->renderFinishedSemaphores[i], "render finished semaphore" );
			MarkGfxObject( pr_state->end_of_frame_fences[i], "In flight fence" );
		}
	}

	VkExtent2D get_backbuffer_size( const R_State* pr_state )
	{
		return pr_state->g_swapchain.extent;
	}

	R_State* CreateRenderer( DisplaySurface swapchainSurface, uint64_t width, uint64_t height )
	{
		R_State* pr_state = new R_State();

		CreateSwapChain( swapchainSurface, width, height, pr_state->g_swapchain );

		CreateCommandPool( g_gfx.device.graphics_queue.queueFamilyIndex, &pr_state->g_graphicsCommandPool );
		CreateSingleUseCommandPool( g_gfx.device.graphics_queue.queueFamilyIndex, &g_gfx.graphicsSingleUseCommandPool );
		if( !CreateCommandBuffers( pr_state->g_graphicsCommandPool, pr_state->g_graphicsCommandBuffers.data(), static_cast< uint32_t >(pr_state->g_graphicsCommandBuffers.size()) ) )
			throw std::runtime_error( "failed to allocate command buffers!" );

		InitSamplers();

		create_sync_objects( pr_state );

		CreateTimeStampsQueryPool( SIMULTANEOUS_FRAMES );

		return pr_state;
	}

	void CompileFrameGraph( R_State* pr_state, FG_CompileScriptCallback_t FGScriptInitialize, void* fg_user_params )
	{
		DeviceWaitIdle( g_gfx.device.device );

		FG::Cleanup( &pr_state->_frameGraph );
		pr_state->_frameGraph = FGScriptInitialize( &pr_state->g_swapchain, fg_user_params );
	}

	void recreate_swap_chain( R_State* pr_state, DisplaySurface swapchainSurface, uint64_t width, uint64_t height, FG_CompileScriptCallback_t FGScriptInitialize, void* fg_user_params )
	{
		DeviceWaitIdle( g_gfx.device.device );

		Destroy( &pr_state->g_swapchain );

		//TODO: try to use the "oldSwapchain" parameter to optimize when recreating swap chains
		CreateSwapChain( swapchainSurface, width, height, pr_state->g_swapchain );

		CompileFrameGraph( pr_state, FGScriptInitialize, fg_user_params );
	}

	static void RecordCommandBuffer( R_State* pr_state, uint32_t currentFrame, const SceneFrameData* frameData )
	{
		GfxCommandBuffer graphicsCommandBuffer = pr_state->g_graphicsCommandBuffers[currentFrame];
		BeginCommandBufferRecording( graphicsCommandBuffer );

		CmdResetTimeStampSet( graphicsCommandBuffer, currentFrame );

		CmdWriteTimestamp( graphicsCommandBuffer, GFX_PIPELINE_STAGE_TOP_OF_PIPE_BIT, Timestamp::COMMAND_BUFFER_START, currentFrame );

		FG::RecordDrawCommands( currentFrame, const_cast< SceneFrameData* >(frameData), graphicsCommandBuffer, pr_state->g_swapchain.extent, &pr_state->_frameGraph );

		//TODO: Maybe make a present task so it changes the final layout in the frame graph?
		GfxImageBarrier( graphicsCommandBuffer, pr_state->g_swapchain.images[currentFrame].image, GfxLayout::COLOR, GfxAccess::WRITE, GfxLayout::PRESENT, GfxAccess::READ );

		CmdWriteTimestamp( graphicsCommandBuffer, GFX_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, Timestamp::COMMAND_BUFFER_END, currentFrame );

		EndCommandBufferRecording( graphicsCommandBuffer );
	}

	void WaitForFrame( const R_State* pr_state, uint32_t currentFrame )
	{
		WaitForFence( &pr_state->end_of_frame_fences[currentFrame], 1 );
	}

	eRenderError draw_frame( R_State* pr_state, uint32_t currentFrame, const SceneFrameData* frameData )
	{
		RecordCommandBuffer( pr_state, currentFrame, frameData );

		GfxSwapchainImage swapchainImage;
		const GfxSwapchainOperationResult aquireSwapChainImageResult = AcquireNextSwapchainImage( pr_state->g_swapchain.swapchain, pr_state->imageAvailableSemaphores[currentFrame], &swapchainImage );
		//TODO: Do I really need 2 check for recreate swap chain in this function?
		if( !SwapchainImageIsValid( aquireSwapChainImageResult ) ) {
			unsignalSemaphore( pr_state->imageAvailableSemaphores[currentFrame] );
			return eRenderError::NEED_FRAMEBUFFER_RESIZE;
		}

		//Submit work
		GfxSemaphore waitSemaphores[] = { pr_state->imageAvailableSemaphores[currentFrame] };
		GfxPipelineStageFlag waitStages[] = { GFX_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		GfxSemaphore signalSemaphores[] = { pr_state->renderFinishedSemaphores[currentFrame] }; //TODO make a system that keeps the semaphores ordered

		ResetGfxFences( &pr_state->end_of_frame_fences[currentFrame], 1 );

		if( !QueueSubmit( g_gfx.device.graphics_queue.queue, &pr_state->g_graphicsCommandBuffers[currentFrame], 1, waitSemaphores, waitStages, 1, signalSemaphores, 1, pr_state->end_of_frame_fences[currentFrame] ) )
			throw std::runtime_error( "failed to submit draw command buffer!" );

		//Present
		const GfxSwapchainOperationResult presentResult = QueuePresent( g_gfx.device.present_queue.queue, swapchainImage, &pr_state->renderFinishedSemaphores[currentFrame], 1 );
		if( !SwapchainImageIsValid( presentResult ) )
		{
			eRenderError::NEED_FRAMEBUFFER_RESIZE;
		}

		return eRenderError::SUCCESS;
	}

	void Destroy( R_State** ppr_state ) 
	{
		R_State* pr_state = *ppr_state;

		DestroyCommandBuffers( pr_state->g_graphicsCommandPool, pr_state->g_graphicsCommandBuffers.data(), static_cast< uint32_t >(pr_state->g_graphicsCommandBuffers.size()) );

		Destroy( &pr_state->g_swapchain );

		DestroySamplers();

		FG::Cleanup( &pr_state->_frameGraph );

		for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
		{
			DestroyGfxSemaphore( &pr_state->renderFinishedSemaphores[i] );
			DestroyGfxSemaphore( &pr_state->imageAvailableSemaphores[i] );
			DestroyGfxFence( &pr_state->end_of_frame_fences[i] );
		}

		Destroy( &pr_state->g_graphicsCommandPool );
		Destroy( &g_gfx.graphicsSingleUseCommandPool );

		DestroyTimeStampsPool();

		delete pr_state;
		*ppr_state = nullptr;
	}
}

void CmdBindVertexInputs( GfxCommandBuffer commandBuffer, const std::vector<VIBinding>& gpuPipelineVIBindings, const GfxModel& gfxModel )
{
	const uint32_t maxVertexInputBinding = 16;
	const uint32_t gpuPipelineVIBingindCount = gpuPipelineVIBindings.size();
	assert( gpuPipelineVIBingindCount <= maxVertexInputBinding );
	GfxApiBuffer vertexBuffers[maxVertexInputBinding];
	GfxDeviceSize offsets[maxVertexInputBinding];

	for( uint32_t i = 0; i < gpuPipelineVIBingindCount; ++i )
	{
		const GfxModelVertexInput* modelVI = GetVertexInput( gfxModel, ( eVIDataType )gpuPipelineVIBindings[i].desc.dataType );
		assert( gpuPipelineVIBindings[i].desc == modelVI->desc );
		//TODO: assert( modelVI.vertAttribBuffers is valid );
		vertexBuffers[i] = modelVI->buffer.buffer;
		offsets[i] = 0;
	}

	CmdBindVertexInputs( commandBuffer, vertexBuffers, 0, gpuPipelineVIBingindCount, offsets );
}

void CmdDrawIndexed( GfxCommandBuffer commandBuffer, const std::vector<VIBinding>& gpuPipelineVIBindings, const GfxModel& gfxModel, uint32_t indexCount )
{
	CmdBindVertexInputs( commandBuffer, gpuPipelineVIBindings, gfxModel );
	CmdBindIndexBuffer( commandBuffer, gfxModel.indexBuffer.buffer, 0, gfxModel.indexType );
	CmdDrawIndexed( commandBuffer, indexCount, 1, 0, 0, 0 );
}

void CmdDrawIndexed( GfxCommandBuffer commandBuffer, const std::vector<VIBinding>& gpuPipelineVIBindings, const GfxModel& gfxModel )
{
	CmdDrawIndexed( commandBuffer, gpuPipelineVIBindings, gfxModel, gfxModel.indexCount );
}