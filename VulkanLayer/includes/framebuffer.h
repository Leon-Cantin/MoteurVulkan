#pragma once

#include "vk_globals.h"

#include "renderpass.h"

#include <array>
#include <cassert>

FrameBuffer CreateFrameBuffer( GfxImageView* colors, uint32_t colorCount, GfxImageView* opt_depth, VkExtent2D extent, const RenderPass& renderPass );
void Destroy( FrameBuffer* frameBuffer );
