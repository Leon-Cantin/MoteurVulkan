#pragma once
#include "vk_memory.h"

//TODO: make it contain a Gfx_Memory
struct GfxHeap
{
	uint32_t memoryTypeIndex;
	VkMemoryPropertyFlags properties;
	GfxMemAlloc gfx_mem_alloc;
};

GfxHeap create_gfx_heap( size_t size, uint32_t memoryTypeBits, VkMemoryPropertyFlags properties );
void destroy( GfxHeap* gfxHeap );
