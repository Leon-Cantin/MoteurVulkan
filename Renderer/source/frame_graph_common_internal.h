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
	};

	class FrameGraphInternal
	{
	public:
#define MAX_RENDERTARGETS 32
#define MAX_BUFFERS 32

		R_HW::GfxImage _render_targets[MAX_RENDERTARGETS][SIMULTANEOUS_FRAMES];
		uint32_t _render_targets_count;

		R_HW::GpuBuffer _buffers[MAX_BUFFERS][SIMULTANEOUS_FRAMES];
		uint32_t _buffers_count;

		FrameGraphCreationData creationData;

		const R_HW::GfxImage* GetImageFromId( user_id_t user_id ) const
		{
			for( fg_handle_t handle = 0; handle < creationData.resources.size(); ++handle )
				if( creationData.resources[handle].user_id == user_id )
					return GetImageFromHandle( handle );

			return nullptr;
		}

		const R_HW::GfxImage* GetImageFromHandle( fg_handle_t handle ) const
		{
			//TODO: temporal stuff
			return &_render_targets[handle][0];
		}

		const R_HW::GpuBuffer* GetBufferFromId( user_id_t user_id, uint32_t frame ) const
		{
			for( fg_handle_t handle = 0; handle < creationData.resources.size(); ++handle )
				if( creationData.resources[handle].user_id == user_id )
					return GetBufferFromHandle( handle, frame );

			return nullptr;
		}

		const R_HW::GpuBuffer* GetBufferFromHandle( fg_handle_t handle, uint32_t frame ) const
		{
			return &_buffers[handle][frame];
		}

		std::array<R_HW::RenderPass, 8> _render_passes;
		uint32_t _render_passes_count = 0;

		std::array<Technique, 8> _techniques;
		uint32_t _techniques_count = 0;

		const R_HW::RenderPass* GetRenderPass( uint32_t id ) const
		{
			return &_render_passes[id];
		}

		R_HW::GfxHeap _gfx_mem_heap;
		R_HW::GfxHeap _gfx_mem_heap_host_visible;

		//hack
		std::array<R_HW::GfxImageSamplerCombined, 32> allImages;
	};
}
