#pragma once
#include "vk_image.h"

#include <glm/vec4.hpp>

struct GfxImage {
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;
	VkFormat format;
	VkExtent2D extent;
	uint32_t mipLevels;
};

struct GfxImageSamplerCombined
{
	GfxImage* image;
	VkSampler sampler = VK_NULL_HANDLE;
};

enum class eSamplers
{
	Point = 0,
	Trilinear,
	Shadow,
	Count
};

void Load3DTexture( const char* filename, GfxImage& o_image );
void Load2DTextureFromFile( const char* filename, GfxImage& o_image );
void Load2DTexture( void * data, uint32_t width, uint32_t height, uint32_t miplevels, uint32_t pixelByteSize, VkFormat format, GfxImage& o_image );
void CreateSolidColorImage( glm::vec4 color, GfxImage* o_image );

void DestroyImage( GfxImage& image );

void InitSamplers();
void DestroySamplers();
VkSampler GetSampler( eSamplers samplerId );
