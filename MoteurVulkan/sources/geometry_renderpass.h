#pragma once

#include "vk_globals.h"

#include "renderpass.h"
#include "model_asset.h"
#include "vk_image.h"
#include "vk_buffer.h"
#include "scene_instance.h"
#include "swapchain.h"
#include "scene_frame_data.h"

void InitializeGeometryRenderPass(const RenderPass* renderpass, const Swapchain* swapchain);
void CleanupGeometryRenderpassAfterSwapchain();
void CleanupGeometryRenderpass();
void RecreateGeometryAfterSwapChain( const Swapchain *swapchain);
void GeometryRecordDrawCommandsBuffer(uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent);

void CreateGeometryDescriptorSet(VkDescriptorPool descriptorPool, VkBuffer* sceneUniformBuffers, VkBuffer* instanceUniformBuffers, VkBuffer* lightBuffers, VkImageView textureView,
	VkImageView normalTextureView, VkSampler sampler, VkImageView shadowTextureView, VkSampler shadowSampler);

void CmdBeginGeometryRenderPass(VkCommandBuffer commandBuffer, VkExtent2D extent, uint32_t currentFrame);
void CmdEndGeometryRenderPass(VkCommandBuffer vkCommandBuffer);

void ReloadGeometryShaders(VkExtent2D extent);