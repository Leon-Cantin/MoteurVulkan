#pragma once

#include <vector>
#include <array>
#include "gfx_image.h"

namespace FG
{
	struct FrameGraphCreationData
	{
		std::vector<TechniqueDataEntry> resources;
		std::vector<RenderPassCreationData> renderPasses;
		uint32_t RT_OUTPUT_TARGET;
	};

	class FrameGraphInternal
	{
	public:
		GfxImage _output_buffers[SIMULTANEOUS_FRAMES];
#define MAX_RENDERTARGETS 32

		GfxImage _render_targets[MAX_RENDERTARGETS];
		uint32_t _render_targets_count;

		const GfxImage* GetImage( uint32_t render_target_id ) const
		{
			return &_render_targets[render_target_id];
		}

		std::array<RenderPass, 8> _render_passes;
		uint32_t _render_passes_count = 0;

		std::array<Technique, 8> _techniques;
		uint32_t _techniques_count = 0;

		const RenderPass* GetRenderPass( uint32_t id ) const
		{
			return &_render_passes[id];
		}

		FrameGraphCreationData creationData;

		//hack
		std::array<PerFrameBuffer, 32> allbuffers;
		std::array<VkDescriptorImageInfo, 32> allImages;
	};
}
