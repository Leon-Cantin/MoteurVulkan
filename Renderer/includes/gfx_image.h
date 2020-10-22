#pragma once
#include "vk_globals.h"
#include "gfx_samplers.h"

#include <glm/vec4.hpp>

class I_ImageAlloctor
{
public:
	virtual bool Allocate( GfxApiImage image, GfxMemAlloc* o_gfx_mem_alloc ) = 0;
	virtual bool UploadData( const GfxImage& image, const void* data ) = 0;
};

void Load3DTexture( const char* filename, GfxImage* o_image, I_ImageAlloctor* allocator );
void Load2DTextureFromFile( const char* filename, GfxImage* o_image, I_ImageAlloctor* allocator );
void Load2DTexture( void * data, uint32_t width, uint32_t height, uint32_t miplevels, GfxFormat format, GfxImage* o_image, I_ImageAlloctor* allocator );
void CreateSolidColorImage( glm::vec4 color, GfxImage* o_image, I_ImageAlloctor* allocator );
void generateMipmaps( GfxCommandBuffer commandBuffer, GfxApiImage image, GfxFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels );