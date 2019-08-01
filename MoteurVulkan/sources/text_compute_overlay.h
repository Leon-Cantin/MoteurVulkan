/*#pragma once

#include "vk_globals.h"
#include "swapchain.h"

#include<vector>

void RecreateTextComputeAfterSwapChain(VkDescriptorPool descriptorPool, const Swapchain& swapchain, VkSampler trilinearSampler);
void CleanupTextComputeAfterSwapChain(VkDescriptorPool descriptorPool);
void InitTextCompute(VkDescriptorPool descriptorPool, const Swapchain& swapchain, VkSampler trilinearSampler);
void CleanupTextCompute();

void createTextComputeDescriptorSetLayout();
void CreateTextComputePipeline();
void CreateTextComputeDescriptorSet(VkDescriptorPool descriptorPool, const Swapchain& swapchain, VkSampler trilinearSampler);
void CmdDispatchTextOverlay();
*/