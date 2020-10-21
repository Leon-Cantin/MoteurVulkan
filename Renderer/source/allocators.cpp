#include "allocators.h"

/*
Avoir un objet descripteur lié à un buffer sur lequel on aloue de la mémoire.
Buffer Dynamique on peut alouer plusieurs objet et non dynamique seulement 1?
*/
size_t AllocateGpuBufferSlot( BufferAllocator* allocator, size_t slotSize )
{
	//TODO: have buffers that you can only allocate in certain chuncks to simplify?
	//TODO: allignment
	size_t allocationOffset = allocator->head;
	size_t newHead = allocationOffset + slotSize;
	assert( newHead <= allocator->buffer->gpuMemory.size );
	allocator->head = newHead;
	return allocationOffset;
}

void Destroy( GfxDescriptorPool* descriptorPool )
{
	vkDestroyDescriptorPool( g_gfx.device.device, *descriptorPool, nullptr );
	*descriptorPool = VK_NULL_HANDLE
}