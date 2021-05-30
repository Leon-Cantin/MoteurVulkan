#pragma once

#include "vk_globals.h"

#include "scene_frame_data.h"
#include "scene_instance.h"
#include "material.h"
#include "frame_graph.h"

R_HW::GpuPipelineLayout GetShadowPipelineLayout();
R_HW::GpuPipelineStateDesc GetShadowPipelineState();

void ShadowRecordDrawCommandsBuffer( R_HW::GfxCommandBuffer graphicsCommandBuffer, const FG::TaskInputData& inputData );

#include "glm/mat4x4.hpp"

void UpdateShadowUniformBuffers( R_HW::GpuBuffer* shadowSceneUniformBuffer, const SceneMatricesUniform* sceneUniforms );
void computeShadowMatrix( const glm::vec3& light_location, glm::mat4* view, glm::mat4* projection );
