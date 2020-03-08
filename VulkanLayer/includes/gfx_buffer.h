#pragma once

#include "vk_buffer.h"
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
	virtual bool Allocate( VkBuffer buffer, GfxMemAlloc* o_gfx_mem_alloc ) = 0;
	virtual bool UploadData( const GpuBuffer& buffer, const void* data ) = 0;
};

void CreateCommitedGpuBuffer( VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryProperties, GpuBuffer* o_buffer );
void DestroyCommitedGpuBuffer( GpuBuffer* buffer );

void UpdateGpuBuffer( const GpuBuffer* buffer, const void* src, VkDeviceSize size, VkDeviceSize offset );

void CreatePerFrameBuffer( VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryProperties, PerFrameBuffer * o_buffer );
void UpdatePerFrameBuffer( const PerFrameBuffer * buffer, const void* src, VkDeviceSize size, uint32_t frame );
void DestroyPerFrameBuffer( PerFrameBuffer * o_buffer );
