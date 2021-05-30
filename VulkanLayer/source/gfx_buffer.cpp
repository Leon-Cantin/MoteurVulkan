#include "vk_globals.h"

#include <cassert>
#include <stdexcept>

namespace R_HW
{
	static GpuBuffer bind_gfx_buffer_mem( VkBuffer buffer, const GfxMemAlloc& gfx_mem_alloc )
	{
		BindMemory( buffer, gfx_mem_alloc );
		const GpuBuffer gfx_buffer { buffer, gfx_mem_alloc };
		return gfx_buffer;
	}

	void CreateCommitedGpuBuffer( GfxDeviceSize size, GfxBufferUsageFlags bufferUsageFlags, GfxMemoryPropertyFlags memoryProperties, GpuBuffer* o_buffer )
	{
		const GfxApiBuffer buffer = create_buffer( size, bufferUsageFlags );

		GfxMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements( g_gfx.device.device, buffer, &memRequirements );

		const GfxMemoryType mem_type = findMemoryType( memRequirements.memoryTypeBits, memoryProperties );
		const GfxMemAlloc gfx_mem = allocate_gfx_memory( memRequirements.size, mem_type );

		*o_buffer = bind_gfx_buffer_mem( buffer, gfx_mem );
	}

	void UpdateGpuBuffer( const GpuBuffer* buffer, const void* src, GfxDeviceSize size, GfxDeviceSize offset )
	{
		UpdateGpuMemory( &buffer->gpuMemory, src, size, offset );
	}

	void Destroy( GpuBuffer* buffer )
	{
		vkDestroyBuffer( g_gfx.device.device, buffer->buffer, nullptr );
		destroy_gfx_memory( &buffer->gpuMemory );
		*buffer = {};
	}
}