#include "frame_graph.h"
#include "frame_graph_common_internal.h"

#include "gfx_heaps_batched_allocator.h"

#include <vector>
#include <map>
#include <stdexcept>

namespace FG
{
	const RenderPass* FrameGraph::GetRenderPass( uint32_t id )
	{
		return imp->GetRenderPass( id );
	}

	const GfxImage* FrameGraph::GetImageFromId( user_id_t render_target_id )
	{
		return imp->GetImageFromId( render_target_id );
	}

	FrameGraph::FrameGraph()
		: imp( nullptr ) {}

	FrameGraph::FrameGraph( FrameGraphInternal* imp )
		: imp( imp ) {}

	//TODO could be generalized in gfxImage
	static GfxImage CreateImage( GfxFormat format, VkExtent2D extent, GfxImageUsageFlags usage_flags, I_ImageAlloctor* allocator )
	{
		//TODO: we aren't getting any error for not transitionning to the image_layout? is it done in the renderpass?
		GfxImage image = CreateImage( extent.width, extent.height, 1, format, usage_flags );

		allocator->Allocate( image.image, &image.gfx_mem_alloc );

		image.imageView = CreateImageView( image );

		return image;
	}

	static void CreateRTCommon(RenderPassCreationData& resource, GfxFormat format, fg_handle_t render_target_handle, GfxLayout optimalLayout)
	{
		assert(resource.attachmentCount < MAX_ATTACHMENTS_COUNT);

		const uint32_t attachement_index = resource.attachmentCount++;
		AttachementDescription& description = resource.descriptions[attachement_index];
		description.format = format;
		description.access = GfxAccess::WRITE;
		description.layout = optimalLayout;
		description.finalAccess = GfxAccess::WRITE;
		description.finalLayout = optimalLayout;
		description.loadOp = GfxLoadOp::DONT_CARE;
		description.oldAccess = GfxAccess::WRITE;//TODO: old access isn't changed anywhere if old access is read we are boned
		description.oldLayout = GfxLayout::UNDEFINED;

		resource.fgHandleAttachement[attachement_index] = render_target_handle;
	}

	void RenderColor(RenderPassCreationData& resource, GfxFormat format, fg_handle_t render_target_handle )
	{
		CreateRTCommon(resource, format, render_target_handle, GfxLayout::COLOR );
	}

	void RenderDepth(RenderPassCreationData& resource, GfxFormat format, fg_handle_t render_target_handle )
	{
		CreateRTCommon(resource, format, render_target_handle, GfxLayout::DEPTH_STENCIL );
	}

	void ClearTarget(RenderPassCreationData& resource )
	{
		assert(resource.attachmentCount > 0);

		resource.descriptions[resource.attachmentCount-1].loadOp = GfxLoadOp::CLEAR;
	}

	void ReadResource(RenderPassCreationData& resource, fg_handle_t render_target_handle )
	{
		assert(resource.read_targets_count < MAX_READ_TARGETS);
		resource.read_targets[resource.read_targets_count++] = render_target_handle;
		/*
		const uint32_t attachement_id = resource.attachmentCount++;
		resource.read_targets[attachement_id] = render_target;
		resource.e_render_targets[attachement_id] = render_target;*/
	}

	static int32_t FindResourceIndex(const RenderPassCreationData& pass, fg_handle_t render_target_handle )
	{
		for (int32_t i = 0; i < pass.attachmentCount; ++i)
		{
			if (pass.fgHandleAttachement[i] == render_target_handle )
				return i;
		}

		return -1;
	}

	static void CreateBuffer( const FG::DataEntry& techniqueDataEntry, I_BufferAllocator* bufferAllocator, GpuBuffer* o_buffer )
	{
		GfxDeviceSize size;
		switch( techniqueDataEntry.descriptorType )
		{
		case eDescriptorType::BUFFER:
			size = techniqueDataEntry.resourceDesc.extent.width;
			break;
		case eDescriptorType::BUFFER_DYNAMIC:
			size = techniqueDataEntry.resourceDesc.extent.width * techniqueDataEntry.resourceDesc.extent.height;
		}

		//TODO: could have to change VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT if we write (store). Will have to check all bindings to know.
		o_buffer->buffer = create_buffer( size, GFX_BUFFER_USAGE_UNIFORM_BUFFER_BIT );
		bufferAllocator->Allocate( o_buffer->buffer, &o_buffer->gpuMemory );
	}

	static void ComposeGraph( FrameGraphCreationData& creationData, FrameGraphInternal* o_frameGraph )
	{
		o_frameGraph->_gfx_mem_heap = create_gfx_heap( 8 * 1024 * 1024, GFX_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
		GfxHeaps_BatchedAllocator image_allocator( &o_frameGraph->_gfx_mem_heap );
		o_frameGraph->_gfx_mem_heap_host_visible = create_gfx_heap( 8 * 1024 * 1024, GFX_MEMORY_PROPERTY_HOST_VISIBLE_BIT | GFX_MEMORY_PROPERTY_HOST_COHERENT_BIT );
		GfxHeaps_BatchedAllocator buffer_allocator( &o_frameGraph->_gfx_mem_heap_host_visible );

		image_allocator.Prepare();

		//TODO: only creating buffers here, can do better. Images and buffers should get the same treatment
		for( uint32_t fg_Handle = 0; fg_Handle < creationData.resources.size(); ++fg_Handle )
		{
			if( IsBufferType( creationData.resources[fg_Handle].descriptorType ) )
			{
				for( uint32_t frameIndex = 0; frameIndex < SIMULTANEOUS_FRAMES; ++frameIndex )
					CreateBuffer( creationData.resources[fg_Handle], &buffer_allocator, &o_frameGraph->_buffers[fg_Handle][frameIndex] );
			}
		}

		o_frameGraph->_render_targets_count = creationData.resources.size();
		std::map< fg_handle_t, RenderPassCreationData*> lastPass;

		for (uint32_t i = 0; i < creationData.renderPasses.size(); ++i)
		{
			RenderPassCreationData& pass = creationData.renderPasses[i];

			//RenderTargets
			for (uint32_t resource_index = 0; resource_index < pass.attachmentCount; ++resource_index)
			{
				fg_handle_t renderTargetFgHandle= pass.fgHandleAttachement[resource_index];
				AttachementDescription& description = pass.descriptions[resource_index];

				auto it_found = lastPass.find( renderTargetFgHandle );
				if (it_found == lastPass.end())
				{
					lastPass[renderTargetFgHandle] = &pass;

					//Create a new image when we encounter it for the first time.
					ResourceDesc* resourceDesc = &creationData.resources[renderTargetFgHandle].resourceDesc;
					if ( renderTargetFgHandle != creationData.RT_OUTPUT_TARGET)
					{
						o_frameGraph->_render_targets[renderTargetFgHandle] = CreateImage( resourceDesc->format, resourceDesc->extent, resourceDesc->usage_flags, &image_allocator);
					}
				}
				else
				{
					RenderPassCreationData* lastPassWithResource = it_found->second;
					int32_t otherPassReferenceIndex = FindResourceIndex(*lastPassWithResource, renderTargetFgHandle );
					//The last pass will have to transition into this pass' layout
					lastPassWithResource->descriptions[otherPassReferenceIndex].finalLayout = description.layout;
					//This pass' initial layout should be this one's layout
					description.oldLayout = description.layout;
					//We should load since the other pass wrote into this
					description.loadOp = GfxLoadOp::LOAD;//TODO: Maybe we shouldn't load if we specified a CLEAR

					lastPass[renderTargetFgHandle] = &pass;
				}
			}

			/*
			read resources
			Look at read resources and change the the final layout of the last appearence to be used
			as shader resource
			*/
			for (uint32_t resource_index = 0; resource_index < pass.read_targets_count; ++resource_index)
			{
				uint32_t render_target = pass.read_targets[resource_index];
				auto it_found = lastPass.find(render_target);
				if (it_found != lastPass.end())
				{
					RenderPassCreationData* lastPassWithResource = it_found->second;
					int32_t otherPassReferenceIndex = FindResourceIndex(*lastPassWithResource, render_target);
					if (otherPassReferenceIndex >= 0)
					{
						//Last pass should transition to read resource at the end
						lastPassWithResource->descriptions[otherPassReferenceIndex].finalLayout = GfxLayout::COLOR;
						lastPassWithResource->descriptions[otherPassReferenceIndex].finalAccess = GfxAccess::READ;
					}
					else
					{
						//We are already in a read state
					}
					lastPass[render_target] = &pass;
				}
				else
				{
					throw std::runtime_error("This render pass does not exist already!");
				}
			}
		}

		image_allocator.Commit();

		//Transition the last pass with RT_OUTPUT_TARGET to present
		auto it_found = lastPass.find( creationData.RT_OUTPUT_TARGET );
		RenderPassCreationData* lastPassWithResource = it_found->second;
		int32_t otherPassReferenceIndex = FindResourceIndex(*lastPassWithResource, creationData.RT_OUTPUT_TARGET );
		lastPassWithResource->descriptions[otherPassReferenceIndex].finalLayout = GfxLayout::PRESENT;
	}

	//TODO: probably doesn't need the frame graph
	static void CreateFrameBuffer( RenderPass* renderpass, const RenderPassCreationData& passCreationData, uint32_t outputTargetIndex, uint32_t colorCount, bool containsDepth, FrameGraphInternal* frameGraph )
	{
		int32_t outputBufferIndex = FindResourceIndex( passCreationData, outputTargetIndex );
		VkExtent2D extent = frameGraph->_render_targets[passCreationData.fgHandleAttachement[0]].extent;
		GfxImageView colorImages[MAX_ATTACHMENTS_COUNT];
		for (uint32_t i = 0; i < colorCount; ++i)
			colorImages[i] = frameGraph->_render_targets[passCreationData.fgHandleAttachement[i]].imageView;
		GfxImageView* depthImage = containsDepth ? &frameGraph->_render_targets[passCreationData.fgHandleAttachement[colorCount]].imageView : nullptr;

		for( uint32_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
		{
			if( outputBufferIndex >= 0 ) //If this pass uses( writes? ) to the backbuffer could probably be generalized to passes that can output to multiple targets (temporal stuff)
				colorImages[outputBufferIndex] = frameGraph->_output_buffers[i].imageView;
			renderpass->outputFrameBuffer[i] = CreateFrameBuffer( colorImages, colorCount, depthImage, extent, *renderpass );
		}
	}

	static void CreateRenderPass(const RenderPassCreationData& passCreationData, uint32_t outputBufferIndex, const char* name, RenderPass* o_renderPass, FrameGraphInternal* frameGraph)
	{
		assert(passCreationData.attachmentCount > 0);
		bool containsDepth = passCreationData.descriptions[passCreationData.attachmentCount - 1].layout == GfxLayout::DEPTH_STENCIL;
		uint32_t colorCount = passCreationData.attachmentCount - (containsDepth ? 1 : 0);
		const AttachementDescription* ptrDepthStencilAttachement = (containsDepth ? &passCreationData.descriptions[colorCount] : nullptr );

		*o_renderPass = CreateRenderPass( name, passCreationData.descriptions, colorCount, ptrDepthStencilAttachement );

		//Create the frame buffer of the render pass
		CreateFrameBuffer( o_renderPass, passCreationData, outputBufferIndex, colorCount, containsDepth, frameGraph );
	}

	static void CreateResourceCreationData( const Swapchain* swapchain, FrameGraphCreationData* creationData, FrameGraphInternal* o_frameGraph )
	{
		for( uint32_t i = 0; i < creationData->renderPasses.size(); ++i )
		{
			RenderPassCreationData& rpCreationData = creationData->renderPasses[i];
			for( uint32_t rtIndex = 0; rtIndex < rpCreationData.frame_graph_node.renderTargetRefs.size(); ++rtIndex )
			{
				RenderTargetRef& rtRef = rpCreationData.frame_graph_node.renderTargetRefs[rtIndex];
				GfxFormat format = creationData->resources[rtRef.resourceHandle].resourceDesc.format;

				if( rtRef.flags & FG_RENDERTARGET_REF_READ_BIT )
				{
					ReadResource( rpCreationData, rtRef.resourceHandle );
				}
				else
				{
					if( creationData->resources[rtRef.resourceHandle].resourceDesc.usage_flags & DEPTH_STENCIL_ATTACHMENT )
						RenderDepth( rpCreationData, format, rtRef.resourceHandle );
					else
						RenderColor( rpCreationData, format, rtRef.resourceHandle );

					if( rtRef.flags & FG_RENDERTARGET_REF_CLEAR_BIT )
						ClearTarget( rpCreationData );
				}
			}
		}

		//setup hacks for outside resources
		o_frameGraph->_render_targets[creationData->RT_OUTPUT_TARGET] = swapchain->images[0];
		for (uint32_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
			o_frameGraph->_output_buffers[i] = swapchain->images[i];

		for (uint32_t i = 0; i < o_frameGraph->_render_targets_count; ++i)
		{
			if ( creationData->resources[i].resourceDesc.swapChainSized)
				creationData->resources[i].resourceDesc.extent = swapchain->extent;
		}
	}

	FrameGraph CreateGraph(const Swapchain* swapchain, std::vector<RenderPassCreationData> *inRpCreationData, std::vector<DataEntry> *inRtCreationData, fg_handle_t backbuffer_fg_handle )
	{
		FrameGraphInternal* frameGraph = new FrameGraphInternal();
		FrameGraph frameGraphExternal( frameGraph );
		FrameGraphCreationData& creationData = frameGraph->creationData;
		//Setup resources
		creationData.RT_OUTPUT_TARGET = backbuffer_fg_handle;
		creationData.resources = *inRtCreationData;
		creationData.renderPasses = *inRpCreationData;

		CreateResourceCreationData( swapchain, &creationData, frameGraph );

		ComposeGraph( creationData, frameGraph );

		for (uint32_t i = 0; i < creationData.renderPasses.size(); ++i)
		{
			//Create the pass
			RenderPassCreationData* rpCreationData = &creationData.renderPasses[i];
			CreateRenderPass( *rpCreationData, creationData.RT_OUTPUT_TARGET, rpCreationData->name, &frameGraph->_render_passes[frameGraph->_render_passes_count++], frameGraph );
		}

		return frameGraphExternal;
	}

	void Cleanup( FrameGraph* frameGraphExternal )
	{
		if( !frameGraphExternal->imp )
			return;

		FrameGraphInternal* frameGraph = frameGraphExternal->imp;

		for (uint32_t i = 0; i < frameGraph->_render_targets_count; ++i)
		{
			GfxImage& image = frameGraph->_render_targets[i];
			if( image.image && i != frameGraph->creationData.RT_OUTPUT_TARGET )
				DestroyImage( &image );
		}
		frameGraph->_render_targets_count = 0;
		for( uint32_t i = 0; i < frameGraph->creationData.resources.size(); ++i )
		{
			for( uint32_t frameIndex = 0; frameIndex < SIMULTANEOUS_FRAMES; ++frameIndex )
			{
				if( IsValid( frameGraph->_buffers[i][frameIndex] ) )
					Destroy( &frameGraph->_buffers[i][frameIndex] );
			}
		}

		for (uint32_t i = 0; i < frameGraph->_render_passes_count; ++i)
		{
			RenderPass& renderpass = frameGraph->_render_passes[i];
			for (uint32_t fb_index = 0; fb_index < SIMULTANEOUS_FRAMES; ++fb_index)
			{
				Destroy( &renderpass.outputFrameBuffer[fb_index] );
			}
			Destroy( &renderpass );
		}
		frameGraph->_render_passes_count = 0;

		//Cleanup technique
		for( uint32_t i = 0; i < frameGraph->_techniques_count; ++i )
		{
			Technique& technique = frameGraph->_techniques[i];
			Destroy( &technique );
			technique = {};
		}
		frameGraph->_techniques_count = 0;

		frameGraph->allImages = {};

		destroy( &frameGraph->_gfx_mem_heap );
		destroy( &frameGraph->_gfx_mem_heap_host_visible );

		delete frameGraph;
		frameGraphExternal->imp = nullptr;
	}

	void RecordDrawCommands(uint32_t currentFrame, const SceneFrameData* frameData, GfxCommandBuffer graphicsCommandBuffer, VkExtent2D extent, FrameGraph* frameGraphExternal)
	{
		FrameGraphInternal* frameGraph = frameGraphExternal->imp;
		for (uint32_t i = 0; i < frameGraph->_render_passes_count; ++i)
		{
			//TODO: _render_passes[i] _techniques[i] doesn't mean is the right one
			frameGraph->creationData.renderPasses[i].frame_graph_node.RecordDrawCommands(currentFrame, frameData, graphicsCommandBuffer, extent, &frameGraph->_render_passes[i], &frameGraph->_techniques[i]);
		}
	}
}