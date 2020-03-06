#pragma once

#include "vk_globals.h"

struct GfxMemAlloc
{
	VkDeviceMemory memory;
	VkDeviceSize offset;
	VkDeviceSize size;
};

GfxMemAlloc allocate_gfx_memory( VkDeviceSize size, uint32_t type );
GfxMemAlloc suballocate_gfx_memory( const GfxMemAlloc& gfx_mem, VkDeviceSize size, VkDeviceSize offset );
void UpdateGpuMemory( const GfxMemAlloc* dstMemory, const void* src, VkDeviceSize size, VkDeviceSize offset );

bool IsRequiredMemoryType( uint32_t typeFilter, uint32_t memoryType );
uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);


