#pragma once

#include "vk_globals.h"
#include "vk_memory.h"

VkBuffer create_buffer( GfxDeviceSize size, GfxBufferUsageFlags bufferUsageFlags );
void BindMemory( GfxApiBuffer buffer, const GfxMemAlloc& gfx_mem_alloc );

void copy_buffer( GfxCommandBuffer commandBuffer, GfxApiBuffer dstBuffer, GfxApiBuffer srcBuffer, GfxDeviceSize dst_offset, GfxDeviceSize src_offset, GfxDeviceSize size );
void copy_buffer( GfxCommandBuffer commandBuffer, GfxApiBuffer dstBuffer, GfxApiBuffer srcBuffer, GfxDeviceSize size );