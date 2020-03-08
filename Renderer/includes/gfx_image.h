#pragma once
#include "vk_image.h"
#include "gfx_samplers.h"

#include <glm/vec4.hpp>

struct GfxImage {
	VkImage image = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;
	VkFormat format;
	VkExtent2D extent;
	uint32_t mipLevels;
	GfxMemAlloc gfx_mem_alloc;
};

struct GfxImageSamplerCombined
{
	GfxImage* image;
	VkSampler sampler = VK_NULL_HANDLE;
};

class I_ImageAlloctor
{
public:
	virtual bool Allocate( VkImage image, GfxMemAlloc* o_gfx_mem_alloc ) = 0;
	virtual bool UploadData( const GfxImage& image, const void* data ) = 0;
};

void Load3DTexture( const char* filename, GfxImage* o_image, I_ImageAlloctor* allocator );
void Load2DTextureFromFile( const char* filename, GfxImage* o_image, I_ImageAlloctor* allocator );
void Load2DTexture( void * data, uint32_t width, uint32_t height, uint32_t miplevels, VkFormat format, GfxImage* o_image, I_ImageAlloctor* allocator );
void CreateSolidColorImage( glm::vec4 color, GfxImage* o_image, I_ImageAlloctor* allocator );
void generateMipmaps( VkCommandBuffer commandBuffer, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels );

void DestroyImage( GfxImage* image );

void create_image_simple( uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlagBits aspect_flags, VkMemoryPropertyFlags properties, GfxImage* o_gfx_image );
