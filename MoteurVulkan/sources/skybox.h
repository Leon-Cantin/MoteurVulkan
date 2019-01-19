#pragma once

#include "vk_globals.h"
#include "vk_image.h"
#include "renderpass.h"

#include "glm/mat4x4.hpp"

void create_skybox_graphics_pipeline(VkExtent2D extent);
void create_skybox_render_pass(VkFormat colorFormat);
void createSkyboxUniformBuffers();
void UpdateSkyboxUniformBuffers(size_t currentFrame, const glm::mat4& world_view_matrix);
void CreateSkyboxDescriptorSet(VkDescriptorPool descriptorPool, VkImageView skyboxImageView, VkSampler trilinearSampler);
void createSkyboxDescriptorSetLayout();
void CmdDrawSkybox(VkCommandBuffer commandBuffer, VkFramebuffer frameBuffer, VkExtent2D extent, size_t currentFrame);
void CleanupSkyboxAfterSwapchain();
void CleanupSkybox();
void ReloadSkyboxShaders(VkExtent2D extent);