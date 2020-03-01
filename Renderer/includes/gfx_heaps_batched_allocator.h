#pragma once

#include "gfx_heaps.h"
#include <cassert>

class GfxHeaps_BatchedAllocator
{
public:
	GfxHeaps_BatchedAllocator( GfxHeap* heap );
	bool Allocate( VkImage image );
private:
	GfxHeap* _heap;
	size_t head = 0;
};

GfxHeaps_BatchedAllocator::GfxHeaps_BatchedAllocator( GfxHeap* heap )
	:_heap( heap )
{

}

bool GfxHeaps_BatchedAllocator::Allocate( VkImage image )
{
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements( g_vk.device, image, &memRequirements );

	assert( IsRequiredMemoryType( memRequirements.memoryTypeBits, _heap->memoryTypeIndex ) );

	const size_t alignementOffset = memRequirements.alignment + ( head % memRequirements.alignment );
	const size_t memOffset = head + alignementOffset;
	const size_t newHead = memOffset + memRequirements.size;

	vkBindImageMemory( g_vk.device, image, _heap->memory, memOffset );

	head = newHead;
}

