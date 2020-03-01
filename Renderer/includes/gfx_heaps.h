#pragma once
#include "vk_memory.h"
#include <stdexcept>

struct GfxHeap
{
	size_t size;
	uint32_t memoryTypeIndex;
	VkMemoryPropertyFlags properties;
	VkDeviceMemory memory;
};

GfxHeap create_gfx_heap( size_t size, uint32_t memoryTypeBits, VkMemoryPropertyFlags properties )
{
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = size;
	allocInfo.memoryTypeIndex = findMemoryType( memoryTypeBits, properties );

	GfxHeap heap = {};
	heap.size = size;
	heap.memoryTypeIndex = allocInfo.memoryTypeIndex;
	heap.properties = properties; //TODO: this is not the type of the heap, but the type we want. May slightly differ

	if( vkAllocateMemory( g_vk.device, &allocInfo, nullptr, &heap.memory ) != VK_SUCCESS )
		throw std::runtime_error( "failed to allocate image memory!" );
}
