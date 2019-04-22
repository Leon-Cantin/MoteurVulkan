#pragma once

#include "vk_globals.h"
#include "vk_image.h"
#include "renderpass.h"

#include "glm/mat4x4.hpp"

void AddSkyboxRenderPass(const RenderPass& skyboxRenderPass);
void create_skybox_graphics_pipeline(VkExtent2D extent);
void createSkyboxUniformBuffers();
void UpdateSkyboxUniformBuffers(size_t currentFrame, const glm::mat4& world_view_matrix);
void CreateSkyboxDescriptorSet(VkDescriptorPool descriptorPool, VkImageView skyboxImageView, VkSampler trilinearSampler);
void createSkyboxDescriptorSetLayout();
void CmdDrawSkybox(VkCommandBuffer commandBuffer, VkExtent2D extent, size_t currentFrame);
void CleanupSkyboxAfterSwapchain();
void CleanupSkybox();
void ReloadSkyboxShaders(VkExtent2D extent);