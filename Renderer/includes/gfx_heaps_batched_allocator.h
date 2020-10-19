#pragma once

#include "gfx_heaps.h"
#include "gfx_image.h"
#include "vk_image.h"
#include "vk_buffer.h"
#include "allocators.h"

class GfxHeaps_BatchedAllocator : public I_ImageAlloctor, public I_BufferAllocator
{
public:
	GfxHeaps_BatchedAllocator();
	GfxHeaps_BatchedAllocator( GfxHeap* heap );
	void Prepare();
	void Commit();

	//Image allocator
	bool Allocate( GfxApiImage image, GfxMemAlloc* o_gfx_mem_alloc );
	bool UploadData( const GfxImage& image, const void* data );

	//Buffer allocator
	bool Allocate( GfxApiBuffer buffer, GfxMemAlloc* o_gfx_mem_alloc );
	bool UploadData( const GpuBuffer& buffer, const void* data );

private:
	GfxHeap* _heap;
	size_t head;
	BufferAllocator stagingBufferAllocator;
	GpuBuffer stagingBuffer;
	GfxCommandBuffer commandBuffer;
};


class GfxHeaps_CommitedResourceAllocator : public I_ImageAlloctor
{
public:
	void Prepare();
	void Commit();

	//Image allocator
	bool Allocate( GfxApiImage image, GfxMemAlloc* o_gfx_mem_alloc );
	bool UploadData( const GfxImage& image, const void* data );

private:
	BufferAllocator stagingBufferAllocator;
	GpuBuffer stagingBuffer;
	GfxCommandBuffer commandBuffer;
};
