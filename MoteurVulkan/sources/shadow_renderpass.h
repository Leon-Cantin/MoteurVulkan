#pragma once

#include "vk_globals.h"
#include "glm/mat4x4.hpp"
#include "model_asset.h"
#include "scene_instance.h"
#include "vk_image.h"

void CreateShadowPass();
void CleanupShadowPass();

void UpdateShadowUniformBuffers(size_t currentFrame, const SceneMatricesUniform* sceneMatrices);
void CmdBeginShadowPass(VkCommandBuffer commandBuffer, size_t currentFrame);
void CmdDrawShadowPass(VkCommandBuffer commandBuffer, const SceneInstanceSet* instanceSet, const ModelAsset* modelAsset, uint32_t currentFrame);
void CmdEndShadowPass(VkCommandBuffer commandBuffer);
void CreateShadowDescriptorSet(VkDescriptorPool descriptorPool, const VkBuffer*instanceUniformBuffer);
void computeShadowMatrix(const glm::vec3& light_location, glm::mat4* view, glm::mat4* projection);

const GfxImage* GetShadowDepthImage();
