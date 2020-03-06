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
	return { memory, offset, size };
}

GfxMemAlloc suballocate_gfx_memory( const GfxMemAlloc& gfx_mem, VkDeviceSize size, VkDeviceSize offset )
{
	//TODO: respect alignement!!!!!
	assert( offset + size <= gfx_mem.size );
	const GfxMemAlloc sub_alloc { gfx_mem.memory, offset, size };
	return sub_alloc;
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

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
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
