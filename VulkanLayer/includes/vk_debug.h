#pragma once

#include "vk_globals.h"

#include "glm/vec4.hpp"

void MarkVkObject(uint64_t object, VkObjectType objetType, const char * name);
void MarkGfxObject( GfxApiImage image, const char * name );
void MarkGfxObject( GfxImageView imageView, const char * name );
void MarkGfxObject( GfxApiSampler sampler, const char * name );

void CmdBeginLabel(VkCommandBuffer commandBuffer, const char * name, const glm::vec4& color);
void CmdEndLabel(VkCommandBuffer commandBuffer);
void CmdInsertVkLabel(VkCommandBuffer commandBuffer, const char * name, const glm::vec4& color);

void QueueBeginVkLabel(VkQueue queue, const char * name, const glm::vec4& color);
void QueueEndVkLabel(VkQueue queue);
void QueueInsertVkLabel(VkQueue queue, const char * name, const glm::vec4& color);

void SetupDebugCallback();
void DestroyDebugCallback();