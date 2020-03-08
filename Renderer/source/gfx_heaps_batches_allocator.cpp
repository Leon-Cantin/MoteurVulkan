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
	CreateCommitedGpuBuffer( 16 * 1024 * 1024, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer );
	commandBuffer = beginSingleTimeCommands();
	stagingBufferAllocator = {};
	stagingBufferAllocator.buffer = &stagingBuffer;
}

void GfxHeaps_BatchedAllocator::Commit()
{
	endSingleTimeCommands( commandBuffer );
	DestroyCommitedGpuBuffer( &stagingBuffer );
}

bool GfxHeaps_BatchedAllocator::Allocate( VkImage image, GfxMemAlloc* o_gfx_mem_alloc )
{
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements( g_vk.device, image, &memRequirements );

	assert( IsRequiredMemoryType( memRequirements.memoryTypeBits, _heap->memoryTypeIndex ) );

	const size_t alignementOffset = memRequirements.alignment - (head % memRequirements.alignment);
	const size_t memOffset = head + alignementOffset;
	const size_t newHead = memOffset + memRequirements.size;

	*o_gfx_mem_alloc = suballocate_gfx_memory( _heap->gfx_mem_alloc, memRequirements.size, memOffset );
	BindMemory( image, *o_gfx_mem_alloc );

	head = newHead;

	return true;
}

bool GfxHeaps_BatchedAllocator::UploadData( const GfxImage& gfxImage, const void* data )
{
	const uint32_t layerCount = 1;

	VkDeviceSize stagingBufferOffset = AllocateGpuBufferSlot( &stagingBufferAllocator, gfxImage.gfx_mem_alloc.size );
	UpdateGpuBuffer( &stagingBuffer, data, gfxImage.gfx_mem_alloc.size, stagingBufferOffset );

	transitionImageLayout( commandBuffer, gfxImage.image, gfxImage.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, gfxImage.mipLevels, layerCount );
	copyBufferToImage( commandBuffer, stagingBuffer.buffer, stagingBufferOffset, gfxImage.image, gfxImage.extent.width, gfxImage.extent.height, layerCount );

	//TODO: mem requirement will be computed by memRequirement in create_image... I hope it's right
	//GenerateMipmaps will do the transition, do it if we don't generate them
	if( gfxImage.mipLevels > 1 )
		generateMipmaps( commandBuffer, gfxImage.image, gfxImage.format, gfxImage.extent.width, gfxImage.extent.height, gfxImage.mipLevels );//TODO: should probably not be done here
	else
		transitionImageLayout( commandBuffer, gfxImage.image, gfxImage.format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, gfxImage.mipLevels, layerCount );

	return true;
}

bool GfxHeaps_BatchedAllocator::Allocate( VkBuffer buffer )
{
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements( g_vk.device, buffer, &memRequirements );

	assert( IsRequiredMemoryType( memRequirements.memoryTypeBits, _heap->memoryTypeIndex ) );

	const size_t alignementOffset = memRequirements.alignment - (head % memRequirements.alignment);
	const size_t memOffset = head + alignementOffset;
	const size_t newHead = memOffset + memRequirements.size;

	vkBindBufferMemory( g_vk.device, buffer, _heap->gfx_mem_alloc.memory, memOffset );

	head = newHead;

	return true;
}

bool GfxHeaps_BatchedAllocator::UploadData( const GpuBuffer& buffer, const void* data )
{
	VkDeviceSize memorySize = buffer.gpuMemory.size;
	VkDeviceSize stagingBufferOffset = AllocateGpuBufferSlot( &stagingBufferAllocator, memorySize );
	UpdateGpuBuffer( &stagingBuffer, data, memorySize, stagingBufferOffset );
	copy_buffer( commandBuffer, buffer.buffer, stagingBuffer.buffer, 0, stagingBufferOffset, memorySize );

	return true;
}

void GfxHeaps_CommitedResourceAllocator::Prepare()
{
	assert( commandBuffer == VK_NULL_HANDLE );
	CreateCommitedGpuBuffer( 16 * 1024 * 1024, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer );
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

bool GfxHeaps_CommitedResourceAllocator::Allocate( VkImage image, GfxMemAlloc* o_gfx_mem_alloc )
{
	VkMemoryRequirements mem_requirements;
	vkGetImageMemoryRequirements( g_vk.device, image, &mem_requirements );
	uint32_t memoryType = findMemoryType( mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

	*o_gfx_mem_alloc = allocate_gfx_memory( mem_requirements.size, memoryType );
	BindMemory( image, *o_gfx_mem_alloc );

	return true;
}

//TODO: bunch of stuff shared with above allocator
bool GfxHeaps_CommitedResourceAllocator::UploadData( const GfxImage& gfxImage, const void* data )
{
	const uint32_t layerCount = 1;

	VkDeviceSize stagingBufferOffset = AllocateGpuBufferSlot( &stagingBufferAllocator, gfxImage.gfx_mem_alloc.size );
	UpdateGpuBuffer( &stagingBuffer, data, gfxImage.gfx_mem_alloc.size, stagingBufferOffset );

	transitionImageLayout( commandBuffer, gfxImage.image, gfxImage.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, gfxImage.mipLevels, layerCount );
	copyBufferToImage( commandBuffer, stagingBuffer.buffer, stagingBufferOffset, gfxImage.image, gfxImage.extent.width, gfxImage.extent.height, layerCount );

	//TODO: mem requirement will be computed by memRequirement in create_image... I hope it's right
	//GenerateMipmaps will do the transition, do it if we don't generate them
	if( gfxImage.mipLevels > 1 )
		generateMipmaps( commandBuffer, gfxImage.image, gfxImage.format, gfxImage.extent.width, gfxImage.extent.height, gfxImage.mipLevels );//TODO: should probably not be done here
	else
		transitionImageLayout( commandBuffer, gfxImage.image, gfxImage.format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, gfxImage.mipLevels, layerCount );

	return true;
}
