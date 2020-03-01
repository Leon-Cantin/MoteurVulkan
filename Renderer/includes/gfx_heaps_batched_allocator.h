#pragma once

#include "gfx_heaps.h"
#include "gfx_image.h"
#include "vk_image.h"
#include "vk_buffer.h"
#include "allocators.h"
#include "vk_commands.h"
#include <cassert>

class GfxHeaps_BatchedAllocator : public I_ImageAlloctor
{
public:
	GfxHeaps_BatchedAllocator();
	GfxHeaps_BatchedAllocator( GfxHeap* heap );
	void Prepare();
	void Commit();

	bool Allocate( VkImage image );
	bool UploadData( const GfxImage& image, void* data );
private:
	GfxHeap* _heap;
	size_t head;
	BufferAllocator stagingBufferAllocator;
	GpuBuffer stagingBuffer;
	VkCommandBuffer commandBuffer;
};

GfxHeaps_BatchedAllocator::GfxHeaps_BatchedAllocator( GfxHeap* heap )
	:_heap( heap ), head(0), stagingBuffer(), commandBuffer(VK_NULL_HANDLE), stagingBufferAllocator()
{

}

GfxHeaps_BatchedAllocator::GfxHeaps_BatchedAllocator()
	: GfxHeaps_BatchedAllocator( nullptr )
{

}

void GfxHeaps_BatchedAllocator::Prepare()
{
	CreateCommitedGpuBuffer( 16*1024*1024, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer );
	commandBuffer = beginSingleTimeCommands();
	stagingBufferAllocator = {};
	stagingBufferAllocator.buffer = &stagingBuffer;
}

void GfxHeaps_BatchedAllocator::Commit()
{
	endSingleTimeCommands( commandBuffer );
	DestroyCommitedGpuBuffer( &stagingBuffer );
}

bool GfxHeaps_BatchedAllocator::Allocate( VkImage image )
{
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements( g_vk.device, image, &memRequirements );

	assert( IsRequiredMemoryType( memRequirements.memoryTypeBits, _heap->memoryTypeIndex ) );

	const size_t alignementOffset = memRequirements.alignment - ( head % memRequirements.alignment );
	const size_t memOffset = head + alignementOffset;
	const size_t newHead = memOffset + memRequirements.size;

	vkBindImageMemory( g_vk.device, image, _heap->memory, memOffset );

	head = newHead;

	return true;
}

bool GfxHeaps_BatchedAllocator::UploadData( const GfxImage& gfxImage, void* data )
{
	assert( gfxImage.format == VK_FORMAT_R8G8B8A8_UNORM ); //assuming 32bits per pixel
	const VkDeviceSize memorySize = gfxImage.extent.width * gfxImage.extent.height * 4;
	const uint32_t layerCount = 1;

	VkDeviceSize stagingBufferOffset = AllocateGpuBufferSlot( &stagingBufferAllocator, memorySize );
	UpdateGpuBuffer( &stagingBuffer, data, memorySize, stagingBufferOffset );

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

