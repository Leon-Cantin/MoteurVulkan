#pragma once

#include <vector>
#include <array>
#include "gfx_image.h"

namespace FG
{
	struct FrameGraphCreationData
	{
		std::vector<DataEntry> resources;
		std::vector<RenderPassCreationData> renderPasses;
		fg_handle_t RT_OUTPUT_TARGET;
	};

	class FrameGraphInternal
	{
	public:
		GfxImage _output_buffers[SIMULTANEOUS_FRAMES];
#define MAX_RENDERTARGETS 32
#define MAX_BUFFERS 32

		GfxImage _render_targets[MAX_RENDERTARGETS];
		uint32_t _render_targets_count;

		GpuBuffer _buffers[MAX_BUFFERS][SIMULTANEOUS_FRAMES];
		uint32_t _buffers_count;

		FrameGraphCreationData creationData;

		const GfxImage* GetImageFromId( user_id_t user_id ) const
		{
			for( fg_handle_t handle = 0; handle < creationData.resources.size(); ++handle )
				if( creationData.resources[handle].user_id == user_id )
					return GetImageFromHandle( handle );

			return nullptr;
		}

		const GfxImage* GetImageFromHandle( fg_handle_t handle ) const
		{
			return &_render_targets[handle];
		}

		const GpuBuffer* GetBufferFromId( user_id_t user_id, uint32_t frame ) const
		{
			for( fg_handle_t handle = 0; handle < creationData.resources.size(); ++handle )
				if( creationData.resources[handle].user_id == user_id )
					return GetBufferFromHandle( handle, frame );

			return nullptr;
		}

		const GpuBuffer* GetBufferFromHandle( fg_handle_t handle, uint32_t frame ) const
		{
			return &_buffers[handle][frame];
		}

		std::array<RenderPass, 8> _render_passes;
		uint32_t _render_passes_count = 0;

		std::array<Technique, 8> _techniques;
		uint32_t _techniques_count = 0;

		const RenderPass* GetRenderPass( uint32_t id ) const
		{
			return &_render_passes[id];
		}

		GfxHeap _gfx_mem_heap;
		GfxHeap _gfx_mem_heap_host_visible;

		//hack
		std::array<GfxImageSamplerCombined, 32> allImages;
	};
}
