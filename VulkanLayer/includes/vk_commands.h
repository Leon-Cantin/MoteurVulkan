#pragma once

#include "vk_globals.h"

VkCommandBuffer beginSingleTimeCommands();
void endSingleTimeCommands(VkCommandBuffer commandBuffer);
void CreateSingleUseCommandPool(uint32_t queueFamilyIndex, VkCommandPool* o_commandPool);
void CreateCommandPool(uint32_t queueFamilyIndex, VkCommandPool* o_commandPool);

void BeginCommandBufferRecording(VkCommandBuffer commandBuffer);
void EndCommandBufferRecording(VkCommandBuffer commandBuffer);