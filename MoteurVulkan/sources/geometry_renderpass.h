#pragma once

#include "vk_globals.h"

#include "renderpass.h"
#include "model_asset.h"
#include "vk_image.h"
#include "vk_buffer.h"
#include "scene_instance.h"
#include "swapchain.h"
#include "scene_frame_data.h"
#include "material.h"

GpuPipelineLayout GetGeoPipelineLayout();
GpuPipelineState GetGeoPipelineState();

void GeometryRecordDrawCommandsBuffer( uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent, const RenderPass * renderpass, const Technique * technique );
