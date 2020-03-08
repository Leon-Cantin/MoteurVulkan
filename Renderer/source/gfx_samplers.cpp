#include "gfx_samplers.h"
#include "vk_debug.h"

#include <algorithm>
#include <array>

std::array<VkSampler, ( size_t )(eSamplers::Count)> samplers;

static void createPointSampler( VkSampler* o_sampler )
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = std::numeric_limits<float>::max();

	if( vkCreateSampler( g_vk.device, &samplerInfo, nullptr, o_sampler ) != VK_SUCCESS )
		throw std::runtime_error( "failed to create texture sampler!" );

	MarkVkObject( ( uint64_t )*o_sampler, VK_OBJECT_TYPE_SAMPLER, "point sampler" );
}

static void createTriLinearSampler( VkSampler* o_sampler )
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = std::numeric_limits<float>::max();

	if( vkCreateSampler( g_vk.device, &samplerInfo, nullptr, o_sampler ) != VK_SUCCESS )
		throw std::runtime_error( "failed to create texture sampler!" );

	MarkVkObject( ( uint64_t )*o_sampler, VK_OBJECT_TYPE_SAMPLER, "trilinear sampler" );
}

static void createShadowSampler( VkSampler* o_sampler )
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_TRUE;
	samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = std::numeric_limits<float>::max();

	if( vkCreateSampler( g_vk.device, &samplerInfo, nullptr, o_sampler ) != VK_SUCCESS )
		throw std::runtime_error( "failed to create texture sampler!" );

	MarkVkObject( ( uint64_t )*o_sampler, VK_OBJECT_TYPE_SAMPLER, "shadow sampler" );
}

void InitSamplers()
{
	createPointSampler( &samplers[( size_t )eSamplers::Point] );
	createTriLinearSampler( &samplers[( size_t )eSamplers::Trilinear] );
	createShadowSampler( &samplers[( size_t )eSamplers::Shadow] );
}

void DestroySamplers()
{
	for( size_t i = 0; i < samplers.size(); ++i )
		vkDestroySampler( g_vk.device, samplers[i], nullptr );
}

VkSampler GetSampler( eSamplers samplerId )
{
	return samplers[( size_t )samplerId];
}