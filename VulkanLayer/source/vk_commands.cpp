#include "vk_commands.h"

#include <stdexcept>

VkCommandBuffer beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = g_vk.graphicsSingleUseCommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(g_vk.device.device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit( g_vk.device.graphics_queue.queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle( g_vk.device.graphics_queue.queue);

	vkFreeCommandBuffers( g_vk.device.device, g_vk.graphicsSingleUseCommandPool, 1, &commandBuffer);
}

void CreateCommandPool(uint32_t queueFamilyIndex, VkCommandPool* o_commandPool)
{
	VkCommandPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex = queueFamilyIndex;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; //Optional

	if (vkCreateCommandPool(g_vk.device.device, &pool_info, nullptr, o_commandPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool!");
}

void CreateSingleUseCommandPool(uint32_t queueFamilyIndex, VkCommandPool* o_commandPool)
{	
	VkCommandPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex = queueFamilyIndex;
	pool_info.flags = 0; //Optional

	if (vkCreateCommandPool(g_vk.device.device, &pool_info, nullptr, o_commandPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool!");
}

void BeginCommandBufferRecording(VkCommandBuffer commandBuffer)
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}
}

void EndCommandBufferRecording(VkCommandBuffer commandBuffer)
{
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		throw std::runtime_error("failed to record command buffer!");
}

VkFilter ToVkFilter( GfxFilter filter )
{
	return static_cast< VkFilter >(filter);
}

void CmdBlitImage( VkCommandBuffer commandBuffer, GfxApiImage srcImage, int32_t srcX1, int32_t srcY1, int32_t srcZ1, int32_t srcX2, int32_t srcY2, int32_t srcZ2, uint32_t srcMipLevel, GfxLayout srcLayout, GfxAccess srcAccess,
	GfxApiImage dstImage, int32_t dstX1, int32_t dstY1, int32_t dstZ1, int32_t dstX2, int32_t dstY2, int32_t dstZ2, uint32_t dstMipLevel, GfxLayout dstLayout, GfxAccess dstAccess, GfxFilter filter )
{
	VkImageBlit blit = {};
	blit.srcOffsets[0] = { srcX1, srcY1, srcZ1 };
	blit.srcOffsets[1] = { srcX2, srcY2, srcZ2 };
	blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.srcSubresource.mipLevel = srcMipLevel;
	blit.srcSubresource.baseArrayLayer = 0;
	blit.srcSubresource.layerCount = 1;

	blit.dstOffsets[0] = { dstX1, dstY1, dstZ1 };
	blit.dstOffsets[1] = { dstX2, dstY2, dstZ2 };
	blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.dstSubresource.mipLevel = dstMipLevel;
	blit.dstSubresource.baseArrayLayer = 0;
	blit.dstSubresource.layerCount = 1;

	vkCmdBlitImage( commandBuffer,
		srcImage, ConvertToVkImageLayout( srcLayout, srcAccess ),
		dstImage, ConvertToVkImageLayout( srcLayout, srcAccess ),
		1, &blit,
		ToVkFilter( filter ) );

}

void CmdBlitImage( VkCommandBuffer commandBuffer, GfxApiImage srcImage, int32_t srcX1, int32_t srcY1, int32_t srcZ1, int32_t srcX2, int32_t srcY2, int32_t srcZ2, uint32_t srcMipLevel,
	GfxApiImage dstImage, int32_t dstX1, int32_t dstY1, int32_t dstZ1, int32_t dstX2, int32_t dstY2, int32_t dstZ2, uint32_t dstMipLevel, GfxFilter filter )
{
	CmdBlitImage( commandBuffer, srcImage, srcX1, srcY1, srcZ1, srcX2, srcY2, srcZ2, srcMipLevel, GfxLayout::TRANSFER, GfxAccess::READ,
		dstImage, dstX1, dstY1, dstZ1, dstX2, dstY2, dstZ2, dstMipLevel, GfxLayout::TRANSFER, GfxAccess::WRITE, filter );
}