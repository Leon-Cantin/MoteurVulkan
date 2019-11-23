#include "bindless_textures.h"

uint32_t RegisterBindlessTexture( BindlessTexturesState* state, GfxImage* image, eSamplers eSampler )
{
	VkSampler sampler = GetSampler( eSampler );
	state->_bindlessTextures[state->_bindlessTexturesCount] = { image, sampler };
	return state->_bindlessTexturesCount++;
}