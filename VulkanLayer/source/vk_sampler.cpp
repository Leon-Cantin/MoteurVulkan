#include "vk_globals.h"

#include <algorithm>

bool CreateSampler( GfxFilter minFilter, GfxFilter magFilter, GfxMipFilter mipFilter, float anisotropy, GfxSamplerAddressMode samplerAddressMode, GfxCompareOp compareOp, GfxApiSampler* o_sampler )
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	samplerInfo.minFilter = ToVkFilter( minFilter );
	samplerInfo.magFilter = ToVkFilter( magFilter );

	const VkSamplerAddressMode vkSamplerAddressMode = ToVkSamplerAddressMode( samplerAddressMode );
	samplerInfo.addressModeU = vkSamplerAddressMode;
	samplerInfo.addressModeV = vkSamplerAddressMode;
	samplerInfo.addressModeW = vkSamplerAddressMode;

	samplerInfo.anisotropyEnable = anisotropy != 0;
	samplerInfo.maxAnisotropy = anisotropy;

	samplerInfo.borderColor = samplerAddressMode == GfxSamplerAddressMode::CLAMP_TO_BORDER ? VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE : VK_BORDER_COLOR_INT_OPAQUE_BLACK;//TODO: so far used only for the shadow sampler
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = compareOp != GfxCompareOp::NONE;
	samplerInfo.compareOp = ToVkCompareOp( compareOp );

	samplerInfo.mipmapMode = ToVkMipFilter( mipFilter );
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = std::numeric_limits<float>::max();

	if( vkCreateSampler( g_vk.device.device, &samplerInfo, nullptr, o_sampler ) != VK_SUCCESS )
		return false;

	return true;
}

void Destroy( GfxApiSampler* sampler )
{
	vkDestroySampler( g_vk.device.device, *sampler, nullptr );
	*sampler = VK_NULL_HANDLE;
}