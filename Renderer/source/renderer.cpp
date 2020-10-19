#include "renderer.h"

#include "vk_buffer.h"
#include "vk_debug.h"
#include "vk_commands.h"
#include "profile.h"
#include "vk_framework.h"
#include "gpu_synchronization.h"
#include "frame_graph.h"
#include "gfx_heaps_batched_allocator.h"

#include <array>
#include <iostream>

Swapchain g_swapchain;
std::array<VkCommandBuffer, SIMULTANEOUS_FRAMES> g_graphicsCommandBuffers;

std::array<GfxSemaphore, SIMULTANEOUS_FRAMES> graphicPassFinishedSemaphores;
std::array<GfxSemaphore, SIMULTANEOUS_FRAMES> imageAvailableSemaphores;
std::array<GfxSemaphore, SIMULTANEOUS_FRAMES> renderFinishedSemaphores;
std::array<GfxFence, SIMULTANEOUS_FRAMES> inFlightFences;

FG::FrameGraph( *_fFGScriptInitialize )(const Swapchain*);

FG::FrameGraph _frameGraph;

bool( *_needResize )();
void( *_getFrameBufferSize )(uint64_t* width, uint64_t* height);

VkSurfaceKHR _swapchainSurface;

GfxImage dummyImage;

static void CreateCommandBuffer()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = g_vk.graphicsCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(g_graphicsCommandBuffers.size());

	if (vkAllocateCommandBuffers(g_vk.device.device, &allocInfo, g_graphicsCommandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

static void create_sync_objects()
{
	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
	{
		if (!CreateGfxSemaphore( &imageAvailableSemaphores[i]) ||
			!CreateGfxSemaphore( &renderFinishedSemaphores[i]) ||
			!CreateGfxSemaphore( &graphicPassFinishedSemaphores[i]) ||
			!CreateGfxFence( &inFlightFences[i]) )
		{
			throw std::runtime_error("failed to create semaphores!");
		}

		MarkGfxObject(imageAvailableSemaphores[i], "image available semaphore");
		MarkGfxObject(renderFinishedSemaphores[i], "render finished semaphore");
		MarkGfxObject(graphicPassFinishedSemaphores[i], "graphic pass finished semaphore");
		MarkGfxObject(inFlightFences[i], "In flight fence");
	}
}

static void cleanup_swap_chain()
{
	FG::Cleanup( &_frameGraph );

	Destroy( &g_swapchain );
}

static void CreateDummyImage()
{
	GfxHeaps_CommitedResourceAllocator allocator = {};
	allocator.Prepare();
	CreateSolidColorImage( glm::vec4( 0, 0, 0, 0 ), &dummyImage, &allocator );
	allocator.Commit();
}

void InitRenderer( VkSurfaceKHR swapchainSurface, bool( *needResize )(), void( *getFrameBufferSize )(uint64_t* width, uint64_t* height) )
{
	//TODO: do better than this shit
	_getFrameBufferSize = getFrameBufferSize;
	_needResize = needResize;

	uint64_t width, height;
	_getFrameBufferSize( &width, &height );
	_swapchainSurface = swapchainSurface;
	CreateSwapChain( swapchainSurface, width, height, g_swapchain );

	CreateCommandPool( g_vk.device.graphics_queue.queueFamilyIndex, &g_vk.graphicsCommandPool );
	CreateSingleUseCommandPool( g_vk.device.graphics_queue.queueFamilyIndex, &g_vk.graphicsSingleUseCommandPool );
	CreateCommandBuffer();

	InitSamplers();

	create_sync_objects();

	CreateTimeStampsQueryPool( SIMULTANEOUS_FRAMES );

	CreateDummyImage();
}

static void CompileFrameGraph()
{
	_frameGraph = _fFGScriptInitialize( &g_swapchain );
}

void recreate_swap_chain( VkSurfaceKHR swapchainSurface )
{
	//TODO: find a better way of handling window minimization
	uint64_t width = 0, height = 0;
	while (width == 0 || height == 0) {
		_getFrameBufferSize( &width, &height );
		//TODO
		//glfwWaitEvents();
	}

	DeviceWaitIdle( g_vk.device.device );

	cleanup_swap_chain();

	//TODO: try to use the "oldSwapchain" parameter to optimize when recreating swap chains
	_getFrameBufferSize(&width, &height);
	CreateSwapChain( swapchainSurface, width, height, g_swapchain );

	CompileFrameGraph();
}

void CleanupFrameGraph()
{
	FG::Cleanup( &_frameGraph );
}

void CompileFrameGraph( FG::FrameGraph( *FGScriptInitialize )( const Swapchain* ) )
{
	_fFGScriptInitialize = FGScriptInitialize;
	CompileFrameGraph();
}

void RecordCommandBuffer(uint32_t currentFrame, const SceneFrameData* frameData)
{
	VkCommandBuffer graphicsCommandBuffer = g_graphicsCommandBuffers[currentFrame];
	BeginCommandBufferRecording(graphicsCommandBuffer);

	CmdResetTimeStampSet(graphicsCommandBuffer, currentFrame);

	CmdWriteTimestamp(graphicsCommandBuffer, GFX_PIPELINE_STAGE_TOP_OF_PIPE_BIT, Timestamp::COMMAND_BUFFER_START, currentFrame);

	FG::RecordDrawCommands(currentFrame, frameData, graphicsCommandBuffer, g_swapchain.extent, &_frameGraph );

	CmdWriteTimestamp(graphicsCommandBuffer, GFX_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, Timestamp::COMMAND_BUFFER_END, currentFrame);

	EndCommandBufferRecording(graphicsCommandBuffer);
}

void WaitForFrame(uint32_t currentFrame)
{
	WaitForFence( &inFlightFences[currentFrame], 1 );
}

void draw_frame(uint32_t currentFrame, const SceneFrameData* frameData)
{
	RecordCommandBuffer(currentFrame, frameData);

	GfxSwapchainImage swapchainImage;
	const GfxSwapchainOperationResult aquireSwapChainImageResult = AcquireNextSwapchainImage( g_swapchain.swapchain, imageAvailableSemaphores[currentFrame], &swapchainImage );
	//TODO: Do I really need 2 check for recreate swap chain in this function?
	if ( !SwapchainImageIsValid( aquireSwapChainImageResult ) || _needResize() ) {
		unsignalSemaphore( imageAvailableSemaphores[currentFrame] );
		recreate_swap_chain( _swapchainSurface );
		return;
	}

	//Submit work
	GfxSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	GfxPipelineStageFlag waitStages[] = { GFX_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	GfxSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] }; //TODO make a system that keeps the semaphores ordered

	ResetGfxFences( &inFlightFences[currentFrame], 1);

	if( !QueueSubmit( g_vk.device.graphics_queue.queue, &g_graphicsCommandBuffers[currentFrame], 1, waitSemaphores, waitStages, 1, signalSemaphores, 1, inFlightFences[currentFrame] ) )
		throw std::runtime_error( "failed to submit draw command buffer!" );

	//Present
	const GfxSwapchainOperationResult presentResult = QueuePresent( g_vk.device.present_queue.queue, swapchainImage, &renderFinishedSemaphores[currentFrame], 1 );
	if (!SwapchainImageIsValid( presentResult ))
	{
		std::cout << "recreating swap chain " << currentFrame << std::endl;
		recreate_swap_chain( _swapchainSurface );
	}
}

void CleanupRenderer() {
	DestroyImage( &dummyImage );

	vkFreeCommandBuffers(g_vk.device.device, g_vk.graphicsCommandPool, static_cast<uint32_t>(g_graphicsCommandBuffers.size()), g_graphicsCommandBuffers.data());

	cleanup_swap_chain();

	DestroySamplers();

	FG::Cleanup( &_frameGraph );

	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
	{
		DestroyGfxSemaphore( &renderFinishedSemaphores[i] );
		DestroyGfxSemaphore( &imageAvailableSemaphores[i] );
		DestroyGfxSemaphore( &graphicPassFinishedSemaphores[i] );
		DestroyGfxFence( &inFlightFences[i] );
	}

	Destroy( &g_vk.graphicsCommandPool );
	Destroy( &g_vk.graphicsSingleUseCommandPool );
	Destroy( &g_vk.computeCommandPool );
	Destroy( &g_vk.transferCommandPool );

	DestroyTimeStampsPool();
}

void CmdBindVertexInputs( VkCommandBuffer commandBuffer, const std::vector<VIBinding>& gpuPipelineVIBindings, const GfxModel& gfxModel )
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
		//assert( modelVI.vertAttribBuffers != VK_NULL_HANDLE );
		vertexBuffers[i] = modelVI->buffer.buffer;
		offsets[i] = 0;
	}

	CmdBindVertexInputs( commandBuffer, vertexBuffers, 0, gpuPipelineVIBingindCount, offsets );
}

void CmdDrawIndexed( VkCommandBuffer commandBuffer, const std::vector<VIBinding>& gpuPipelineVIBindings, const GfxModel& gfxModel, uint32_t indexCount )
{
	CmdBindVertexInputs( commandBuffer, gpuPipelineVIBindings, gfxModel );
	CmdBindIndexBuffer( commandBuffer, gfxModel.indexBuffer.buffer, 0, gfxModel.indexType );
	CmdDrawIndexed( commandBuffer, indexCount, 1, 0, 0, 0 );
}

void CmdDrawIndexed( VkCommandBuffer commandBuffer, const std::vector<VIBinding>& gpuPipelineVIBindings, const GfxModel& gfxModel )
{
	CmdDrawIndexed( commandBuffer, gpuPipelineVIBindings, gfxModel, gfxModel.indexCount );
}
