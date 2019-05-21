#pragma once

#include "vk_globals.h"
#include "scene_instance.h"
#include "scene_frame_data.h"

#include "vk_image.h"
#include <vector>

#include <glm/mat4x4.hpp>

struct LightUniform {
	glm::mat4 shadowMatrix;
	glm::vec3 position;
	float intensity;
};

void InitScene();
void InitSkybox(const GfxImage* skyboxImage);

void ReloadSceneShaders();
void UpdateGeometryUniformBuffer(const SceneInstance* sceneInstance, const SceneInstanceSet* sceneInstanceDescriptorSet, uint32_t currentFrame);
void UpdateLightUniformBuffer(const SceneMatricesUniform* shadowSceneMatrices, LightUniform* light, uint32_t currentFrame);
void UpdateSceneUniformBuffer(const glm::mat4& world_view_matrix, VkExtent2D extent, uint32_t currentFrame);
void CleanupScene();
void CreateGeometryInstanceDescriptorSet(SceneInstanceSet* sceneInstanceDescriptorSet);
void CreateGeometryRenderpassDescriptorSet(const GfxImage* albedoImage, const GfxImage* normalImage);
void WaitForFrame(uint32_t currentFrame);
void draw_frame(uint32_t currentFrame, const SceneFrameData* frameData);
