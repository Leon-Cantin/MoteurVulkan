#pragma once

#include "vk_globals.h"

#include "frame_graph.h"
#include "scene_frame_data.h"
#include "material.h"
#include "gfx_image.h"
#include <string>

struct TextZone {
	float x;
	float y;
	std::string text;
};


GpuPipelineLayout GetTextPipelineLayout();
GpuPipelineStateDesc GetTextPipelineState();
void CleanupTextRenderPass();
void TextRecordDrawCommandsBuffer( GfxCommandBuffer graphicsCommandBuffer, const FG::TaskInputData& inputData );
void CreateTextVertexBuffer( size_t maxCharCount);
void UpdateText( const TextZone * textZones, size_t textZonesCount, VkExtent2D surfaceExtent);
void LoadFontTexture();
const GfxImage* GetTextImage();