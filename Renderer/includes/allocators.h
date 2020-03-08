#pragma once
#include <cassert>
#include "gfx_buffer.h"

struct BufferAllocator
{
	const GpuBuffer* buffer;
	size_t head = 0;
};

size_t AllocateGpuBufferSlot( BufferAllocator* allocator, size_t slotSize );