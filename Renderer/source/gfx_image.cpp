#include "gfx_image.h"

#include "vk_debug.h"
#include "vk_commands.h"
#include "vk_buffer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "ktx_loader.h"

#include <stdexcept>

void Load3DTexture( const char* filename, GfxImage* o_image, I_ImageAlloctor* allocator )
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
	o_image->mipLevels = 1;
	o_image->extent = { ( uint32_t )texWidth, ( uint32_t )texHeight };
	o_image->format = format;
	create_cube_image( texWidth, texHeight, o_image->mipLevels, format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, &o_image->image );
	MarkVkObject( ( uint64_t )o_image->image, VK_OBJECT_TYPE_IMAGE, filename );

	allocator->Allocate( o_image->image, &o_image->gfx_mem_alloc );
	allocator->UploadData( *o_image, pixels.data() );

	o_image->imageView = create_image_view( o_image->image, VK_IMAGE_VIEW_TYPE_CUBE, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, o_image->mipLevels );
	MarkVkObject( ( uint64_t )o_image->imageView, VK_OBJECT_TYPE_IMAGE_VIEW, filename );
}

void Load2DTexture( void * data, uint32_t width, uint32_t height, uint32_t miplevels, VkFormat format, GfxImage* o_image, I_ImageAlloctor* allocator )
{
	o_image->mipLevels = miplevels;
	o_image->format = format;
	o_image->extent.width = width;
	o_image->extent.height = height;

	create_image( o_image->extent.width, o_image->extent.height, o_image->mipLevels, o_image->format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, &o_image->image );

	allocator->Allocate( o_image->image, &o_image->gfx_mem_alloc );
	allocator->UploadData( *o_image, data );

	o_image->imageView = create_image_view( o_image->image, VK_IMAGE_VIEW_TYPE_2D, o_image->format, VK_IMAGE_ASPECT_COLOR_BIT, o_image->mipLevels );
}

void Load2DTextureFromFile( const char* filename, GfxImage* o_image, I_ImageAlloctor* allocator )
{
	int texWidth, texHeight, channelCount;
	stbi_uc* pixels = stbi_load( filename, &texWidth, &texHeight, &channelCount, STBI_rgb_alpha );
	VkDeviceSize imageSize = texWidth * texHeight * 4;
	o_image->extent = { (uint32_t)texWidth, (uint32_t)texHeight };
	o_image->format = VK_FORMAT_R8G8B8A8_UNORM;
	o_image->mipLevels = 1; /*static_cast< uint32_t >(std::floor( std::log2( std::max( texWidth, texHeight ) ) )) + 1;*/

	if( !pixels )
		throw std::runtime_error( "failed to load texture image!" );

	create_image( o_image->extent.width, o_image->extent.height, o_image->mipLevels, o_image->format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, &o_image->image );
	MarkVkObject( ( uint64_t )o_image->image, VK_OBJECT_TYPE_IMAGE, filename );

	allocator->Allocate( o_image->image, &o_image->gfx_mem_alloc );
	allocator->UploadData( *o_image, pixels );

	o_image->imageView = create_image_view( o_image->image, VK_IMAGE_VIEW_TYPE_2D, o_image->format, VK_IMAGE_ASPECT_COLOR_BIT, o_image->mipLevels );
	MarkVkObject( ( uint64_t )o_image->imageView, VK_OBJECT_TYPE_IMAGE_VIEW, filename );

	stbi_image_free( pixels );

	//TODO: get a buffer to copy our data straight into
	/*void* mem = allocator->GetMemory();
	memcpy( mem, pixels, imageSize );
	stbi_image_free( pixels );
	allocator->Flush( mem );*/
}

void DestroyImage( GfxImage* image )
{
	vkDestroyImageView( g_vk.device, image->imageView, nullptr );
	vkDestroyImage( g_vk.device, image->image, nullptr );
	destroy_gfx_memory( &image->gfx_mem_alloc );
	image = {};
}

void CreateSolidColorImage( glm::vec4 color, GfxImage* o_image, I_ImageAlloctor* allocator )
{
	assert( !IsValid( *o_image ) );

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

	create_image( o_image->extent.width, o_image->extent.height, o_image->mipLevels, o_image->format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, &o_image->image );

	allocator->Allocate( o_image->image, &o_image->gfx_mem_alloc );
	allocator->UploadData( *o_image, pixels );

	o_image->imageView = create_image_view( o_image->image, VK_IMAGE_VIEW_TYPE_2D, o_image->format, VK_IMAGE_ASPECT_COLOR_BIT, o_image->mipLevels );
}

void generateMipmaps( VkCommandBuffer commandBuffer, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels )
{
	//TODO: Move that check somewhere else
	// Check if image format supports linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties( g_vk.physicalDevice, imageFormat, &formatProperties );
	if( !(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) )
		throw std::runtime_error( "texture image format does not support linear blitting!" );

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	/*All mips should be in DST_OPTIMAL at the start
	Turn the last mip into SRC_OPTIMAL to transfer from last to current
	At the end it is changed to SHADER_READ_ONLY_OPTIMAL*/
	for( uint32_t i = 1; i < mipLevels; ++i ) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier( commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier );

		VkImageBlit blit = {};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;

		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage( commandBuffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR );

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier( commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier );

		if( mipWidth > 1 ) mipWidth /= 2;
		if( mipHeight > 1 ) mipHeight /= 2;
	}

	//Transition the last one
	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier( commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier );
}

void create_image_simple( uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlagBits aspect_flags, VkMemoryPropertyFlags properties, GfxImage* o_gfx_image )
{
	o_gfx_image->extent = { width, height };
	o_gfx_image->mipLevels = mipLevels;
	o_gfx_image->format = format;

	create_image( width, height, mipLevels, format, usage, &o_gfx_image->image );

	VkMemoryRequirements mem_requirements;
	vkGetImageMemoryRequirements( g_vk.device, o_gfx_image->image, &mem_requirements );
	uint32_t memoryType = findMemoryType( mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	o_gfx_image->gfx_mem_alloc = allocate_gfx_memory( mem_requirements.size, memoryType );
	BindMemory( o_gfx_image->image, o_gfx_image->gfx_mem_alloc );

	o_gfx_image->imageView = create_image_view( o_gfx_image->image, VK_IMAGE_VIEW_TYPE_2D, format, aspect_flags, mipLevels );
}