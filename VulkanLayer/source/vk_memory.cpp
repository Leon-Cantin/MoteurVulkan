#include "vk_memory.h"

#include <stdexcept>
#include <assert.h>
#include <cstring>

GfxMemAlloc allocate_gfx_memory( GfxDeviceSize size, GfxMemoryType type )
{
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = size;
	allocInfo.memoryTypeIndex = type;

	VkDeviceMemory memory;
	if( vkAllocateMemory( g_vk.device.device, &allocInfo, nullptr, &memory ) != VK_SUCCESS )
		throw std::runtime_error( "failed to allocate buffer memory!" );

	const VkDeviceSize offset = 0;
	const bool is_parent_pool = true;
	return { memory, offset, size, is_parent_pool };
}

GfxMemAlloc suballocate_gfx_memory( const GfxMemAlloc& gfx_mem, GfxDeviceSize size, GfxDeviceSize offset )
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
		vkFreeMemory( g_vk.device.device, gfx_mem->memory, nullptr );
}

void UpdateGpuMemory( const GfxMemAlloc* dstMemory, const void* src, GfxDeviceSize size, GfxDeviceSize offset )
{
	assert( offset + size <= dstMemory->size );
	assert( size );//don't map memory with no size
	void* dst;
	vkMapMemory( g_vk.device.device, dstMemory->memory, dstMemory->offset + offset, size, 0, &dst );
	memcpy( dst, src, size );
	vkUnmapMemory( g_vk.device.device, dstMemory->memory );
}

bool IsRequiredMemoryType( GfxMemoryTypeFilter typeFilter, GfxMemoryType memoryType )
{
	uint32_t memoryTypeBit = 1 << memoryType;
	return (typeFilter & memoryTypeBit);
}

GfxMemoryType findMemoryType( GfxMemoryTypeFilter typeFilter, GfxMemoryPropertyFlags properties )
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

GfxMemoryRequirements GetImageMemoryRequirement( GfxApiImage image )
{
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements( g_vk.device.device, image, &memRequirements );
	return memRequirements;
}

GfxMemoryRequirements GetBufferMemoryRequirement( GfxApiBuffer buffer )
{
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements( g_vk.device.device, buffer, &memRequirements );
	return memRequirements;
}

