#pragma once
#include "gfx_image.h"
#include "../shaders/shadersCommon.h"

struct BindlessTexturesState
{
	R_HW::GfxImageSamplerCombined _bindlessTextures[BINDLESS_TEXTURES_MAX];
	uint32_t _bindlessTexturesCount = 0;
};

uint32_t RegisterBindlessTexture( BindlessTexturesState* state, R_HW::GfxImage* image, eSamplers eSampler );
uint32_t RegisterBindlessTexture( BindlessTexturesState* state, R_HW::GfxImage* image );