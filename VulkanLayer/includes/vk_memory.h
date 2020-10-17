#pragma once

#include "vk_globals.h"

struct GfxMemAlloc
{
	VkDeviceMemory memory;
	GfxDeviceSize offset;
	GfxDeviceSize size;
	bool is_parent_pool;
};

GfxMemAlloc allocate_gfx_memory( GfxDeviceSize size, GfxMemoryType type );
GfxMemAlloc suballocate_gfx_memory( const GfxMemAlloc& gfx_mem, GfxDeviceSize size, GfxDeviceSize offset );
void destroy_gfx_memory( GfxMemAlloc* gfx_mem );
void UpdateGpuMemory( const GfxMemAlloc* dstMemory, const void* src, GfxDeviceSize size, GfxDeviceSize offset );

inline bool IsValid( const GfxMemAlloc& mem_alloc )
{
	return mem_alloc.memory != VK_NULL_HANDLE;
}

//TODO: Find where this goes in the Vulkan layer. public or not?
bool IsRequiredMemoryType( GfxMemoryTypeFilter typeFilter, GfxMemoryType memoryType );
GfxMemoryType findMemoryType( GfxMemoryTypeFilter typeFilter, GfxMemoryPropertyFlags properties );
