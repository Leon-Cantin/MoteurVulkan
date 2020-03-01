#include "vk_memory.h"

#include <stdexcept>

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
