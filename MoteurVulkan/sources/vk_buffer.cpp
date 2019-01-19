#include "vk_buffer.h"

#include "vk_memory.h"
#include "vk_commands.h"

#include<stdexcept>

void create_buffer(VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryProperties, VkBuffer& buffer, VkDeviceMemory& deviceMemory)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = bufferUsageFlags;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(g_vk.device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		throw std::runtime_error("failed to create buffer!");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(g_vk.device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, memoryProperties);

	if (vkAllocateMemory(g_vk.device, &allocInfo, nullptr, &deviceMemory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate buffer memory!");

	vkBindBufferMemory(g_vk.device, buffer, deviceMemory, 0);
}

void CreatePerFrameBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryProperties, PerFrameBuffer * o_buffer)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = bufferUsageFlags;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	for(uint32_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		if (vkCreateBuffer(g_vk.device, &bufferInfo, nullptr, &o_buffer->buffers[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create buffer!");

	VkMemoryRequirements memRequirements;
	//Call it on all just so the validation layer doesn't complain
	for(uint32_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		vkGetBufferMemoryRequirements(g_vk.device, o_buffer->buffers[i], &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size * SIMULTANEOUS_FRAMES;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, memoryProperties);

	if (vkAllocateMemory(g_vk.device, &allocInfo, nullptr, &o_buffer->memory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate buffer memory!");

	for(uint32_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		vkBindBufferMemory(g_vk.device, o_buffer->buffers[i], o_buffer->memory, memRequirements.size * i);

	o_buffer->stride = memRequirements.size;
}

VkDeviceSize GetMemoryOffsetForFrame(const PerFrameBuffer * buffer, uint32_t frame)
{
	return buffer->stride * frame;
}

void UpdatePerFrameBuffer(const PerFrameBuffer * buffer, const void* src, VkDeviceSize size, uint32_t frame)
{
	VkDeviceSize frameMemoryOffset = GetMemoryOffsetForFrame(buffer, frame);
	void* data;
	vkMapMemory(g_vk.device, buffer->memory, frameMemoryOffset, size, 0, &data);
	memcpy(data, src, size);
	vkUnmapMemory(g_vk.device, buffer->memory);
}

void DestroyPerFrameBuffer(PerFrameBuffer * o_buffer)
{
	for(uint32_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		vkDestroyBuffer(g_vk.device, o_buffer->buffers[i], nullptr);		
	vkFreeMemory(g_vk.device, o_buffer->memory, nullptr);
}

void copy_buffer(VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size)
{
	//TODO: maybe create a new command pool with VK_COMMAND_POOL_CREATE_TRANSIENT_BIT for memory transfers
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer);
}

void copy_buffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size)
{
	//TODO: maybe create a new command pool with VK_COMMAND_POOL_CREATE_TRANSIENT_BIT for memory transfers
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
}


void copyDataToDeviceLocalMemory(VkBuffer dstBuffer, const void* data, VkDeviceSize bufferSize)
{
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	//TODO: have a big temp staging buffer
	create_buffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	UpdateBuffer(stagingBufferMemory, data, bufferSize);

	copy_buffer(dstBuffer, stagingBuffer, bufferSize);

	vkDestroyBuffer(g_vk.device, stagingBuffer, nullptr);
	vkFreeMemory(g_vk.device, stagingBufferMemory, nullptr);
}

void copyDataToDeviceLocalMemory(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, const void* data, VkDeviceSize bufferSize)
{
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	//TODO: have a big temp staging buffer
	create_buffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	UpdateBuffer(stagingBufferMemory, data, bufferSize);

	copy_buffer(commandBuffer, dstBuffer, stagingBuffer, bufferSize);

	vkDestroyBuffer(g_vk.device, stagingBuffer, nullptr);
	vkFreeMemory(g_vk.device, stagingBufferMemory, nullptr);
}

void createBufferToDeviceLocalMemory(VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, VkBuffer* o_buffer, VkDeviceMemory* o_VkDeviceMemory)
{
	create_buffer(bufferSize, usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		*o_buffer, *o_VkDeviceMemory);
}

void createBufferToDeviceLocalMemory(const void* data, VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, VkBuffer* o_buffer, VkDeviceMemory* o_VkDeviceMemory)
{
	createBufferToDeviceLocalMemory(bufferSize, usageFlags, o_buffer, o_VkDeviceMemory);

	copyDataToDeviceLocalMemory(*o_buffer, data, bufferSize);
}

void UpdateBuffer(VkDeviceMemory dstMemory, const void* src, VkDeviceSize size)
{
	void* data;
	vkMapMemory(g_vk.device, dstMemory, 0, size, 0, &data);
	memcpy(data, src, size);
	vkUnmapMemory(g_vk.device, dstMemory);
}