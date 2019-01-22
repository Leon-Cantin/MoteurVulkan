#pragma once

#include "vk_globals.h"
#include "vk_image.h"
#include <glm/vec4.hpp>

#include <vector>

struct GfxImage {
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;
	VkFormat format;
	VkExtent2D extent;
	uint32_t mipLevels;
};

enum class Samplers
{
	Trilinear = 0,
	Shadow,
	Count
};

void Load3DTexture(const char* filename, GfxImage& o_image);
void Load2DTextureFromFile(const char* filename, GfxImage& o_image);
void Load2DTexture(void * data, uint32_t width, uint32_t height, uint32_t miplevels, uint32_t pixelByteSize, VkFormat format, GfxImage& o_image);

void DestroyImage(GfxImage& image);

void create_cube_image(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
void create_image(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
VkImageView createCubeImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

//TODO: create an enum with all sampler types to index inside of an array of samplers : sampler getSampler(enumSampler)
void InitSamplers();
void DestroySamplers();
VkSampler GetSampler(Samplers samplerId);

void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);
void copyImageToDeviceLocalMemory(void* pixels, VkDeviceSize imageSize, uint32_t texWidth, uint32_t texHeight, uint32_t layerCount, uint32_t mipLevel, VkFormat format, VkImage image);
void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

bool hasStencilComponent(VkFormat format);
void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t layerCount);
void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t layerCount);
VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

void CreateSolidColodImage(glm::vec4 color, GfxImage* o_image);