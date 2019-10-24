#include "bindless_textures.h"

uint32_t RegisterBindlessTexture( BindlessTexturesState* state, GfxImage* image )
{
	VkSampler sampler = GetSampler( Samplers::Trilinear );
	state->_bindlessTextures[state->_bindlessTexturesCount] = { image, sampler };
	return state->_bindlessTexturesCount++;
}