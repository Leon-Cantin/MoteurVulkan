#pragma once

#include "vk_globals.h"
#include "vk_memory.h"
#include <array>

struct GpuBuffer
{
	VkBuffer buffer;
	GfxMemAlloc gpuMemory;
};

struct PerFrameBuffer
{
	std::array<GpuBuffer, SIMULTANEOUS_FRAMES> buffers;
	GfxMemAlloc gfx_mem_alloc;
};

class I_BufferAllocator
{
public:
	virtual bool Allocate( VkBuffer buffer ) = 0;
	virtual bool UploadData( const GpuBuffer& buffer, const void* data ) = 0;
};

VkBuffer create_buffer( VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags );
void CreateCommitedGpuBuffer( VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryProperties, GpuBuffer* o_buffer );
void DestroyCommitedGpuBuffer( GpuBuffer* buffer );

void copy_buffer( VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize dst_offset, VkDeviceSize src_offset, VkDeviceSize size );
void copy_buffer( VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size );

void UpdateGpuBuffer( const GpuBuffer* buffer, const void* src, VkDeviceSize size, VkDeviceSize offset );

void CreatePerFrameBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryProperties, PerFrameBuffer * o_buffer);
void UpdatePerFrameBuffer(const PerFrameBuffer * buffer, const void* src, VkDeviceSize size, uint32_t frame);
void DestroyPerFrameBuffer(PerFrameBuffer * o_buffer);