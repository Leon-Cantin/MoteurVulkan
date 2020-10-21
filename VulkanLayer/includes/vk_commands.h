#pragma once

#include "vk_globals.h"

GfxCommandBuffer beginSingleTimeCommands();
void endSingleTimeCommands(GfxCommandBuffer commandBuffer);
void CreateSingleUseCommandPool(uint32_t queueFamilyIndex, GfxCommandPool* o_commandPool);
void CreateCommandPool(uint32_t queueFamilyIndex, GfxCommandPool* o_commandPool);
void Destroy( GfxCommandPool* commandPool );

void BeginCommandBufferRecording(GfxCommandBuffer commandBuffer);
void EndCommandBufferRecording(GfxCommandBuffer commandBuffer);