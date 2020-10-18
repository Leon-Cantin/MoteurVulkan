#pragma once

#include "vk_globals.h"
#include "vk_vertex_input.h"
#include "renderpass.h"
#include <vector>

uint32_t GetBindingDescription( const std::vector<VIBinding>& VIBindings, VIState* o_viState );
void CreatePipeline( const GpuPipelineStateDesc& gpuPipelineDesc, const RenderPass& renderPass, GfxPipelineLayout pipelineLayout, VkPipeline* o_pipeline );
