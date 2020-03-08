#pragma once

#include "vk_globals.h"
#include "vk_memory.h"

VkBuffer create_buffer( VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags );
void BindMemory( VkBuffer buffer, const GfxMemAlloc& gfx_mem_alloc );

void copy_buffer( VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize dst_offset, VkDeviceSize src_offset, VkDeviceSize size );
void copy_buffer( VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size );