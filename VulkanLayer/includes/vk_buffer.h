#pragma once

#include "vk_globals.h"
#include <array>

struct GpuMemory
{
	VkDeviceMemory memory;
	VkDeviceSize offset;
	VkDeviceSize size;
};

struct GpuBuffer
{
	VkBuffer buffer;
	GpuMemory gpuMemory;
};

struct PerFrameBuffer
{
	std::array<GpuBuffer, SIMULTANEOUS_FRAMES> buffers;
	VkDeviceMemory memory;
	VkDeviceSize stride;
};

void CreateCommitedGpuBuffer( VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryProperties, GpuBuffer* o_buffer );
void CopyBufferImmediate(VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size);
void copy_buffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size);
void createBufferToDeviceLocalMemory(VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, VkBuffer* o_buffer, VkDeviceMemory* o_VkDeviceMemory);
void createBufferToDeviceLocalMemory(const void* data, VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, VkBuffer* o_buffer, VkDeviceMemory* o_VkDeviceMemory);
void copyDataToDeviceLocalMemoryImmediate(VkBuffer dstBuffer, const void* data, VkDeviceSize bufferSize);
void copyDataToDeviceLocalMemory(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, const void* data, VkDeviceSize bufferSize);
void DestroyCommitedGpuBuffer( GpuBuffer* buffer );

void UpdateGpuBuffer( const GpuBuffer* buffer, const void* src, VkDeviceSize size, VkDeviceSize offset );
void CreatePerFrameBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryProperties, PerFrameBuffer * o_buffer);
void UpdatePerFrameBuffer(const PerFrameBuffer * buffer, const void* src, VkDeviceSize size, uint32_t frame);
void DestroyPerFrameBuffer(PerFrameBuffer * o_buffer);