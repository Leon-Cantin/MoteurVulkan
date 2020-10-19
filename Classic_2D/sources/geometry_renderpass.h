#pragma once

#include "vk_globals.h"

#include "renderpass.h"
#include "material.h"
#include "scene_instance.h"//TODO: only there because scene_frame_data needs to have a definition for GfxAsset
#include "scene_frame_data.h"

GpuPipelineLayout GetGeoPipelineLayout();
GpuPipelineStateDesc GetGeoPipelineState();

void GeometryRecordDrawCommandsBuffer( uint32_t currentFrame, const SceneFrameData* frameData, GfxCommandBuffer graphicsCommandBuffer, VkExtent2D extent, const RenderPass * renderpass, const Technique * technique );
