#include "vk_image.h"
#include "vk_commands.h"
#include "vk_memory.h"
#include "vk_buffer.h"
#include "vk_debug.h"
#include <glm\vec4.hpp>

#include <assert.h>
#include <algorithm>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "ktx_loader.h"

std::array<VkSampler, (size_t)(Samplers::Count)> samplers;

void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
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

	endSingleTimeCommands(commandBuffer);
}

bool hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t layerCount)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = layerCount;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (hasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {//TODO: this is clearly too generic maybe have a chain that determine these
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;;

		sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
}

void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t layerCount)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	transitionImageLayout(commandBuffer, image, format, oldLayout, newLayout, mipLevels, layerCount);

	endSingleTimeCommands(commandBuffer);
}

VkImageView createCubeImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
	VkImageViewCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	create_info.image = image;
	create_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
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
	create_info.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(g_vk.device, &create_info, nullptr, &imageView) != VK_SUCCESS)
		throw std::runtime_error("failed to create image views!");

	return imageView;
}

VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
	VkImageViewCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	create_info.image = image;
	create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
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
	create_info.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView( g_vk.device, &create_info, nullptr, &imageView) != VK_SUCCESS)
		throw std::runtime_error("failed to create image views!");

	return imageView;
}

void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
	// Check if image format supports linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(g_vk.physicalDevice, imageFormat, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		throw std::runtime_error("texture image format does not support linear blitting!");

	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

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
	for (uint32_t i = 1; i < mipLevels; ++i) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

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

		vkCmdBlitImage(commandBuffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	//Transition the last one
	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	endSingleTimeCommands(commandBuffer);
}

void create_cube_image(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = static_cast<uint32_t>(width);
	imageInfo.extent.height = static_cast<uint32_t>(height);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 6;
	imageInfo.format = format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL; // use VK_IMAGE_TILING_LINEAR for direct access to texels
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; // Optional

	if (vkCreateImage(g_vk.device, &imageInfo, nullptr, &image) != VK_SUCCESS)
		throw std::runtime_error("failed to create image!");

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(g_vk.device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(g_vk.device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate image memory!");

	vkBindImageMemory(g_vk.device, image, imageMemory, 0);
}

void create_image(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = static_cast<uint32_t>(width);
	imageInfo.extent.height = static_cast<uint32_t>(height);
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

	if (vkCreateImage(g_vk.device, &imageInfo, nullptr, &image) != VK_SUCCESS)
		throw std::runtime_error("failed to create image!");

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(g_vk.device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(g_vk.device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate image memory!");

	vkBindImageMemory(g_vk.device, image, imageMemory, 0);
}

VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(g_vk.physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

void copyImageToDeviceLocalMemory(void* pixels, VkDeviceSize imageSize, uint32_t texWidth, uint32_t texHeight, uint32_t layerCount, uint32_t mipLevel, VkFormat format, VkImage image)
{
	//TODO: Is using staging buffer for texture also better on AMD?
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	create_buffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	UpdateBuffer(stagingBufferMemory, pixels,imageSize);

	transitionImageLayout(image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevel, layerCount);
	copyBufferToImage(stagingBuffer, image, texWidth, texHeight, layerCount);

	//TODO: mem requirement will be computed by memRequirement in create_image... I hope it's right
	//GenerateMipmaps will do the transition, do it if we don't generate them
	if (mipLevel > 1)
		generateMipmaps(image, format, texWidth, texHeight, mipLevel);
	else
		transitionImageLayout(image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevel, layerCount);

	vkDestroyBuffer(g_vk.device, stagingBuffer, nullptr);
	vkFreeMemory(g_vk.device, stagingBufferMemory, nullptr);
}

void Load3DTexture(const char* filename, GfxImage& o_image)
{
	std::vector<char> pixels;
	KTX_Header header;
	TextureType textureType;
	uint32_t layerCount = 6;
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
	load_ktx(filename, pixels, header, textureType);

	assert(textureType == TextureType::Cube);

	int texWidth = header.pixelwidth, texHeight = header.pixelheight;
	VkDeviceSize imageSize = texWidth * texHeight * 4 * layerCount;
	o_image.mipLevels = 1;

	//TODO: now it's also a src image because of the generating mip map. Perhaps we could change it back to only dst somehow?
	create_cube_image(texWidth, texHeight, o_image.mipLevels, format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, o_image.image, o_image.memory);

	copyImageToDeviceLocalMemory(pixels.data(), imageSize, texWidth, texHeight, layerCount, 1, format, o_image.image);

	o_image.imageView = createCubeImageView(o_image.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	MarkVkObject((uint64_t)o_image.image, VK_OBJECT_TYPE_IMAGE, filename);
	MarkVkObject((uint64_t)o_image.imageView, VK_OBJECT_TYPE_IMAGE_VIEW, filename);
}

void Load2DTexture(void * data, uint32_t width, uint32_t height, uint32_t miplevels, uint32_t pixelByteSize,VkFormat format, GfxImage& o_image)
{
	VkDeviceSize imageSize = width * height * pixelByteSize;
	o_image.mipLevels = miplevels;

	create_image(width, height, o_image.mipLevels, format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, o_image.image, o_image.memory);

	copyImageToDeviceLocalMemory(data, imageSize, width, height, 1, o_image.mipLevels, format, o_image.image);

	o_image.imageView = createImageView(o_image.image, format, VK_IMAGE_ASPECT_COLOR_BIT, o_image.mipLevels);
}


void Load2DTextureFromFile(const char* filename, GfxImage& o_image)
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
	o_image.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

	if (!pixels)
		throw std::runtime_error("failed to load texture image!");

	//TODO: now it's also a src image because of the generating mip map. Perhaps we could change it back to only dst somehow?
	create_image(texWidth, texHeight, o_image.mipLevels, format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, o_image.image, o_image.memory);

	copyImageToDeviceLocalMemory(pixels, imageSize, texWidth, texHeight, 1, o_image.mipLevels, format, o_image.image);

	stbi_image_free(pixels);
	o_image.imageView = createImageView(o_image.image, format, VK_IMAGE_ASPECT_COLOR_BIT, o_image.mipLevels);

	MarkVkObject((uint64_t)o_image.image, VK_OBJECT_TYPE_IMAGE, filename);
	MarkVkObject((uint64_t)o_image.imageView, VK_OBJECT_TYPE_IMAGE_VIEW, filename);
}

void DestroyImage(GfxImage& image)
{
	vkDestroyImageView(g_vk.device, image.imageView, nullptr);
	vkDestroyImage(g_vk.device, image.image, nullptr);
	vkFreeMemory(g_vk.device, image.memory, nullptr);
	image.imageView = VK_NULL_HANDLE;
	image.image = VK_NULL_HANDLE;
	image.memory = VK_NULL_HANDLE;
}

void CreateSolidColodImage(glm::vec4 color, GfxImage* o_image)
{
	const uint32_t width = 4, height = 4;
	o_image->extent = { width, height };
	o_image->format = VK_FORMAT_R8G8B8A8_UNORM;
	o_image->mipLevels = 1;
	const VkDeviceSize memorySize = width * height * 4;
	uint8_t pixels[memorySize];
	for (uint32_t i = 0; i < width * height; ++i)
	{
		pixels[i * 4] = static_cast<uint8_t>(color.r * 255.0f);
		pixels[i * 4 + 1] = static_cast<uint8_t>(color.g * 255.0f);
		pixels[i * 4 + 2] = static_cast<uint8_t>(color.b * 255.0f);
		pixels[i * 4 + 3] = static_cast<uint8_t>(color.a * 255.0f);
	}

	create_image(width, height, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, o_image->image, o_image->memory);
	copyImageToDeviceLocalMemory(pixels, memorySize, o_image->extent.width, o_image->extent.height, 1, o_image->mipLevels, o_image->format, o_image->image);
	o_image->imageView = createImageView(o_image->image, o_image->format, VK_IMAGE_ASPECT_COLOR_BIT, o_image->mipLevels);
}

static void createTriLinearSampler(VkSampler* o_sampler)
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

	if (vkCreateSampler(g_vk.device, &samplerInfo, nullptr, o_sampler) != VK_SUCCESS)
		throw std::runtime_error("failed to create texture sampler!");

	MarkVkObject((uint64_t)*o_sampler, VK_OBJECT_TYPE_SAMPLER, "trilinear sampler");
}

static void createShadowSampler(VkSampler* o_sampler)
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

	if (vkCreateSampler(g_vk.device, &samplerInfo, nullptr, o_sampler) != VK_SUCCESS)
		throw std::runtime_error("failed to create texture sampler!");

	MarkVkObject((uint64_t)*o_sampler, VK_OBJECT_TYPE_SAMPLER, "shadow sampler");
}

void InitSamplers()
{
	createTriLinearSampler(&samplers[(size_t)Samplers::Trilinear]);
	createShadowSampler(&samplers[(size_t)Samplers::Shadow]);
}

void DestroySamplers()
{
	for (size_t i = 0; i < samplers.size(); ++i)
		vkDestroySampler(g_vk.device, samplers[i], nullptr);
}

VkSampler GetSampler(Samplers samplerId)
{
	return samplers[(size_t)samplerId];
}