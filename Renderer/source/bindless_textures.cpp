#include "bindless_textures.h"

uint32_t RegisterBindlessTexture( BindlessTexturesState* state, const GfxImage* image )
{
	VkSampler sampler = GetSampler( Samplers::Trilinear );
	state->_bindlessTextures[state->_bindlessTexturesCount] = { sampler, image->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	return state->_bindlessTexturesCount++;
}