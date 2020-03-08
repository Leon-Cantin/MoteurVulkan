#include "gfx_buffer.h"

#include <cassert>
#include <stdexcept>

static GpuBuffer bind_gfx_buffer_mem( VkBuffer buffer, const GfxMemAlloc& gfx_mem_alloc )
{
	BindMemory( buffer, gfx_mem_alloc );
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

void CreatePerFrameBuffer( VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryProperties, PerFrameBuffer * o_buffer )
{
	for( uint32_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
		o_buffer->buffers[i].buffer = create_buffer( size, bufferUsageFlags );

	VkMemoryRequirements memRequirements;
	//Call it on all just so the validation layer doesn't complain
	for( uint32_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
		vkGetBufferMemoryRequirements( g_vk.device, o_buffer->buffers[i].buffer, &memRequirements );

	const VkDeviceSize alignement_extra_size = memRequirements.size % memRequirements.alignment;
	const VkDeviceSize memory_stride = memRequirements.size + alignement_extra_size;
	const VkDeviceSize mem_size = memory_stride * SIMULTANEOUS_FRAMES;
	const uint32_t mem_type = findMemoryType( memRequirements.memoryTypeBits, memoryProperties );
	const GfxMemAlloc gfx_mem = allocate_gfx_memory( mem_size, mem_type );
	
	for( uint32_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
	{
		const VkDeviceSize memoryOffset = memory_stride * i;
		const GfxMemAlloc sub_gfx_mem = suballocate_gfx_memory( gfx_mem, memRequirements.size, memoryOffset );
		o_buffer->buffers[i] = bind_gfx_buffer_mem( o_buffer->buffers[i].buffer, sub_gfx_mem );
	}

	o_buffer->gfx_mem_alloc = gfx_mem;
}

void UpdateGpuBuffer( const GpuBuffer* buffer, const void* src, VkDeviceSize size, VkDeviceSize offset )
{
	UpdateGpuMemory( &buffer->gpuMemory, src, size, offset );
}

void UpdatePerFrameBuffer( const PerFrameBuffer * buffer, const void* src, VkDeviceSize size, uint32_t frame )
{
	assert( frame < SIMULTANEOUS_FRAMES );

	const VkDeviceSize frameMemoryOffset = buffer->buffers[frame].gpuMemory.offset;
	UpdateGpuBuffer( &buffer->buffers[frame], src, size, frameMemoryOffset );
}

void DestroyPerFrameBuffer( PerFrameBuffer * o_buffer )
{
	for( uint32_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
		vkDestroyBuffer( g_vk.device, o_buffer->buffers[i].buffer, nullptr );
	destroy_gfx_memory( &o_buffer->gfx_mem_alloc );
	*o_buffer = {};
}

void DestroyCommitedGpuBuffer( GpuBuffer* buffer )
{
	vkDestroyBuffer( g_vk.device, buffer->buffer, nullptr );
	destroy_gfx_memory( &buffer->gpuMemory );
	*buffer = {};
}