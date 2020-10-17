#pragma once
#include "vk_globals.h"
#include "vk_memory.h"

#include <vector>

void BindMemory( VkImage image, VkDeviceMemory memory );
void BindMemory( VkImage image, const GfxMemAlloc& gfx_mem_alloc );

void copyBufferToImage( VkCommandBuffer commandBuffer, VkBuffer buffer, uint32_t bufferOffset, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount );

bool hasStencilComponent(VkFormat format);
VkFormat findDepthFormat();
VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);


struct GfxImage {
	//TODO: should split into the following. Also fix IsValid to check the image
	/*
	-image
		-image
		-format
		-extent
		-miplevels
	-view
	-alloc
	*/
	GfxApiImage image = VK_NULL_HANDLE;
	GfxImageView imageView = VK_NULL_HANDLE;
	GfxFormat format;
	VkExtent2D extent;
	uint32_t layers;
	uint32_t mipLevels;
	GfxMemAlloc gfx_mem_alloc;
};

struct GfxImageSamplerCombined
{
	GfxImage* image;
	VkSampler sampler = VK_NULL_HANDLE;
};

inline bool IsValid( const GfxImage& image )
{
	return IsValid( image.gfx_mem_alloc );
}

GfxImage CreateImage( uint32_t width, uint32_t height, uint32_t mipLevels, GfxFormat format, GfxImageUsageFlags usage );
GfxImage CreateCubeImage( uint32_t width, uint32_t height, uint32_t mipLevels, GfxFormat format, GfxImageUsageFlags usage );
GfxImageView CreateCubeImageView( const GfxImage& parentImage );
GfxImageView CreateImageView( const GfxImage& parentImage );

void DestroyImage( GfxImage* image );
