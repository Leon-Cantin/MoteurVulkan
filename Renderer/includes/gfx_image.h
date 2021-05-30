#pragma once
#include "vk_globals.h"
#include "gfx_samplers.h"

#include <glm/vec4.hpp>

class I_ImageAlloctor
{
public:
	virtual bool Allocate( R_HW::GfxApiImage image, R_HW::GfxMemAlloc* o_gfx_mem_alloc ) = 0;
	virtual bool UploadData( const R_HW::GfxImage& image, const void* data ) = 0;
};

void Load3DTexture( const char* filename, R_HW::GfxImage* o_image, I_ImageAlloctor* allocator );
void Load2DTextureFromFile( const char* filename, R_HW::GfxImage* o_image, I_ImageAlloctor* allocator );
void Load2DTexture( void * data, uint32_t width, uint32_t height, uint32_t miplevels, R_HW::GfxFormat format, R_HW::GfxImage* o_image, I_ImageAlloctor* allocator );
void CreateSolidColorImage( glm::vec4 color, R_HW::GfxImage* o_image, I_ImageAlloctor* allocator );
void generateMipmaps( R_HW::GfxCommandBuffer commandBuffer, R_HW::GfxApiImage image, R_HW::GfxFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels );