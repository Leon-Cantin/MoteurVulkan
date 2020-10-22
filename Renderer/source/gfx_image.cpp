#include "gfx_image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "ktx_loader.h"

#include <stdexcept>

void Load3DTexture( const char* filename, GfxImage* o_image, I_ImageAlloctor* allocator )
{
	std::vector<char> pixels;
	KTX_Header header;
	TextureType textureType;
	GfxFormat format = GfxFormat::R8G8B8A8_UNORM;
	load_ktx( filename, pixels, header, textureType );

	assert( textureType == TextureType::Cube );

	int texWidth = header.pixelwidth, texHeight = header.pixelheight;

	*o_image = CreateCubeImage( ( uint32_t )texWidth, ( uint32_t )texHeight, 1, format, GfxImageUsageFlagBits::TRANSFER_SRC | GfxImageUsageFlagBits::TRANSFER_DST | GfxImageUsageFlagBits::SAMPLED );

	MarkGfxObject( o_image->image, filename );

	allocator->Allocate( o_image->image, &o_image->gfx_mem_alloc );
	allocator->UploadData( *o_image, pixels.data() );

	o_image->imageView = CreateCubeImageView( *o_image );
	MarkGfxObject( o_image->imageView, filename );
}

void Load2DTexture( void * data, uint32_t width, uint32_t height, uint32_t miplevels, GfxFormat format, GfxImage* o_image, I_ImageAlloctor* allocator )
{
	*o_image = CreateImage( width, height, miplevels, format, GfxImageUsageFlagBits::TRANSFER_SRC | GfxImageUsageFlagBits::TRANSFER_DST | GfxImageUsageFlagBits::SAMPLED );

	allocator->Allocate( o_image->image, &o_image->gfx_mem_alloc );
	allocator->UploadData( *o_image, data );

	o_image->imageView = CreateImageView( *o_image );
}

void Load2DTextureFromFile( const char* filename, GfxImage* o_image, I_ImageAlloctor* allocator )
{
	int texWidth, texHeight, channelCount;
	stbi_uc* pixels = stbi_load( filename, &texWidth, &texHeight, &channelCount, STBI_rgb_alpha );
	GfxDeviceSize imageSize = texWidth * texHeight * 4;
	const uint32_t mipLevels = 1; /*static_cast< uint32_t >(std::floor( std::log2( std::max( texWidth, texHeight ) ) )) + 1;*/

	if( !pixels )
		throw std::runtime_error( "failed to load texture image!" );

	*o_image = CreateImage( ( uint32_t )texWidth, ( uint32_t )texHeight, mipLevels, GfxFormat::R8G8B8A8_UNORM, GfxImageUsageFlagBits::TRANSFER_SRC | GfxImageUsageFlagBits::TRANSFER_DST | GfxImageUsageFlagBits::SAMPLED );
	MarkGfxObject( o_image->image, filename );

	allocator->Allocate( o_image->image, &o_image->gfx_mem_alloc );
	allocator->UploadData( *o_image, pixels );

	o_image->imageView = CreateImageView( *o_image );
	MarkGfxObject( o_image->imageView, filename );

	stbi_image_free( pixels );

	//TODO: get a buffer to copy our data straight into
	/*void* mem = allocator->GetMemory();
	memcpy( mem, pixels, imageSize );
	stbi_image_free( pixels );
	allocator->Flush( mem );*/
}

void CreateSolidColorImage( glm::vec4 color, GfxImage* o_image, I_ImageAlloctor* allocator )
{
	assert( !IsValid( *o_image ) );

	const uint32_t width = 4, height = 4;
	const GfxDeviceSize memorySize = width * height * 4;
	uint8_t pixels[memorySize];
	for( uint32_t i = 0; i < width * height; ++i )
	{
		pixels[i * 4] = static_cast< uint8_t >(color.r * 255.0f);
		pixels[i * 4 + 1] = static_cast< uint8_t >(color.g * 255.0f);
		pixels[i * 4 + 2] = static_cast< uint8_t >(color.b * 255.0f);
		pixels[i * 4 + 3] = static_cast< uint8_t >(color.a * 255.0f);
	}

	*o_image = CreateImage( width, height, 1, GfxFormat::R8G8B8A8_UNORM, GfxImageUsageFlagBits::TRANSFER_SRC | GfxImageUsageFlagBits::TRANSFER_DST | GfxImageUsageFlagBits::SAMPLED );

	allocator->Allocate( o_image->image, &o_image->gfx_mem_alloc );
	allocator->UploadData( *o_image, pixels );

	o_image->imageView = CreateImageView( *o_image );
}

void generateMipmaps( GfxCommandBuffer commandBuffer, GfxApiImage image, GfxFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels )
{
	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	/*All mips should be in DST_OPTIMAL at the start
	Turn the last mip into SRC_OPTIMAL to transfer from last to current
	At the end it is changed to SHADER_READ_ONLY_OPTIMAL*/
	for( uint32_t i = 1; i < mipLevels; ++i ) {
		GfxImageBarrier( commandBuffer, image, GfxLayout::TRANSFER, GfxAccess::WRITE, GfxLayout::TRANSFER, GfxAccess::READ, i - 1, 1, 0, 1 );

		const uint32_t srcMip = i - 1;
		const uint32_t dstMip = i;
		const int32_t dstMipWidth = mipWidth > 1 ? mipWidth / 2 : 1;
		const int32_t dstMipHeight = mipHeight > 1 ? mipHeight / 2 : 1;
		CmdBlitImage( commandBuffer, image, 0, 0, 0, mipWidth, mipHeight, 1, srcMip,
			image, 0, 0, 0, dstMipWidth, dstMipHeight, 1, dstMip, GfxFilter::LINEAR );

		GfxImageBarrier( commandBuffer, image, GfxLayout::TRANSFER, GfxAccess::READ, GfxLayout::COLOR, GfxAccess::READ, i - 1, 1, 0, 1 );

		if( mipWidth > 1 ) 
			mipWidth /= 2;
		if( mipHeight > 1 ) 
			mipHeight /= 2;
	}

	//Transition the last one
	GfxImageBarrier( commandBuffer, image, GfxLayout::TRANSFER, GfxAccess::WRITE, GfxLayout::COLOR, GfxAccess::READ, mipLevels - 1, 1, 0, 1 );
}