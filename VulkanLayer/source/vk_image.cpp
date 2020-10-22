
#include "vk_globals.h"
#include <glm/vec4.hpp>

#include <assert.h>
#include <algorithm>
#include <stdexcept>

void copyBufferToImage( GfxCommandBuffer commandBuffer, VkBuffer buffer, uint32_t bufferOffset, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount )
{
	VkBufferImageCopy region = {};
	region.bufferOffset = bufferOffset;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = layerCount;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);
}

bool hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkImageLayout ConvertToVkImageLayout( GfxLayout layout, GfxAccess access )
{
	switch( layout )
	{
	case GfxLayout::UNDEFINED:
		return VK_IMAGE_LAYOUT_UNDEFINED;
	case GfxLayout::GENERAL:
		return VK_IMAGE_LAYOUT_GENERAL;
	case GfxLayout::TRANSFER:
		return access == GfxAccess::READ ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	case GfxLayout::DEPTH_STENCIL:
		return access == GfxAccess::READ ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	case GfxLayout::COLOR:
		return access == GfxAccess::READ ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	case GfxLayout::PRESENT:
		return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	default:
		assert( false );
		return VK_IMAGE_LAYOUT_UNDEFINED;
	}
}

VkAttachmentLoadOp ConvertVkLoadOp( GfxLoadOp loadOp )
{
	return static_cast< VkAttachmentLoadOp >(loadOp);
}

VkImageAspectFlags GetAspectFlags( VkFormat format )
{
	switch( format )
	{
	case VK_FORMAT_D32_SFLOAT:
		return VK_IMAGE_ASPECT_DEPTH_BIT;
	case VK_FORMAT_S8_UINT:
		return VK_IMAGE_ASPECT_STENCIL_BIT;
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
	case VK_FORMAT_D16_UNORM_S8_UINT:
	case VK_FORMAT_D24_UNORM_S8_UINT:
		return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_DEPTH_BIT;
	default:
		return VK_IMAGE_ASPECT_COLOR_BIT;
	}
}

static VkImageView CreateImageView( VkImage image, VkFormat format, VkImageViewType imageViewType, VkImageAspectFlags aspectFlags, uint32_t mipLevels, uint32_t layerCount )
{
	//TODO: use something like this, but with the format, to get the aspectMask
/*if( newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ) {
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

	if( hasStencilComponent( format ) ) {
		barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
}
else {
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
}*/

	VkImageViewCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	create_info.image = image;
	create_info.viewType = imageViewType;
	create_info.format = format;

	/* Not required since it's defined as 0
	create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;*/

	create_info.subresourceRange.aspectMask = aspectFlags;
	create_info.subresourceRange.baseMipLevel = 0;
	create_info.subresourceRange.levelCount = mipLevels;
	create_info.subresourceRange.baseArrayLayer = 0;
	create_info.subresourceRange.layerCount = layerCount;

	VkImageView imageView;
	if( vkCreateImageView( g_gfx.device.device, &create_info, nullptr, &imageView ) != VK_SUCCESS )
		throw std::runtime_error( "failed to create image views!" );

	return imageView;
}

void BindMemory( VkImage image, VkDeviceMemory memory )
{
	vkBindImageMemory( g_gfx.device.device, image, memory, 0 );
}

void BindMemory( VkImage image, const GfxMemAlloc& gfx_mem_alloc )
{
	vkBindImageMemory( g_gfx.device.device, image, gfx_mem_alloc.memory, gfx_mem_alloc.offset );
}

static VkImage CreateImage( const VkImageCreateInfo& imageInfo )
{
	VkImage image;

	if( vkCreateImage( g_gfx.device.device, &imageInfo, nullptr, &image ) != VK_SUCCESS )
		throw std::runtime_error( "failed to create image!" );

	return image;
}

#define CUBE_IMAGE_LAYER_COUNT 6;

static VkImageCreateInfo fill_cube_image_info( uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageUsageFlags usage )
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = static_cast< uint32_t >(width);
	imageInfo.extent.height = static_cast< uint32_t >(height);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = CUBE_IMAGE_LAYER_COUNT;
	imageInfo.format = format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL; // use VK_IMAGE_TILING_LINEAR for direct access to texels
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; // Optional
	return imageInfo;
}

static VkImageCreateInfo fill_image_info( VkImageType type, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageUsageFlags usage )
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = type;
	imageInfo.extent.width = static_cast< uint32_t >(width);
	imageInfo.extent.height = static_cast< uint32_t >(height);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL; // use VK_IMAGE_TILING_LINEAR for direct access to texels
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0; // Optional VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT for cube map
	return imageInfo;
}

static void create_image( uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageUsageFlags usage, VkImage* image )
{
	VkImageCreateInfo imageInfo = fill_image_info( VK_IMAGE_TYPE_2D, width, height, mipLevels, format, usage );
	*image = CreateImage( imageInfo );
}

void create_cube_image( uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageUsageFlags usage, VkImage* image )
{
	VkImageCreateInfo imageInfo = fill_cube_image_info( width, height, mipLevels, format, usage );
	*image = CreateImage( imageInfo );
}

VkFormat findDepthFormat() {
	return findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(g_gfx.physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

GfxImage CreateImage( uint32_t width, uint32_t height, uint32_t mipLevels, GfxFormat format, GfxImageUsageFlags usage )
{
	GfxImage image = {};
	create_image( width, height, mipLevels, ToVkFormat( format ), usage, &image.image );
	image.extent = { width, height };
	image.format = format;
	image.layers = 0;
	image.mipLevels = mipLevels;

	return image;
}

GfxImage CreateCubeImage( uint32_t width, uint32_t height, uint32_t mipLevels, GfxFormat format, GfxImageUsageFlags usage )
{
	GfxImage image = {};
	create_cube_image( width, height, mipLevels, ToVkFormat( format ), usage, &image.image );
	image.extent = { width, height };
	image.format = format;
	image.layers = CUBE_IMAGE_LAYER_COUNT;
	image.mipLevels = mipLevels;

	return image;
}

GfxImageView CreateCubeImageView( const GfxImage& parentImage )
{
	const VkFormat format = ToVkFormat( parentImage.format );
	return CreateImageView( parentImage.image, format, VK_IMAGE_VIEW_TYPE_CUBE, GetAspectFlags( format ), parentImage.mipLevels, parentImage.layers );
}

GfxImageView CreateImageView( const GfxImage& parentImage )
{
	const VkFormat format = ToVkFormat( parentImage.format );
	return CreateImageView( parentImage.image, format, VK_IMAGE_VIEW_TYPE_2D, GetAspectFlags( format ), parentImage.mipLevels, 1 );
}

void Destroy( GfxImageView* imageView )
{
	vkDestroyImageView( g_gfx.device.device, *imageView, nullptr );
	imageView = VK_NULL_HANDLE;
}

void DestroyImage( GfxImage* image )
{
	Destroy( &image->imageView );
	vkDestroyImage( g_gfx.device.device, image->image, nullptr );
	destroy_gfx_memory( &image->gfx_mem_alloc );
	image = {};
}