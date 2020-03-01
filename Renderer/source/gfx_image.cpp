#include "gfx_image.h"

#include "vk_debug.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "ktx_loader.h"
#include <algorithm>
#include <array>


std::array<VkSampler, ( size_t )(eSamplers::Count)> samplers;

void Load3DTexture( const char* filename, GfxImage& o_image )
{
	std::vector<char> pixels;
	KTX_Header header;
	TextureType textureType;
	uint32_t layerCount = 6;
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
	load_ktx( filename, pixels, header, textureType );

	assert( textureType == TextureType::Cube );

	int texWidth = header.pixelwidth, texHeight = header.pixelheight;
	VkDeviceSize imageSize = texWidth * texHeight * 4 * layerCount;
	o_image.mipLevels = 1;
	o_image.extent = { ( uint32_t )texWidth, ( uint32_t )texHeight };
	o_image.format = format;
	//TODO: now it's also a src image because of the generating mip map. Perhaps we could change it back to only dst somehow?
	create_cube_image( texWidth, texHeight, o_image.mipLevels, format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, o_image.image, o_image.memory );

	copyImageToDeviceLocalMemory( pixels.data(), imageSize, texWidth, texHeight, layerCount, 1, format, o_image.image );

	o_image.imageView = createCubeImageView( o_image.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 1 );

	MarkVkObject( ( uint64_t )o_image.image, VK_OBJECT_TYPE_IMAGE, filename );
	MarkVkObject( ( uint64_t )o_image.imageView, VK_OBJECT_TYPE_IMAGE_VIEW, filename );
}

void Load2DTexture( void * data, uint32_t width, uint32_t height, uint32_t miplevels, uint32_t pixelByteSize, VkFormat format, GfxImage& o_image )
{
	VkDeviceSize imageSize = width * height * pixelByteSize;
	o_image.mipLevels = miplevels;

	create_image_simple( width, height, o_image.mipLevels, format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &o_image.image, &o_image.memory, &o_image.imageView );

	copyImageToDeviceLocalMemory( data, imageSize, width, height, 1, o_image.mipLevels, format, o_image.image );
}

void Load2DTextureFromFile( const char* filename, GfxImage& o_image, I_ImageAlloctor* allocator )
{
	int texWidth, texHeight, channelCount;
	stbi_uc* pixels = stbi_load( filename, &texWidth, &texHeight, &channelCount, STBI_rgb_alpha );
	VkDeviceSize imageSize = texWidth * texHeight * 4;
	o_image.extent = { (uint32_t)texWidth, (uint32_t)texHeight };
	o_image.format = VK_FORMAT_R8G8B8A8_UNORM;
	o_image.mipLevels = 1; /*static_cast< uint32_t >(std::floor( std::log2( std::max( texWidth, texHeight ) ) )) + 1;*/

	if( !pixels )
		throw std::runtime_error( "failed to load texture image!" );

	//TODO: now it's also a src image because of the generating mip map. Perhaps we could change it back to only dst somehow?
	create_image( o_image.extent.width, o_image.extent.height, o_image.mipLevels, o_image.format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, &o_image.image );
	MarkVkObject( ( uint64_t )o_image.image, VK_OBJECT_TYPE_IMAGE, filename );

	allocator->Allocate( o_image.image );
	allocator->UploadData( o_image, pixels );
	stbi_image_free( pixels );

	//TODO: get a buffer to copy our data straight into
	/*void* mem = allocator->GetMemory();
	memcpy( mem, pixels, imageSize );
	stbi_image_free( pixels );
	allocator->Flush( mem );*/

	o_image.imageView = Create2DImageView( o_image.image, o_image.format, VK_IMAGE_ASPECT_COLOR_BIT, o_image.mipLevels );
	MarkVkObject( ( uint64_t )o_image.imageView, VK_OBJECT_TYPE_IMAGE_VIEW, filename );
}

void DestroyImage( GfxImage& image )
{
	vkDestroyImageView( g_vk.device, image.imageView, nullptr );
	vkDestroyImage( g_vk.device, image.image, nullptr );
	if( image.memory ) //Images allocated on a heap don't have memory
		vkFreeMemory( g_vk.device, image.memory, nullptr );
	image = {};
}

void CreateSolidColorImage( glm::vec4 color, GfxImage* o_image )
{
	const uint32_t width = 4, height = 4;
	o_image->extent = { width, height };
	o_image->format = VK_FORMAT_R8G8B8A8_UNORM;
	o_image->mipLevels = 1;
	const VkDeviceSize memorySize = width * height * 4;
	uint8_t pixels[memorySize];
	for( uint32_t i = 0; i < width * height; ++i )
	{
		pixels[i * 4] = static_cast< uint8_t >(color.r * 255.0f);
		pixels[i * 4 + 1] = static_cast< uint8_t >(color.g * 255.0f);
		pixels[i * 4 + 2] = static_cast< uint8_t >(color.b * 255.0f);
		pixels[i * 4 + 3] = static_cast< uint8_t >(color.a * 255.0f);
	}

	create_image_simple( width, height, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&o_image->image, &o_image->memory, &o_image->imageView );
	copyImageToDeviceLocalMemory( pixels, memorySize, o_image->extent.width, o_image->extent.height, 1, o_image->mipLevels, o_image->format, o_image->image );
}

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

	MarkVkObject( ( uint64_t )*o_sampler, VK_OBJECT_TYPE_SAMPLER, "trilinear sampler" );
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