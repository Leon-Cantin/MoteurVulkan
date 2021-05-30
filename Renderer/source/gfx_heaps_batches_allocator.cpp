#include "gfx_heaps_batched_allocator.h"

#include <cassert>

GfxHeaps_Allocator::GfxHeaps_Allocator( R_HW::GfxHeap* heap )
	:_heap( heap ), head( 0 )
{

}

GfxHeaps_Allocator::GfxHeaps_Allocator()
	: GfxHeaps_Allocator( nullptr )
{

}

bool GfxHeaps_Allocator::Allocate( R_HW::GfxApiBuffer buffer, R_HW::GfxMemAlloc* o_gfx_mem_alloc )
{
	R_HW::GfxMemoryRequirements memRequirements = R_HW::GetBufferMemoryRequirement( buffer );

	assert( R_HW::IsRequiredMemoryType( memRequirements.memoryTypeBits, _heap->memoryTypeIndex ) );

	const R_HW::GfxDeviceSize alignment = R_HW::GetAlignment( memRequirements );
	const R_HW::GfxDeviceSize size = R_HW::GetSize( memRequirements );
	const size_t alignementOffset = alignment - (head % alignment);
	const size_t memOffset = head + alignementOffset;
	const size_t newHead = memOffset + size;

	*o_gfx_mem_alloc = suballocate_gfx_memory( _heap->gfx_mem_alloc, size, memOffset );
	BindMemory( buffer, *o_gfx_mem_alloc );

	head = newHead;

	return true;
}

bool GfxHeaps_Allocator::UploadData( const R_HW::GpuBuffer& buffer, const void* data )
{
	R_HW::GfxDeviceSize memorySize = buffer.gpuMemory.size;
	UpdateGpuBuffer( &buffer, data, memorySize, 0 );

	return true;
}

// ********************** Batched allocator *************************

GfxHeaps_BatchedAllocator::GfxHeaps_BatchedAllocator( R_HW::GfxHeap* heap )
	:_heap( heap ), head( 0 ), stagingBuffer(), commandBuffer( VK_NULL_HANDLE ), stagingBufferAllocator()
{

}

GfxHeaps_BatchedAllocator::GfxHeaps_BatchedAllocator()
	: GfxHeaps_BatchedAllocator( nullptr )
{

}

void GfxHeaps_BatchedAllocator::Prepare()
{
	CreateCommitedGpuBuffer( 16 * 1024 * 1024, R_HW::GFX_BUFFER_USAGE_TRANSFER_SRC_BIT, R_HW::GFX_MEMORY_PROPERTY_HOST_VISIBLE_BIT | R_HW::GFX_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer );
	commandBuffer = R_HW::beginSingleTimeCommands();
	stagingBufferAllocator = {};
	stagingBufferAllocator.buffer = &stagingBuffer;
}

void GfxHeaps_BatchedAllocator::Commit()
{
	R_HW::endSingleTimeCommands( commandBuffer );
	Destroy( &stagingBuffer );
}

bool GfxHeaps_BatchedAllocator::Allocate( R_HW::GfxApiImage image, R_HW::GfxMemAlloc* o_gfx_mem_alloc )
{
	R_HW::GfxMemoryRequirements memRequirements = R_HW::GetImageMemoryRequirement( image );

	assert( R_HW::IsRequiredMemoryType( memRequirements.memoryTypeBits, _heap->memoryTypeIndex ) );

	const R_HW::GfxDeviceSize alignment = R_HW::GetAlignment( memRequirements );
	const R_HW::GfxDeviceSize size = R_HW::GetSize( memRequirements );
	const size_t alignementOffset = alignment - (head % alignment);
	const size_t memOffset = head + alignementOffset;
	const size_t newHead = memOffset + size;

	*o_gfx_mem_alloc = suballocate_gfx_memory( _heap->gfx_mem_alloc, size, memOffset );
	BindMemory( image, *o_gfx_mem_alloc );

	head = newHead;

	return true;
}

static bool UploadDataCommon( R_HW::GfxCommandBuffer commandBuffer, const R_HW::GfxImage& gfxImage, const void* data, BufferAllocator* stagingBufferAllocator, R_HW::GpuBuffer* stagingBuffer )
{
	R_HW::GfxDeviceSize stagingBufferOffset = AllocateGpuBufferSlot( stagingBufferAllocator, gfxImage.gfx_mem_alloc.size );
	R_HW::UpdateGpuBuffer( stagingBuffer, data, gfxImage.gfx_mem_alloc.size, stagingBufferOffset );

	R_HW::GfxImageBarrier( commandBuffer, gfxImage.image, R_HW::GfxLayout::UNDEFINED, R_HW::GfxAccess::WRITE, R_HW::GfxLayout::TRANSFER, R_HW::GfxAccess::WRITE );

	R_HW::copyBufferToImage( commandBuffer, stagingBuffer->buffer, stagingBufferOffset, gfxImage.image, gfxImage.extent.width, gfxImage.extent.height, gfxImage.layers );

	//GenerateMipmaps will do the transition, do it if we don't generate them
	if( gfxImage.mipLevels > 1 )
		generateMipmaps( commandBuffer, gfxImage.image, gfxImage.format, gfxImage.extent.width, gfxImage.extent.height, gfxImage.mipLevels );//TODO: should probably not be done here
	else
		R_HW::GfxImageBarrier( commandBuffer, gfxImage.image, R_HW::GfxLayout::TRANSFER, R_HW::GfxAccess::WRITE, R_HW::GfxLayout::COLOR, R_HW::GfxAccess::READ );

	return true;
}

bool GfxHeaps_BatchedAllocator::UploadData( const R_HW::GfxImage& gfxImage, const void* data )
{
	return UploadDataCommon( commandBuffer, gfxImage, data, &stagingBufferAllocator, &stagingBuffer );
}

bool GfxHeaps_BatchedAllocator::Allocate( R_HW::GfxApiBuffer buffer, R_HW::GfxMemAlloc* o_gfx_mem_alloc )
{
	R_HW::GfxMemoryRequirements memRequirements = R_HW::GetBufferMemoryRequirement( buffer );

	assert( R_HW::IsRequiredMemoryType( memRequirements.memoryTypeBits, _heap->memoryTypeIndex ) );

	const R_HW::GfxDeviceSize alignment = R_HW::GetAlignment( memRequirements );
	const R_HW::GfxDeviceSize size = R_HW::GetSize( memRequirements );
	const size_t alignementOffset = alignment - (head % alignment);
	const size_t memOffset = head + alignementOffset;
	const size_t newHead = memOffset + size;

	*o_gfx_mem_alloc = suballocate_gfx_memory( _heap->gfx_mem_alloc, size, memOffset );
	BindMemory( buffer, *o_gfx_mem_alloc );

	head = newHead;

	return true;
}

bool GfxHeaps_BatchedAllocator::UploadData( const R_HW::GpuBuffer& buffer, const void* data )
{
	R_HW::GfxDeviceSize memorySize = buffer.gpuMemory.size;
	R_HW::GfxDeviceSize stagingBufferOffset = AllocateGpuBufferSlot( &stagingBufferAllocator, memorySize );
	UpdateGpuBuffer( &stagingBuffer, data, memorySize, stagingBufferOffset );
	R_HW::copy_buffer( commandBuffer, buffer.buffer, stagingBuffer.buffer, 0, stagingBufferOffset, memorySize );

	return true;
}

void GfxHeaps_CommitedResourceAllocator::Prepare()
{
	assert( commandBuffer == VK_NULL_HANDLE );
	CreateCommitedGpuBuffer( 16 * 1024 * 1024, R_HW::GFX_BUFFER_USAGE_TRANSFER_SRC_BIT, R_HW::GFX_MEMORY_PROPERTY_HOST_VISIBLE_BIT | R_HW::GFX_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer );
	commandBuffer = R_HW::beginSingleTimeCommands();
	stagingBufferAllocator = {};
	stagingBufferAllocator.buffer = &stagingBuffer;
}

void GfxHeaps_CommitedResourceAllocator::Commit()
{
	R_HW::endSingleTimeCommands( commandBuffer );
	commandBuffer = VK_NULL_HANDLE;
	Destroy( &stagingBuffer );
}

bool GfxHeaps_CommitedResourceAllocator::Allocate( R_HW::GfxApiImage image, R_HW::GfxMemAlloc* o_gfx_mem_alloc )
{
	R_HW::GfxMemoryRequirements mem_requirements = R_HW::GetImageMemoryRequirement( image );

	const R_HW::GfxDeviceSize alignment = R_HW::GetAlignment( mem_requirements );
	const R_HW::GfxDeviceSize size = R_HW::GetSize( mem_requirements );
	const R_HW::GfxMemoryTypeFilter memoryTypeFilter = R_HW::GetMemoryTypeFilter( mem_requirements );

	const R_HW::GfxMemoryType memoryType = R_HW::findMemoryType( memoryTypeFilter, R_HW::GFX_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

	*o_gfx_mem_alloc = R_HW::allocate_gfx_memory( size, memoryType );
	BindMemory( image, *o_gfx_mem_alloc );

	return true;
}

bool GfxHeaps_CommitedResourceAllocator::UploadData( const R_HW::GfxImage& gfxImage, const void* data )
{
	return UploadDataCommon( commandBuffer, gfxImage, data, &stagingBufferAllocator, &stagingBuffer );
}
