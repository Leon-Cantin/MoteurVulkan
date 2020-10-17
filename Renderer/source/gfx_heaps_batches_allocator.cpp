#include "gfx_heaps_batched_allocator.h"

#include "vk_commands.h"
#include <cassert>

GfxHeaps_BatchedAllocator::GfxHeaps_BatchedAllocator( GfxHeap* heap )
	:_heap( heap ), head( 0 ), stagingBuffer(), commandBuffer( VK_NULL_HANDLE ), stagingBufferAllocator()
{

}

GfxHeaps_BatchedAllocator::GfxHeaps_BatchedAllocator()
	: GfxHeaps_BatchedAllocator( nullptr )
{

}

void GfxHeaps_BatchedAllocator::Prepare()
{
	CreateCommitedGpuBuffer( 16 * 1024 * 1024, GfxBufferUsageFlagBits::TRANSFER_SRC_BUFFER, GfxMemoryPropertyBit::HOST_VISIBLE | GfxMemoryPropertyBit::HOST_COHERENT, &stagingBuffer );
	commandBuffer = beginSingleTimeCommands();
	stagingBufferAllocator = {};
	stagingBufferAllocator.buffer = &stagingBuffer;
}

void GfxHeaps_BatchedAllocator::Commit()
{
	endSingleTimeCommands( commandBuffer );
	DestroyCommitedGpuBuffer( &stagingBuffer );
}

bool GfxHeaps_BatchedAllocator::Allocate( GfxApiImage image, GfxMemAlloc* o_gfx_mem_alloc )
{
	GfxMemoryRequirements memRequirements = GetImageMemoryRequirement( image );

	assert( IsRequiredMemoryType( memRequirements.memoryTypeBits, _heap->memoryTypeIndex ) );

	const GfxDeviceSize alignment = GetAlignment( memRequirements );
	const GfxDeviceSize size = GetSize( memRequirements );
	const size_t alignementOffset = alignment - (head % alignment);
	const size_t memOffset = head + alignementOffset;
	const size_t newHead = memOffset + size;

	*o_gfx_mem_alloc = suballocate_gfx_memory( _heap->gfx_mem_alloc, size, memOffset );
	BindMemory( image, *o_gfx_mem_alloc );

	head = newHead;

	return true;
}

static bool UploadDataCommon( VkCommandBuffer commandBuffer, const GfxImage& gfxImage, const void* data, BufferAllocator* stagingBufferAllocator, GpuBuffer* stagingBuffer )
{
	const uint32_t layerCount = 1;

	GfxDeviceSize stagingBufferOffset = AllocateGpuBufferSlot( stagingBufferAllocator, gfxImage.gfx_mem_alloc.size );
	UpdateGpuBuffer( stagingBuffer, data, gfxImage.gfx_mem_alloc.size, stagingBufferOffset );

	GfxImageBarrier( commandBuffer, gfxImage.image, GfxLayout::UNDEFINED, GfxAccess::WRITE, GfxLayout::TRANSFER, GfxAccess::WRITE );

	copyBufferToImage( commandBuffer, stagingBuffer->buffer, stagingBufferOffset, gfxImage.image, gfxImage.extent.width, gfxImage.extent.height, layerCount );

	//GenerateMipmaps will do the transition, do it if we don't generate them
	if( gfxImage.mipLevels > 1 )
		generateMipmaps( commandBuffer, gfxImage.image, gfxImage.format, gfxImage.extent.width, gfxImage.extent.height, gfxImage.mipLevels );//TODO: should probably not be done here
	else
		GfxImageBarrier( commandBuffer, gfxImage.image, GfxLayout::TRANSFER, GfxAccess::WRITE, GfxLayout::COLOR, GfxAccess::READ );

	return true;
}

bool GfxHeaps_BatchedAllocator::UploadData( const GfxImage& gfxImage, const void* data )
{
	return UploadDataCommon( commandBuffer, gfxImage, data, &stagingBufferAllocator, &stagingBuffer );
}

bool GfxHeaps_BatchedAllocator::Allocate( GfxApiBuffer buffer, GfxMemAlloc* o_gfx_mem_alloc )
{
	GfxMemoryRequirements memRequirements = GetBufferMemoryRequirement( buffer );

	assert( IsRequiredMemoryType( memRequirements.memoryTypeBits, _heap->memoryTypeIndex ) );

	const GfxDeviceSize alignment = GetAlignment( memRequirements );
	const GfxDeviceSize size = GetSize( memRequirements );
	const size_t alignementOffset = alignment - (head % alignment);
	const size_t memOffset = head + alignementOffset;
	const size_t newHead = memOffset + size;

	*o_gfx_mem_alloc = suballocate_gfx_memory( _heap->gfx_mem_alloc, size, memOffset );
	BindMemory( buffer, *o_gfx_mem_alloc );

	head = newHead;

	return true;
}

bool GfxHeaps_BatchedAllocator::UploadData( const GpuBuffer& buffer, const void* data )
{
	GfxDeviceSize memorySize = buffer.gpuMemory.size;
	GfxDeviceSize stagingBufferOffset = AllocateGpuBufferSlot( &stagingBufferAllocator, memorySize );
	UpdateGpuBuffer( &stagingBuffer, data, memorySize, stagingBufferOffset );
	copy_buffer( commandBuffer, buffer.buffer, stagingBuffer.buffer, 0, stagingBufferOffset, memorySize );

	return true;
}

void GfxHeaps_CommitedResourceAllocator::Prepare()
{
	assert( commandBuffer == VK_NULL_HANDLE );
	CreateCommitedGpuBuffer( 16 * 1024 * 1024, GfxBufferUsageFlagBits::TRANSFER_SRC_BUFFER, GfxMemoryPropertyBit::HOST_VISIBLE | GfxMemoryPropertyBit::HOST_COHERENT, &stagingBuffer );
	commandBuffer = beginSingleTimeCommands();
	stagingBufferAllocator = {};
	stagingBufferAllocator.buffer = &stagingBuffer;
}

void GfxHeaps_CommitedResourceAllocator::Commit()
{
	endSingleTimeCommands( commandBuffer );
	commandBuffer = VK_NULL_HANDLE;
	DestroyCommitedGpuBuffer( &stagingBuffer );
}

bool GfxHeaps_CommitedResourceAllocator::Allocate( GfxApiImage image, GfxMemAlloc* o_gfx_mem_alloc )
{
	GfxMemoryRequirements mem_requirements = GetImageMemoryRequirement( image );

	const GfxDeviceSize alignment = GetAlignment( mem_requirements );
	const GfxDeviceSize size = GetSize( mem_requirements );
	const GfxMemoryTypeFilter memoryTypeFilter = GetMemoryTypeFilter( mem_requirements );

	const GfxMemoryType memoryType = findMemoryType( memoryTypeFilter, GfxMemoryPropertyBit::DEVICE_LOCAL );

	*o_gfx_mem_alloc = allocate_gfx_memory( size, memoryType );
	BindMemory( image, *o_gfx_mem_alloc );

	return true;
}

bool GfxHeaps_CommitedResourceAllocator::UploadData( const GfxImage& gfxImage, const void* data )
{
	return UploadDataCommon( commandBuffer, gfxImage, data, &stagingBufferAllocator, &stagingBuffer );
}
