#include "vk_buffer.h"

#include <cassert>
#include <stdexcept>

VkMemoryPropertyFlags ToVkMemoryPropertyFlags( GfxMemoryPropertyFlags gfxMemoryProperty )
{
	return static_cast< VkMemoryPropertyFlags >(gfxMemoryProperty);
}

VkBufferUsageFlags ToVkBufferUsageFlags( GfxBufferUsageFlags bufferUsageFlags )
{
	return static_cast< VkBufferUsageFlags >(bufferUsageFlags);
}

VkBuffer create_buffer( GfxDeviceSize size, GfxBufferUsageFlags bufferUsageFlags )
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = ToVkBufferUsageFlags( bufferUsageFlags );
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer buffer;
	if( vkCreateBuffer( g_gfx.device.device, &bufferInfo, nullptr, &buffer ) != VK_SUCCESS )
		throw std::runtime_error( "failed to create buffer!" );

	return buffer;
}

void BindMemory( GfxApiBuffer buffer, const GfxMemAlloc& gfx_mem_alloc )
{
	vkBindBufferMemory( g_gfx.device.device, buffer, gfx_mem_alloc.memory, gfx_mem_alloc.offset );
}

void copy_buffer( GfxCommandBuffer commandBuffer, GfxApiBuffer dstBuffer, GfxApiBuffer srcBuffer, GfxDeviceSize dst_offset, GfxDeviceSize src_offset, GfxDeviceSize size )
{
	//TODO: maybe create a new command pool with VK_COMMAND_POOL_CREATE_TRANSIENT_BIT for memory transfers
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = src_offset;
	copyRegion.dstOffset = dst_offset;
	copyRegion.size = size;
	vkCmdCopyBuffer( commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion );
}

void copy_buffer( GfxCommandBuffer commandBuffer, GfxApiBuffer dstBuffer, GfxApiBuffer srcBuffer, GfxDeviceSize size )
{
	copy_buffer( commandBuffer, dstBuffer, srcBuffer, 0, 0, size );
}