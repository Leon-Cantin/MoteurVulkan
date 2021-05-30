#pragma once
#include "vk_globals.h"

struct BufferAllocator
{
	const R_HW::GpuBuffer* buffer;
	size_t head = 0;
};

size_t AllocateGpuBufferSlot( BufferAllocator* allocator, size_t slotSize );