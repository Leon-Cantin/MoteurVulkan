#include "bindless_textures.h"

uint32_t RegisterBindlessTexture( BindlessTexturesState* state, GfxImage* image, eSamplers eSampler )
{
	assert( state->_bindlessTexturesCount < BINDLESS_TEXTURES_MAX );
	VkSampler sampler = eSampler == eSamplers::Count ? VK_NULL_HANDLE : GetSampler( eSampler );
	state->_bindlessTextures[state->_bindlessTexturesCount] = { image, sampler };
	return state->_bindlessTexturesCount++;
}

uint32_t RegisterBindlessTexture( BindlessTexturesState* state, GfxImage* image )
{
	return RegisterBindlessTexture( state, image, eSamplers::Count );
}