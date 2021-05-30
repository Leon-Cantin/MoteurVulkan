#pragma once

#include "vk_globals.h"

#include "scene_frame_data.h"
#include "material.h"
#include "gfx_image.h"
#include "frame_graph.h"

struct TextZone {
	float x;
	float y;
	std::string text;
};


R_HW::GpuPipelineLayout GetTextPipelineLayout();
R_HW::GpuPipelineStateDesc GetTextPipelineState();
void CleanupTextRenderPass();
void TextRecordDrawCommandsBuffer( R_HW::GfxCommandBuffer graphicsCommandBuffer, const FG::TaskInputData& inputData );
void CreateTextVertexBuffer( size_t maxCharCount );
void UpdateText( const TextZone * textZones, size_t textZonesCount, VkExtent2D surfaceExtent );
void LoadFontTexture();
const R_HW::GfxImage* GetTextImage();