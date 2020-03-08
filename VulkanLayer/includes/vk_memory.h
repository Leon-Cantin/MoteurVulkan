#pragma once

#include "vk_globals.h"

struct GfxMemAlloc
{
	VkDeviceMemory memory;
	VkDeviceSize offset;
	VkDeviceSize size;
	bool is_parent_pool;
};

GfxMemAlloc allocate_gfx_memory( VkDeviceSize size, uint32_t type );
GfxMemAlloc suballocate_gfx_memory( const GfxMemAlloc& gfx_mem, VkDeviceSize size, VkDeviceSize offset );
void destroy_gfx_memory( GfxMemAlloc* gfx_mem );
void UpdateGpuMemory( const GfxMemAlloc* dstMemory, const void* src, VkDeviceSize size, VkDeviceSize offset );

bool IsRequiredMemoryType( uint32_t typeFilter, uint32_t memoryType );
uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);


