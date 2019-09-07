#include "vk_buffer.h"

#include "vk_memory.h"
#include "vk_commands.h"

#include <cassert>
#include <stdexcept>

static void create_buffer( VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryProperties, VkBuffer& o_buffer, VkDeviceMemory& o_deviceMemory )
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = bufferUsageFlags;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if( vkCreateBuffer( g_vk.device, &bufferInfo, nullptr, &o_buffer ) != VK_SUCCESS )
		throw std::runtime_error( "failed to create buffer!" );

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements( g_vk.device, o_buffer, &memRequirements );

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType( memRequirements.memoryTypeBits, memoryProperties );

	if( vkAllocateMemory( g_vk.device, &allocInfo, nullptr, &o_deviceMemory ) != VK_SUCCESS )
		throw std::runtime_error( "failed to allocate buffer memory!" );

	vkBindBufferMemory( g_vk.device, o_buffer, o_deviceMemory, 0 );
}

void CreateCommitedGpuBuffer( VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryProperties, GpuBuffer* o_buffer )
{
	create_buffer( size, bufferUsageFlags, memoryProperties, o_buffer->buffer, o_buffer->gpuMemory.memory );
	o_buffer->gpuMemory.offset = 0;
	o_buffer->gpuMemory.size = size;
}

void CreatePerFrameBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryProperties, PerFrameBuffer * o_buffer)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = bufferUsageFlags;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	for(uint32_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		if (vkCreateBuffer(g_vk.device, &bufferInfo, nullptr, &o_buffer->buffers[i].buffer) != VK_SUCCESS)
			throw std::runtime_error("failed to create buffer!");

	VkMemoryRequirements memRequirements;
	//Call it on all just so the validation layer doesn't complain
	for(uint32_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		vkGetBufferMemoryRequirements(g_vk.device, o_buffer->buffers[i].buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size * SIMULTANEOUS_FRAMES;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, memoryProperties);

	if (vkAllocateMemory(g_vk.device, &allocInfo, nullptr, &o_buffer->memory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate buffer memory!");

	VkDeviceSize memoryStride = memRequirements.size;
	for( uint32_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
	{
		VkDeviceSize memoryOffset = memoryStride * i;
		vkBindBufferMemory( g_vk.device, o_buffer->buffers[i].buffer, o_buffer->memory, memoryOffset );
		o_buffer->buffers[i].gpuMemory = { o_buffer->memory, memoryOffset, memoryStride };
	}
	o_buffer->stride = memoryStride;
}


static void UpdateGpuMemory( const GpuMemory* dstMemory, const void* src, VkDeviceSize size, VkDeviceSize offset )
{
	assert( offset + size <= dstMemory->size );
	void* data;
	vkMapMemory( g_vk.device, dstMemory->memory, dstMemory->offset + offset, size, 0, &data );
	memcpy( data, src, size );
	vkUnmapMemory( g_vk.device, dstMemory->memory );
}

void UpdateGpuBuffer( const GpuBuffer* buffer, const void* src, VkDeviceSize size, VkDeviceSize offset )
{
	UpdateGpuMemory( &buffer->gpuMemory, src, size, offset);
}

void UpdatePerFrameBuffer(const PerFrameBuffer * buffer, const void* src, VkDeviceSize size, uint32_t frame)
{
	assert( frame < SIMULTANEOUS_FRAMES );

	VkDeviceSize frameMemoryOffset = buffer->stride * frame;
	UpdateGpuBuffer( &buffer->buffers[frame], src, size, frameMemoryOffset );
}

void DestroyPerFrameBuffer(PerFrameBuffer * o_buffer)
{
	for(uint32_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		vkDestroyBuffer(g_vk.device, o_buffer->buffers[i].buffer, nullptr);		
	vkFreeMemory(g_vk.device, o_buffer->memory, nullptr);

	*o_buffer = {};
}


void copy_buffer( VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size )
{
	//TODO: maybe create a new command pool with VK_COMMAND_POOL_CREATE_TRANSIENT_BIT for memory transfers
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer( commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion );
}


void CopyBufferImmediate(VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size)
{
	//TODO: maybe create a new command pool with VK_COMMAND_POOL_CREATE_TRANSIENT_BIT for memory transfers
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	copy_buffer( commandBuffer, dstBuffer, srcBuffer, size );
	endSingleTimeCommands(commandBuffer);
}


void copyDataToDeviceLocalMemory(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, const void* data, VkDeviceSize bufferSize)
{
	GpuBuffer stagingBuffer;

	//TODO: have a big temp staging buffer
	CreateCommitedGpuBuffer( bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer );

	UpdateGpuBuffer( &stagingBuffer, data, bufferSize, 0 );

	copy_buffer(commandBuffer, dstBuffer, stagingBuffer.buffer, bufferSize);

	DestroyCommitedGpuBuffer( &stagingBuffer );
}


void copyDataToDeviceLocalMemoryImmediate( VkBuffer dstBuffer, const void* data, VkDeviceSize bufferSize )
{
	GpuBuffer stagingBuffer;

	//TODO: have a big temp staging buffer
	CreateCommitedGpuBuffer( bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer );

	UpdateGpuBuffer( &stagingBuffer, data, bufferSize, 0 );

	CopyBufferImmediate( dstBuffer, stagingBuffer.buffer, bufferSize );

	DestroyCommitedGpuBuffer( &stagingBuffer );
}


void createBufferToDeviceLocalMemory(VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, VkBuffer* o_buffer, VkDeviceMemory* o_VkDeviceMemory)
{
	create_buffer(bufferSize, usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		*o_buffer, *o_VkDeviceMemory);
}


void createBufferToDeviceLocalMemory(const void* data, VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, VkBuffer* o_buffer, VkDeviceMemory* o_VkDeviceMemory)
{
	createBufferToDeviceLocalMemory(bufferSize, usageFlags, o_buffer, o_VkDeviceMemory);

	copyDataToDeviceLocalMemoryImmediate(*o_buffer, data, bufferSize);
}


void DestroyCommitedGpuBuffer( GpuBuffer* buffer )
{
	//TODO: differentiate between placed buffers and commited buffers, don't destroy memory in the first case
	vkDestroyBuffer( g_vk.device, buffer->buffer, nullptr );
	vkFreeMemory( g_vk.device, buffer->gpuMemory.memory, nullptr );
	memset( buffer, 0, sizeof( GpuBuffer ) );
}