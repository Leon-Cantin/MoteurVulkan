#pragma once

#include "vk_buffer.h"
#include <array>

struct GpuBuffer
{
	GfxApiBuffer buffer;
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
	virtual bool Allocate( GfxApiBuffer buffer, GfxMemAlloc* o_gfx_mem_alloc ) = 0;
	virtual bool UploadData( const GpuBuffer& buffer, const void* data ) = 0;
};

void CreateCommitedGpuBuffer( GfxDeviceSize size, GfxBufferUsageFlags bufferUsageFlags, GfxMemoryPropertyFlags memoryProperties, GpuBuffer* o_buffer );
void DestroyCommitedGpuBuffer( GpuBuffer* buffer );

void UpdateGpuBuffer( const GpuBuffer* buffer, const void* src, GfxDeviceSize size, GfxDeviceSize offset );

void CreatePerFrameBuffer( GfxDeviceSize size, GfxBufferUsageFlags bufferUsageFlags, GfxMemoryPropertyFlags memoryProperties, PerFrameBuffer * o_buffer );
void UpdatePerFrameBuffer( const PerFrameBuffer * buffer, const void* src, GfxDeviceSize size, uint32_t frame );
void DestroyPerFrameBuffer( PerFrameBuffer * o_buffer );
