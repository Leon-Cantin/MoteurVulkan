#pragma once
#include "vk_globals.h"
#include "vk_memory.h"

#include <vector>

void create_image( uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageUsageFlags usage, VkImage* image );
void create_cube_image( uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageUsageFlags usage, VkImage* image );
VkImageView create_image_view( VkImage image, VkImageViewType type, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels );

void BindMemory( VkImage image, VkDeviceMemory memory );
void BindMemory( VkImage image, const GfxMemAlloc& gfx_mem_alloc );

void copyBufferToImage( VkCommandBuffer commandBuffer, VkBuffer buffer, uint32_t bufferOffset, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount );

bool hasStencilComponent(VkFormat format);
void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t layerCount);
VkFormat findDepthFormat();
VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);