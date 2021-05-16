#pragma once

#include "vk_globals.h"
#include "scene_frame_data.h"
#include "material.h"
#include "frame_graph.h"

GpuPipelineLayout GetSkyboxPipelineLayout();
GpuPipelineStateDesc GetSkyboxPipelineState();
void SkyboxRecordDrawCommandsBuffer( GfxCommandBuffer graphicsCommandBuffer, const FG::TaskInputData& inputData );

#include "glm/mat4x4.hpp"
//TODO the .h doesn't need to know about this struct
//TODO I want to use a mat3 but the mem requirement size is at 48 instead of 36, it ends up broken
// when received by the shader
// https://www.khronos.org/registry/vulkan/specs/1.0-extensions/html/vkspec.html#interfaces-resources  14.5.4
struct SkyboxUniformBufferObject {
	glm::mat4 inv_view_matrix;
};

void UpdateSkyboxUniformBuffers( GpuBuffer* skyboxUniformBuffer, const glm::mat4& world_view_matrix );