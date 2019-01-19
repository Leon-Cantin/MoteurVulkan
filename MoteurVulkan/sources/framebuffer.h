#pragma once

#include "vk_globals.h"
#include "vk_image.h"

#include <array>
#include <cassert>

struct FrameBuffer
{
	VkFramebuffer frameBuffer;
	VkExtent2D extent;
	uint32_t layerCount;
	uint32_t colorCount;
	uint32_t depthCount;
};

void createFrameBuffer(GfxImage* colors, uint32_t colorCount, GfxImage* depth, VkExtent2D extent, VkRenderPass renderPass, FrameBuffer* o_frameBuffer);
