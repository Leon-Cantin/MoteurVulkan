#pragma once

#include "vk_globals.h"

#include "renderpass.h"
#include "model_asset.h"
#include "scene_frame_data.h"
#include "material.h"
#include "gfx_image.h"

struct TextZone {
	float x;
	float y;
	std::string text;
};


GpuPipelineLayout GetTextPipelineLayout();
GpuPipelineState GetTextPipelineState();
void CleanupTextRenderPass();
void TextRecordDrawCommandsBuffer( uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent, const RenderPass * renderpass, const Technique * technique );
void CreateTextVertexBuffer( size_t maxCharCount );
void UpdateText( const TextZone * textZones, size_t textZonesCount, VkExtent2D surfaceExtent );
void LoadFontTexture();
const GfxImage* GetTextImage();