#pragma once

#include "vk_globals.h"
#include "vk_image.h"
#include "renderpass.h"

#include "glm/mat4x4.hpp"
#include "swapchain.h"

void InitializeSkyboxRenderPass(const RenderPass* renderpass, const Swapchain* swapchain);
void RecreateSkyboxAfterSwapchain(const Swapchain* swapchain);
void CleanupSkyboxAfterSwapchain();
void CleanupSkybox();


void createSkyboxUniformBuffers();
void UpdateSkyboxUniformBuffers(size_t currentFrame, const glm::mat4& world_view_matrix);
void CreateSkyboxDescriptorSet(VkDescriptorPool descriptorPool, VkImageView skyboxImageView, VkSampler trilinearSampler);
void CmdDrawSkybox(VkCommandBuffer commandBuffer, VkExtent2D extent, size_t currentFrame);

void ReloadSkyboxShaders(VkExtent2D extent);