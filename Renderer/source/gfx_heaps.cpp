#include "gfx_heaps.h"

#include <stdexcept>

GfxHeap create_gfx_heap( size_t size, uint32_t memoryTypeBits, VkMemoryPropertyFlags properties )
{
	GfxHeap heap = {};
	heap.memoryTypeIndex = findMemoryType( memoryTypeBits, properties );
	heap.properties = properties; //TODO: this is not the type of the heap, but the type we want. May slightly differ
	heap.gfx_mem_alloc = allocate_gfx_memory( size, heap.memoryTypeIndex );

	return heap;
}

void destroy( GfxHeap* gfxHeap )
{
	destroy_gfx_memory( &gfxHeap->gfx_mem_alloc );
	*gfxHeap = {};
}