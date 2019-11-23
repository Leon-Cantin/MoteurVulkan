#pragma once
#include "gfx_image.h"

struct BindlessTexturesState
{
	GfxImageSamplerCombined _bindlessTextures[5];
	uint32_t _bindlessTexturesCount = 0;
};

uint32_t RegisterBindlessTexture( BindlessTexturesState* state, GfxImage* image, eSamplers eSampler );