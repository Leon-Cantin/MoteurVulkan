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

	void FrameGraph::AddExternalImage( fg_handle_t handle, uint32_t frameIndex, const GfxImage& image )
	{
		imp->_render_targets[handle][frameIndex] = image;
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

	static AttachementDescription CreateRTCommon( GfxFormat format, fg_handle_t render_target_handle, GfxLayout optimalLayout, GfxAccess access )
	{
		AttachementDescription description;
		description.format = format;
		description.access = access;
		description.layout = optimalLayout;
		description.finalAccess = access; //TODO: Just like old Access, this isn't great, We never set this to write, only read.
		description.finalLayout = optimalLayout;
		description.loadOp = GfxLoadOp::DONT_CARE;
		description.oldAccess = GfxAccess::WRITE;//TODO: old access isn't changed anywhere if old access is read we are boned
		description.oldLayout = GfxLayout::UNDEFINED;

		return description;
	}

	static AttachementDescription RenderColor( GfxFormat format, fg_handle_t render_target_handle )
	{
		return CreateRTCommon( format, render_target_handle, GfxLayout::COLOR, GfxAccess::WRITE );
	}

	static AttachementDescription RenderDepth( GfxFormat format, fg_handle_t render_target_handle )
	{
		return CreateRTCommon( format, render_target_handle, GfxLayout::DEPTH_STENCIL, GfxAccess::WRITE );
	}

	static AttachementDescription ReadRenderTargetDepth( GfxFormat format, fg_handle_t render_target_handle )
	{
		return CreateRTCommon( format, render_target_handle, GfxLayout::DEPTH_STENCIL, GfxAccess::READ );
	}

	void ClearTarget( AttachementDescription* attachementDesc )
	{
		attachementDesc->loadOp = GfxLoadOp::CLEAR;
	}

	static int32_t FindResourceIndex( const RenderPassCreationData& pass, fg_handle_t render_target_handle )
	{
		for (int32_t i = 0; i < pass.attachmentCount; ++i)
		{
			if (pass.fgHandleAttachement[i] == render_target_handle )
				return i;
		}

		return -1;
	}

	static AttachementDescription* GetAttachementDesc( RenderPassCreationData* pass, fg_handle_t render_target_handle )
	{
		uint32_t resource_index = FindResourceIndex( *pass, render_target_handle );
		assert( resource_index >= 0 && resource_index < pass->attachmentCount );
		return &pass->descriptions[resource_index];
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
		//TODO: this is wrong, render targets don't include buffers
		o_frameGraph->_render_targets_count = creationData.resources.size();
		std::map< fg_handle_t, RenderPassCreationData*> lastPass;

		for (uint32_t i = 0; i < creationData.renderPasses.size(); ++i)
		{
			RenderPassCreationData& pass = creationData.renderPasses[i];

			for( uint32_t rtIndex = 0; rtIndex < pass.frame_graph_node.renderTargetRefs.size(); ++rtIndex )
			{
				const RenderTargetRef& rtRef = pass.frame_graph_node.renderTargetRefs[rtIndex];
				const fg_handle_t resource_h = rtRef.resourceHandle;
				const GfxFormat format = creationData.resources[rtRef.resourceHandle].resourceDesc.format;

				if( rtRef.flags & FG_RENDERTARGET_REF_READ_BIT )
				{
					// read resources -	Look at read resources and change the the final layout of the last appearence to be used as shader resource or depth read
					auto it_found = lastPass.find( resource_h );
					if( it_found != lastPass.end() )
					{
						RenderPassCreationData* lastPassWithResource = it_found->second;
						int32_t otherPassReferenceIndex = FindResourceIndex( *lastPassWithResource, resource_h );
						if( otherPassReferenceIndex >= 0 )
						{
							//Last pass should transition to read resource at the end
							lastPassWithResource->descriptions[otherPassReferenceIndex].finalLayout = GfxLayout::COLOR;
							lastPassWithResource->descriptions[otherPassReferenceIndex].finalAccess = GfxAccess::READ;
						}
						else
						{
							//We are already in a read state
						}
					}
					else
					{
						throw std::runtime_error( "This render pass does not exist already!" );
					}
				}
				else
				{
					assert( pass.attachmentCount < MAX_ATTACHMENTS_COUNT );

					const uint32_t attachement_index = pass.attachmentCount++;
					pass.fgHandleAttachement[attachement_index] = resource_h;
					AttachementDescription& description = pass.descriptions[attachement_index];

					if( creationData.resources[resource_h].resourceDesc.usage_flags & DEPTH_STENCIL_ATTACHMENT )
					{
						if( rtRef.flags & FG_RENDERTARGET_REF_DEPTH_READ )
							description = ReadRenderTargetDepth( format, rtRef.resourceHandle );
						else
							description = RenderDepth( format, rtRef.resourceHandle );
					}
					else
						description = RenderColor( format, rtRef.resourceHandle );

					if( rtRef.flags & FG_RENDERTARGET_REF_CLEAR_BIT )
						ClearTarget( &description );

					auto it_found = lastPass.find( resource_h );
					if( it_found != lastPass.end() )
					{
						RenderPassCreationData* lastPassWithResource = it_found->second;
						//The last pass will have to transition into this pass' layout
						AttachementDescription* lastAttachementDesc = GetAttachementDesc( lastPassWithResource, resource_h );
						lastAttachementDesc->finalLayout = description.layout;
						if( rtRef.flags & FG_RENDERTARGET_REF_DEPTH_READ )
						{
							lastAttachementDesc->finalAccess = GfxAccess::READ;
							description.oldAccess = GfxAccess::READ;
						}

						//This pass'initial layout should be the same as the layout since the line above will transition it
						description.oldLayout = description.layout;

						//If another pass wrote into this, we shouldn't discard the data
						description.loadOp = GfxLoadOp::LOAD;//TODO: Maybe we shouldn't load if we specified a CLEAR
					}
				}

				lastPass[resource_h] = &pass;
			}
		}
	}

	static void CreateResources( FrameGraphCreationData& creationData, FrameGraphInternal* o_frameGraph )
	{
		o_frameGraph->_gfx_mem_heap = create_gfx_heap( 16 * 1024 * 1024, GFX_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
		GfxHeaps_BatchedAllocator image_allocator( &o_frameGraph->_gfx_mem_heap );
		o_frameGraph->_gfx_mem_heap_host_visible = create_gfx_heap( 8 * 1024 * 1024, GFX_MEMORY_PROPERTY_HOST_VISIBLE_BIT | GFX_MEMORY_PROPERTY_HOST_COHERENT_BIT );
		GfxHeaps_BatchedAllocator buffer_allocator( &o_frameGraph->_gfx_mem_heap_host_visible );

		image_allocator.Prepare();

		for( uint32_t fg_Handle = 0; fg_Handle < creationData.resources.size(); ++fg_Handle )
		{
			if( !(creationData.resources[fg_Handle].flags & eDataEntryFlags::EXTERNAL) )
			{
				if( IsBufferType( creationData.resources[fg_Handle].descriptorType ) )
				{
					for( uint32_t frameIndex = 0; frameIndex < SIMULTANEOUS_FRAMES; ++frameIndex )
						CreateBuffer( creationData.resources[fg_Handle], &buffer_allocator, &o_frameGraph->_buffers[fg_Handle][frameIndex] );
				}
				else if( creationData.resources[fg_Handle].descriptorType == eDescriptorType::SAMPLER )
				{
				}
				else
				{
					ResourceDesc* resourceDesc = &creationData.resources[fg_Handle].resourceDesc;
					o_frameGraph->_render_targets[fg_Handle][0] = CreateImage( resourceDesc->format, resourceDesc->extent, resourceDesc->usage_flags, &image_allocator );
					for( uint32_t frameIndex = 1; frameIndex < SIMULTANEOUS_FRAMES; ++frameIndex )
						o_frameGraph->_render_targets[fg_Handle][frameIndex] = o_frameGraph->_render_targets[fg_Handle][0];
				}
			}
		}

		image_allocator.Commit();
	}

	//TODO: probably doesn't need the frame graph
	static void CreateFrameBuffer( RenderPass* renderpass, const RenderPassCreationData& passCreationData, uint32_t colorCount, bool containsDepth, FrameGraphInternal* frameGraph )
	{
		VkExtent2D extent = frameGraph->_render_targets[passCreationData.fgHandleAttachement[0]][0].extent;
		for( uint32_t frameIndex = 0; frameIndex < SIMULTANEOUS_FRAMES; frameIndex++ )
		{
			GfxImageView colorImages[MAX_ATTACHMENTS_COUNT];
			for( uint32_t colorIndex = 0; colorIndex < colorCount; ++colorIndex )
			{
				const fg_handle_t resourceHandle = passCreationData.fgHandleAttachement[colorIndex];
				colorImages[colorIndex] = frameGraph->_render_targets[resourceHandle][frameIndex].imageView;
			}
			GfxImageView* depthImage = containsDepth ? &frameGraph->_render_targets[passCreationData.fgHandleAttachement[colorCount]][frameIndex].imageView : nullptr;


			renderpass->outputFrameBuffer[frameIndex] = CreateFrameBuffer( colorImages, colorCount, depthImage, extent, *renderpass );
		}
	}

	static void CreateRenderPass(const RenderPassCreationData& passCreationData, const char* name, RenderPass* o_renderPass, FrameGraphInternal* frameGraph)
	{
		assert(passCreationData.attachmentCount > 0);
		bool containsDepth = passCreationData.descriptions[passCreationData.attachmentCount - 1].layout == GfxLayout::DEPTH_STENCIL;
		uint32_t colorCount = passCreationData.attachmentCount - (containsDepth ? 1 : 0);
		const AttachementDescription* ptrDepthStencilAttachement = (containsDepth ? &passCreationData.descriptions[colorCount] : nullptr );

		*o_renderPass = CreateRenderPass( name, passCreationData.descriptions, colorCount, ptrDepthStencilAttachement );

		//Create the frame buffer of the render pass
		CreateFrameBuffer( o_renderPass, passCreationData, colorCount, containsDepth, frameGraph );
	}

	void CreateRenderPasses( FrameGraph* frameGraphExternal )
	{
		FrameGraphInternal* frameGraph = frameGraphExternal->imp;
		const FrameGraphCreationData& creationData = frameGraph->creationData;
		for( uint32_t i = 0; i < creationData.renderPasses.size(); ++i )
		{
			//Create the pass
			const RenderPassCreationData* rpCreationData = &creationData.renderPasses[i];
			CreateRenderPass( *rpCreationData, rpCreationData->name, &frameGraph->_render_passes[frameGraph->_render_passes_count++], frameGraph );
		}
	}

	FrameGraph CreateGraph( std::vector<RenderPassCreationData> *inRpCreationData, std::vector<DataEntry> *inRtCreationData )
	{
		FrameGraphInternal* frameGraphInternal = new FrameGraphInternal();
		FrameGraph frameGraph( frameGraphInternal );
		FrameGraphCreationData& creationData = frameGraphInternal->creationData;

		//Setup resources
		creationData.resources = *inRtCreationData;
		creationData.renderPasses = *inRpCreationData;

		ComposeGraph( creationData, frameGraphInternal );

		CreateResources( creationData, frameGraphInternal );

		return frameGraph;
	}

	void Cleanup( FrameGraph* frameGraphExternal )
	{
		if( !frameGraphExternal->imp )
			return;

		FrameGraphInternal* frameGraph = frameGraphExternal->imp;

		for (uint32_t i = 0; i < frameGraph->_render_targets_count; ++i)
		{
			//TODO: So far only the external stuff is multi buffered
			GfxImage& image = frameGraph->_render_targets[i][0];
			if( image.image && !( frameGraph->creationData.resources[i].flags & eDataEntryFlags::EXTERNAL ) )
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

	void RecordDrawCommands(uint32_t currentFrame, void* userData, GfxCommandBuffer graphicsCommandBuffer, VkExtent2D extent, FrameGraph* frameGraphExternal)
	{
		FrameGraphInternal* frameGraph = frameGraphExternal->imp;
		for (uint32_t i = 0; i < frameGraph->_render_passes_count; ++i)
		{
			TaskInputData taskInputData = { userData, currentFrame, extent, &frameGraph->_render_passes[i], &frameGraph->_techniques[i] };
			frameGraph->creationData.renderPasses[i].frame_graph_node.RecordDrawCommands( graphicsCommandBuffer, taskInputData );
		}
	}
}