#pragma once

#include "vk_globals.h"
#include "scene_instance.h"
#include "vk_image.h"

struct LightUniform {
	glm::mat4 shadowMatrix;
	glm::vec3 position;
	float intensity;
};
void InitSkybox(const GfxImage* skyboxImage);
SceneInstanceSet*  CreateGeometryInstanceDescriptorSet();
void CreateGeometryRenderpassDescriptorSet(const GfxImage* albedoImage, const GfxImage* normalImage);
void UpdateGeometryUniformBuffer(const SceneInstance* sceneInstance, const SceneInstanceSet* sceneInstanceDescriptorSet, uint32_t currentFrame);
void UpdateLightUniformBuffer(const SceneMatricesUniform* shadowSceneMatrices, LightUniform* light, uint32_t currentFrame);
void UpdateSceneUniformBuffer(const glm::mat4& world_view_matrix, VkExtent2D extent, uint32_t currentFrame);
void DrawFrame( uint32_t currentFrame, const SceneInstance* cameraSceneInstance, LightUniform* light, const std::vector<std::pair<const SceneInstance*, const SceneRenderableAsset*>>& drawList );
void ForceReloadShaders();

void ReloadSceneShaders();

void InitRendererImp();
void CleanupRendererImp();