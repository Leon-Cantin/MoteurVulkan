#include "vk_buffer.h"

#include <cassert>
#include <stdexcept>

VkBuffer create_buffer( VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags )
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = bufferUsageFlags;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer buffer;
	if( vkCreateBuffer( g_vk.device.device, &bufferInfo, nullptr, &buffer ) != VK_SUCCESS )
		throw std::runtime_error( "failed to create buffer!" );

	return buffer;
}

void BindMemory( VkBuffer buffer, const GfxMemAlloc& gfx_mem_alloc )
{
	vkBindBufferMemory( g_vk.device.device, buffer, gfx_mem_alloc.memory, gfx_mem_alloc.offset );
}

void copy_buffer( VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize dst_offset, VkDeviceSize src_offset, VkDeviceSize size )
{
	//TODO: maybe create a new command pool with VK_COMMAND_POOL_CREATE_TRANSIENT_BIT for memory transfers
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = src_offset;
	copyRegion.dstOffset = dst_offset;
	copyRegion.size = size;
	vkCmdCopyBuffer( commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion );
}

void copy_buffer( VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size )
{
	copy_buffer( commandBuffer, dstBuffer, srcBuffer, 0, 0, size );
}