#pragma once

#include "vk_globals.h"
#include "renderpass.h"
#include "scene_frame_data.h"
#include "material.h"

#include "glm/mat4x4.hpp"
#include "swapchain.h"

//TODO the .h doesn't need to know about this struct
//TODO I want to use a mat3 but the mem requirement size is at 48 instead of 36, it ends up broken
// when received by the shader
// https://www.khronos.org/registry/vulkan/specs/1.0-extensions/html/vkspec.html#interfaces-resources  14.5.4
struct SkyboxUniformBufferObject {
	glm::mat4 inv_view_matrix;
};

GpuPipelineLayout GetSkyboxPipelineLayout();
GpuPipelineState GetSkyboxPipelineState();
void SkyboxRecordDrawCommandsBuffer(uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent, const RenderPass * renderpass, const Technique * technique );

void UpdateSkyboxUniformBuffers( GpuBuffer* skyboxUniformBuffer, const glm::mat4& world_view_matrix );