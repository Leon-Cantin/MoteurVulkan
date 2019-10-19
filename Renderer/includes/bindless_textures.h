#pragma once
#include "gfx_image.h"

struct BindlessTexturesState
{
	VkDescriptorImageInfo _bindlessTextures[5];
	uint32_t _bindlessTexturesCount = 0;
};

uint32_t RegisterBindlessTexture( BindlessTexturesState* state, const GfxImage* image );