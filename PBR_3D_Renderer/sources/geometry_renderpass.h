#pragma once

#include "vk_globals.h"

#include "renderpass.h"
#include "material.h"
#include "scene_frame_data.h"

GpuPipelineLayout GetGeoPipelineLayout();
GpuPipelineState GetGeoPipelineState();

void GeometryRecordDrawCommandsBuffer( uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent, const RenderPass * renderpass, const Technique * technique );