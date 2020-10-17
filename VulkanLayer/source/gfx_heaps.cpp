#include "gfx_heaps.h"

#include <stdexcept>

GfxHeap create_gfx_heap( GfxDeviceSize size, GfxMemoryPropertyFlags properties )
{
	const uint32_t memoryTypeBits = 0xFFFFFFFF; //TODO: For now let's just allow it on all types as long as the properties match
	GfxHeap heap = {};
	heap.memoryTypeIndex = findMemoryType( memoryTypeBits, properties );
	heap.properties = properties; //TODOTODO: this is not the type of the heap, but the type we want. May slightly differ
	heap.gfx_mem_alloc = allocate_gfx_memory( size, heap.memoryTypeIndex );

	return heap;
}

void destroy( GfxHeap* gfxHeap )
{
	destroy_gfx_memory( &gfxHeap->gfx_mem_alloc );
	*gfxHeap = {};
}