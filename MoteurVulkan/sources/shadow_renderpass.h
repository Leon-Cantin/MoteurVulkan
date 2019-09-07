#pragma once

#include "vk_globals.h"
#include "glm/mat4x4.hpp"
#include "model_asset.h"
#include "scene_instance.h"
#include "renderpass.h"
#include "swapchain.h"
#include "scene_frame_data.h"
#include "material.h"

GpuPipelineLayout GetShadowPipelineLayout();
GpuPipelineState GetShadowPipelineState();
void ShadowRecordDrawCommandsBuffer( uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent, const RenderPass * renderpass, const Technique * technique );

void UpdateShadowUniformBuffers( GpuBuffer* shadowSceneUniformBuffer, const SceneMatricesUniform* sceneUniforms );
void computeShadowMatrix( const glm::vec3& light_location, glm::mat4* view, glm::mat4* projection );
