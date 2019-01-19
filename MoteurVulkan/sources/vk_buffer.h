#pragma once

#include "vk_globals.h"
#include <array>

struct PerFrameBuffer
{
	std::array<VkBuffer, SIMULTANEOUS_FRAMES> buffers;
	VkDeviceMemory memory;
	VkDeviceSize stride;
};

void create_buffer(VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryProperties, VkBuffer& buffer, VkDeviceMemory& deviceMemory);
void copy_buffer(VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size);
void copy_buffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size);
void createBufferToDeviceLocalMemory(VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, VkBuffer* o_buffer, VkDeviceMemory* o_VkDeviceMemory);
void createBufferToDeviceLocalMemory(const void* data, VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, VkBuffer* o_buffer, VkDeviceMemory* o_VkDeviceMemory);
void copyDataToDeviceLocalMemory(VkBuffer dstBuffer, const void* data, VkDeviceSize bufferSize);
void copyDataToDeviceLocalMemory(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, const void* data, VkDeviceSize bufferSize);
void UpdateBuffer(VkDeviceMemory dstMemory, const void* src, VkDeviceSize size);

void CreatePerFrameBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryProperties, PerFrameBuffer * o_buffer);
void UpdatePerFrameBuffer(const PerFrameBuffer * buffer, const void* src, VkDeviceSize size, uint32_t frame);
VkDeviceSize GetMemoryOffsetForFrame(const PerFrameBuffer * buffer, uint32_t frame);
void DestroyPerFrameBuffer(PerFrameBuffer * o_buffer);