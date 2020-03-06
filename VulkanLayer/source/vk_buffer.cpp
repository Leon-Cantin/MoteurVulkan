#include "vk_buffer.h"

#include "vk_memory.h"
#include "vk_commands.h"

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
	if( vkCreateBuffer( g_vk.device, &bufferInfo, nullptr, &buffer ) != VK_SUCCESS )
		throw std::runtime_error( "failed to create buffer!" );

	return buffer;
}

static GpuBuffer bind_gfx_buffer_mem( const VkBuffer buffer, const GfxMemAlloc& gfx_mem_alloc )
{
	vkBindBufferMemory( g_vk.device, buffer, gfx_mem_alloc.memory, gfx_mem_alloc.offset );
	const GpuBuffer gfx_buffer { buffer, gfx_mem_alloc };
	return gfx_buffer;
}

void CreateCommitedGpuBuffer( VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryProperties, GpuBuffer* o_buffer )
{
	const VkBuffer buffer = create_buffer( size, bufferUsageFlags );

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements( g_vk.device, buffer, &memRequirements );

	const uint32_t mem_type = findMemoryType( memRequirements.memoryTypeBits, memoryProperties );
	const GfxMemAlloc gfx_mem = allocate_gfx_memory( memRequirements.size, mem_type );

	*o_buffer = bind_gfx_buffer_mem( buffer, gfx_mem );
}

void CreatePerFrameBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryProperties, PerFrameBuffer * o_buffer)
{
	for( uint32_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
		o_buffer->buffers[i].buffer = create_buffer( size, bufferUsageFlags );

	VkMemoryRequirements memRequirements;
	//Call it on all just so the validation layer doesn't complain
	for(uint32_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		vkGetBufferMemoryRequirements(g_vk.device, o_buffer->buffers[i].buffer, &memRequirements);

	//TODO: respect alignement!!!!!
	const VkDeviceSize mem_size = memRequirements.size * SIMULTANEOUS_FRAMES;
	const uint32_t mem_type = findMemoryType( memRequirements.memoryTypeBits, memoryProperties );
	const GfxMemAlloc gfx_mem = allocate_gfx_memory( mem_size, mem_type );

	VkDeviceSize memoryStride = memRequirements.size;
	for( uint32_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
	{
		const VkDeviceSize memoryOffset = memoryStride * i;
		const GfxMemAlloc sub_gfx_mem = suballocate_gfx_memory( gfx_mem, memoryStride, memoryOffset );
		o_buffer->buffers[i] = bind_gfx_buffer_mem( o_buffer->buffers[i].buffer, sub_gfx_mem );
	}

	o_buffer->gfx_mem_alloc = gfx_mem;
}

void UpdateGpuBuffer( const GpuBuffer* buffer, const void* src, VkDeviceSize size, VkDeviceSize offset )
{
	UpdateGpuMemory( &buffer->gpuMemory, src, size, offset);
}

void UpdatePerFrameBuffer(const PerFrameBuffer * buffer, const void* src, VkDeviceSize size, uint32_t frame)
{
	assert( frame < SIMULTANEOUS_FRAMES );

	const VkDeviceSize frameMemoryOffset = buffer->buffers[frame].gpuMemory.offset;
	UpdateGpuBuffer( &buffer->buffers[frame], src, size, frameMemoryOffset );
}

void DestroyPerFrameBuffer(PerFrameBuffer * o_buffer)
{
	for(uint32_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		vkDestroyBuffer(g_vk.device, o_buffer->buffers[i].buffer, nullptr);		
	vkFreeMemory(g_vk.device, o_buffer->gfx_mem_alloc.memory, nullptr);

	*o_buffer = {};
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

void DestroyCommitedGpuBuffer( GpuBuffer* buffer )
{
	//TODO: differentiate between placed buffers and commited buffers, don't destroy memory in the first case
	vkDestroyBuffer( g_vk.device, buffer->buffer, nullptr );
	vkFreeMemory( g_vk.device, buffer->gpuMemory.memory, nullptr );
	memset( buffer, 0, sizeof( GpuBuffer ) );
}