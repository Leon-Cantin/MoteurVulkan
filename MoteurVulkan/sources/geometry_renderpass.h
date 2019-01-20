#pragma once

#include "vk_globals.h"

#include "renderpass.h"
#include "model_asset.h"
#include "vk_image.h"
#include "vk_buffer.h"
#include "scene_instance.h"

void createGeoDescriptorSetLayout();
void CleanupGeometryRenderpassAfterSwapchain();
void CleanupGeometryRenderpass();
void createGeoGraphicPipeline(VkExtent2D extent);
void createGeometryRenderPass(VkFormat colorFormat);
void CreateGeometryDescriptorSet(VkDescriptorPool descriptorPool, VkBuffer* sceneUniformBuffers, VkBuffer* instanceUniformBuffers, VkBuffer* lightBuffers, VkImageView textureView,
	VkImageView normalTextureView, VkSampler sampler, VkImageView shadowTextureView, VkSampler shadowSampler);

void CmdBeginGeometryRenderPass(VkCommandBuffer commandBuffer, VkFramebuffer frameBuffer, VkExtent2D extent, uint32_t currentFrame);
void CmdEndGeometryRenderPass(VkCommandBuffer vkCommandBuffer);
void CmdDrawModelAsset(VkCommandBuffer commandBuffer, const SceneInstanceSet* instanceSet, const ModelAsset& modelAsset, uint32_t currentFrame);

VkRenderPass GetGeometryRenderPass();
void ReloadGeometryShaders(VkExtent2D extent);