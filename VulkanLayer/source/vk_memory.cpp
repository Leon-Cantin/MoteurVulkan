#include "vk_memory.h"

#include <stdexcept>
#include <assert.h>

GfxMemAlloc allocate_gfx_memory( VkDeviceSize size, uint32_t type )
{
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = size;
	allocInfo.memoryTypeIndex = type;

	VkDeviceMemory memory;
	if( vkAllocateMemory( g_vk.device, &allocInfo, nullptr, &memory ) != VK_SUCCESS )
		throw std::runtime_error( "failed to allocate buffer memory!" );

	const VkDeviceSize offset = 0;
	const bool is_parent_pool = true;
	return { memory, offset, size, is_parent_pool };
}

GfxMemAlloc suballocate_gfx_memory( const GfxMemAlloc& gfx_mem, VkDeviceSize size, VkDeviceSize offset )
{
	assert( offset + size <= gfx_mem.size );
	assert( gfx_mem.offset == 0 );//Just to be sure right now
	const bool is_parent_pool = false;
	const GfxMemAlloc sub_alloc { gfx_mem.memory, gfx_mem.offset + offset, size, is_parent_pool };
	return sub_alloc;
}

void destroy_gfx_memory( GfxMemAlloc* gfx_mem )
{
	if( gfx_mem->is_parent_pool )
		vkFreeMemory( g_vk.device, gfx_mem->memory, nullptr );
}

void UpdateGpuMemory( const GfxMemAlloc* dstMemory, const void* src, VkDeviceSize size, VkDeviceSize offset )
{
	assert( offset + size <= dstMemory->size );
	void* dst;
	vkMapMemory( g_vk.device, dstMemory->memory, dstMemory->offset + offset, size, 0, &dst );
	memcpy( dst, src, size );
	vkUnmapMemory( g_vk.device, dstMemory->memory );
}

bool IsRequiredMemoryType( uint32_t typeFilter, uint32_t memoryType )
{
	uint32_t memoryTypeBit = 1 << memoryType;
	return (typeFilter & memoryTypeBit);
}

uint32_t findMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties )
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(g_vk.physicalDevice, &memProperties);

	for (uint32_t memoryType = 0; memoryType < memProperties.memoryTypeCount; memoryType++) {
		if ( IsRequiredMemoryType(typeFilter, memoryType) && (memProperties.memoryTypes[memoryType].propertyFlags & properties) == properties) {
			return memoryType;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}
