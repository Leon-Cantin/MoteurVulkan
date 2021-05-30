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
		R_HW::GfxCommandPool g_graphicsCommandPool;

		R_HW::Swapchain g_swapchain;
		std::array<R_HW::GfxCommandBuffer, SIMULTANEOUS_FRAMES> g_graphicsCommandBuffers;

		std::array<R_HW::GfxSemaphore, SIMULTANEOUS_FRAMES> imageAvailableSemaphores;
		std::array<R_HW::GfxSemaphore, SIMULTANEOUS_FRAMES> renderFinishedSemaphores;
		std::array<R_HW::GfxFence, SIMULTANEOUS_FRAMES> end_of_frame_fences;

		FG::FrameGraph _frameGraph;
	};

	static void create_sync_objects( R_State* pr_state )
	{
		for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
		{
			if( !R_HW::CreateGfxSemaphore( &pr_state->imageAvailableSemaphores[i] ) ||
				!R_HW::CreateGfxSemaphore( &pr_state->renderFinishedSemaphores[i] ) ||
				!R_HW::CreateGfxFence( &pr_state->end_of_frame_fences[i] ) )
			{
				throw std::runtime_error( "failed to create semaphores!" );
			}

			R_HW::MarkGfxObject( pr_state->imageAvailableSemaphores[i], "image available semaphore" );
			R_HW::MarkGfxObject( pr_state->renderFinishedSemaphores[i], "render finished semaphore" );
			R_HW::MarkGfxObject( pr_state->end_of_frame_fences[i], "In flight fence" );
		}
	}

	VkExtent2D get_backbuffer_size( const R_State* pr_state )
	{
		return pr_state->g_swapchain.extent;
	}

	R_State* CreateRenderer( R_HW::DisplaySurface swapchainSurface, uint64_t width, uint64_t height )
	{
		R_State* pr_state = new R_State();

		CreateSwapChain( swapchainSurface, width, height, pr_state->g_swapchain );

		R_HW::CreateCommandPool( g_gfx.device.graphics_queue.queueFamilyIndex, &pr_state->g_graphicsCommandPool );
		R_HW::CreateSingleUseCommandPool( g_gfx.device.graphics_queue.queueFamilyIndex, &g_gfx.graphicsSingleUseCommandPool );
		if( !R_HW::CreateCommandBuffers( pr_state->g_graphicsCommandPool, pr_state->g_graphicsCommandBuffers.data(), static_cast< uint32_t >(pr_state->g_graphicsCommandBuffers.size()) ) )
			throw std::runtime_error( "failed to allocate command buffers!" );

		InitSamplers();

		create_sync_objects( pr_state );

		CreateTimeStampsQueryPool( SIMULTANEOUS_FRAMES );

		return pr_state;
	}

	void CompileFrameGraph( R_State* pr_state, FG_CompileScriptCallback_t FGScriptInitialize, void* fg_user_params )
	{
		R_HW::DeviceWaitIdle( g_gfx.device.device );

		FG::Cleanup( &pr_state->_frameGraph );
		pr_state->_frameGraph = FGScriptInitialize( &pr_state->g_swapchain, fg_user_params );
	}

	void recreate_swap_chain( R_State* pr_state, R_HW::DisplaySurface swapchainSurface, uint64_t width, uint64_t height, FG_CompileScriptCallback_t FGScriptInitialize, void* fg_user_params )
	{
		R_HW::DeviceWaitIdle( g_gfx.device.device );

		Destroy( &pr_state->g_swapchain );

		//TODO: try to use the "oldSwapchain" parameter to optimize when recreating swap chains
		CreateSwapChain( swapchainSurface, width, height, pr_state->g_swapchain );

		CompileFrameGraph( pr_state, FGScriptInitialize, fg_user_params );
	}

	static void RecordCommandBuffer( R_State* pr_state, uint32_t currentFrame, const SceneFrameData* frameData )
	{
		R_HW::GfxCommandBuffer graphicsCommandBuffer = pr_state->g_graphicsCommandBuffers[currentFrame];
		R_HW::BeginCommandBufferRecording( graphicsCommandBuffer );

		CmdResetTimeStampSet( graphicsCommandBuffer, currentFrame );

		CmdWriteTimestamp( graphicsCommandBuffer, R_HW::GFX_PIPELINE_STAGE_TOP_OF_PIPE_BIT, Timestamp::COMMAND_BUFFER_START, currentFrame );

		FG::RecordDrawCommands( currentFrame, const_cast< SceneFrameData* >(frameData), graphicsCommandBuffer, pr_state->g_swapchain.extent, &pr_state->_frameGraph );

		//TODO: Maybe make a present task so it changes the final layout in the frame graph?
		R_HW::GfxImageBarrier( graphicsCommandBuffer, pr_state->g_swapchain.images[currentFrame].image, R_HW::GfxLayout::COLOR, R_HW::GfxAccess::WRITE, R_HW::GfxLayout::PRESENT, R_HW::GfxAccess::READ );

		CmdWriteTimestamp( graphicsCommandBuffer, R_HW::GFX_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, Timestamp::COMMAND_BUFFER_END, currentFrame );

		R_HW::EndCommandBufferRecording( graphicsCommandBuffer );
	}

	void WaitForFrame( const R_State* pr_state, uint32_t currentFrame )
	{
		R_HW::WaitForFence( &pr_state->end_of_frame_fences[currentFrame], 1 );
	}

	eRenderError draw_frame( R_State* pr_state, uint32_t currentFrame, const SceneFrameData* frameData )
	{
		RecordCommandBuffer( pr_state, currentFrame, frameData );

		R_HW::GfxSwapchainImage swapchainImage;
		const R_HW::GfxSwapchainOperationResult aquireSwapChainImageResult = AcquireNextSwapchainImage( pr_state->g_swapchain.swapchain, pr_state->imageAvailableSemaphores[currentFrame], &swapchainImage );
		//TODO: Do I really need 2 check for recreate swap chain in this function?
		if( !R_HW::SwapchainImageIsValid( aquireSwapChainImageResult ) ) {
			R_HW::unsignalSemaphore( pr_state->imageAvailableSemaphores[currentFrame] );
			return eRenderError::NEED_FRAMEBUFFER_RESIZE;
		}

		//Submit work
		R_HW::GfxSemaphore waitSemaphores[] = { pr_state->imageAvailableSemaphores[currentFrame] };
		R_HW::GfxPipelineStageFlag waitStages[] = { R_HW::GFX_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		R_HW::GfxSemaphore signalSemaphores[] = { pr_state->renderFinishedSemaphores[currentFrame] }; //TODO make a system that keeps the semaphores ordered

		R_HW::ResetGfxFences( &pr_state->end_of_frame_fences[currentFrame], 1 );

		if( !R_HW::QueueSubmit( g_gfx.device.graphics_queue.queue, &pr_state->g_graphicsCommandBuffers[currentFrame], 1, waitSemaphores, waitStages, 1, signalSemaphores, 1, pr_state->end_of_frame_fences[currentFrame] ) )
			throw std::runtime_error( "failed to submit draw command buffer!" );

		//Present
		const R_HW::GfxSwapchainOperationResult presentResult = QueuePresent( g_gfx.device.present_queue.queue, swapchainImage, &pr_state->renderFinishedSemaphores[currentFrame], 1 );
		if( !R_HW::SwapchainImageIsValid( presentResult ) )
		{
			eRenderError::NEED_FRAMEBUFFER_RESIZE;
		}

		return eRenderError::SUCCESS;
	}

	void Destroy( R_State** ppr_state ) 
	{
		R_State* pr_state = *ppr_state;

		R_HW::DestroyCommandBuffers( pr_state->g_graphicsCommandPool, pr_state->g_graphicsCommandBuffers.data(), static_cast< uint32_t >(pr_state->g_graphicsCommandBuffers.size()) );

		Destroy( &pr_state->g_swapchain );

		DestroySamplers();

		FG::Cleanup( &pr_state->_frameGraph );

		for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
		{
			R_HW::DestroyGfxSemaphore( &pr_state->renderFinishedSemaphores[i] );
			R_HW::DestroyGfxSemaphore( &pr_state->imageAvailableSemaphores[i] );
			R_HW::DestroyGfxFence( &pr_state->end_of_frame_fences[i] );
		}

		R_HW::Destroy( &pr_state->g_graphicsCommandPool );
		R_HW::Destroy( &g_gfx.graphicsSingleUseCommandPool );

		DestroyTimeStampsPool();

		delete pr_state;
		*ppr_state = nullptr;
	}
}

void CmdBindVertexInputs( R_HW::GfxCommandBuffer commandBuffer, const std::vector<R_HW::VIBinding>& gpuPipelineVIBindings, const GfxModel& gfxModel )
{
	const uint32_t maxVertexInputBinding = 16;
	const uint32_t gpuPipelineVIBingindCount = gpuPipelineVIBindings.size();
	assert( gpuPipelineVIBingindCount <= maxVertexInputBinding );
	R_HW::GfxApiBuffer vertexBuffers[maxVertexInputBinding];
	R_HW::GfxDeviceSize offsets[maxVertexInputBinding];

	for( uint32_t i = 0; i < gpuPipelineVIBingindCount; ++i )
	{
		const GfxModelVertexInput* modelVI = GetVertexInput( gfxModel, ( eVIDataType )gpuPipelineVIBindings[i].desc.dataType );
		assert( gpuPipelineVIBindings[i].desc == modelVI->desc );
		//TODO: assert( modelVI.vertAttribBuffers is valid );
		vertexBuffers[i] = modelVI->buffer.buffer;
		offsets[i] = 0;
	}

	R_HW::CmdBindVertexInputs( commandBuffer, vertexBuffers, 0, gpuPipelineVIBingindCount, offsets );
}

void CmdDrawIndexed( R_HW::GfxCommandBuffer commandBuffer, const std::vector<R_HW::VIBinding>& gpuPipelineVIBindings, const GfxModel& gfxModel, uint32_t indexCount )
{
	CmdBindVertexInputs( commandBuffer, gpuPipelineVIBindings, gfxModel );
	CmdBindIndexBuffer( commandBuffer, gfxModel.indexBuffer.buffer, 0, gfxModel.indexType );
	R_HW::CmdDrawIndexed( commandBuffer, indexCount, 1, 0, 0, 0 );
}

void CmdDrawIndexed( R_HW::GfxCommandBuffer commandBuffer, const std::vector<R_HW::VIBinding>& gpuPipelineVIBindings, const GfxModel& gfxModel )
{
	CmdDrawIndexed( commandBuffer, gpuPipelineVIBindings, gfxModel, gfxModel.indexCount );
}