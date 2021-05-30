#pragma once

#include "vk_globals.h"
#include "gfx_image.h"
#include "allocators.h"

class GfxHeaps_Allocator : public R_HW::I_BufferAllocator
{
public:
	GfxHeaps_Allocator();
	GfxHeaps_Allocator( R_HW::GfxHeap* heap );

	//Buffer allocator
	bool Allocate( R_HW::GfxApiBuffer buffer, R_HW::GfxMemAlloc* o_gfx_mem_alloc );
	bool UploadData( const R_HW::GpuBuffer& buffer, const void* data );

private:
	R_HW::GfxHeap* _heap;
	size_t head;
};


class GfxHeaps_BatchedAllocator : public I_ImageAlloctor, public R_HW::I_BufferAllocator
{
public:
	GfxHeaps_BatchedAllocator();
	GfxHeaps_BatchedAllocator( R_HW::GfxHeap* heap );
	void Prepare();
	void Commit();

	//Image allocator
	bool Allocate( R_HW::GfxApiImage image, R_HW::GfxMemAlloc* o_gfx_mem_alloc );
	bool UploadData( const R_HW::GfxImage& image, const void* data );

	//Buffer allocator
	bool Allocate( R_HW::GfxApiBuffer buffer, R_HW::GfxMemAlloc* o_gfx_mem_alloc );
	bool UploadData( const R_HW::GpuBuffer& buffer, const void* data );

private:
	R_HW::GfxHeap* _heap;
	size_t head;
	BufferAllocator stagingBufferAllocator;
	R_HW::GpuBuffer stagingBuffer;
	R_HW::GfxCommandBuffer commandBuffer;
};


class GfxHeaps_CommitedResourceAllocator : public I_ImageAlloctor
{
public:
	void Prepare();
	void Commit();

	//Image allocator
	bool Allocate( R_HW::GfxApiImage image, R_HW::GfxMemAlloc* o_gfx_mem_alloc );
	bool UploadData( const R_HW::GfxImage& image, const void* data );

private:
	BufferAllocator stagingBufferAllocator;
	R_HW::GpuBuffer stagingBuffer;
	R_HW::GfxCommandBuffer commandBuffer;
};
