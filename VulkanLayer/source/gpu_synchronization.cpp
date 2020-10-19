#include "gpu_synchronization.h"

#include <stdexcept>

void unsignalSemaphore(VkSemaphore semaphore)
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { semaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 0;
	submitInfo.pCommandBuffers = VK_NULL_HANDLE;

	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = VK_NULL_HANDLE;

	if (vkQueueSubmit(g_vk.device.graphics_queue.queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}
}

bool CreateGfxSemaphore( GfxSemaphore* pSemaphore )
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	return vkCreateSemaphore( g_vk.device.device, &semaphoreInfo, nullptr, pSemaphore ) == VK_SUCCESS;
}

void DestroyGfxSemaphore( GfxSemaphore* pSemaphore )
{
	vkDestroySemaphore( g_vk.device.device, *pSemaphore, nullptr );
	*pSemaphore = VK_NULL_HANDLE;
}

bool CreateGfxFence( GfxFence* pFence )
{
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	return vkCreateFence( g_vk.device.device, &fenceInfo, nullptr, pFence ) == VK_SUCCESS;
}

void DestroyGfxFence( GfxFence* pFence )
{
	vkDestroyFence( g_vk.device.device, *pFence, nullptr );
	*pFence = VK_NULL_HANDLE;
}

void ResetGfxFences( const GfxFence* pFences, uint32_t fencesCount )
{
	vkResetFences( g_vk.device.device, fencesCount, pFences );
}

void WaitForFence( const GfxFence* pFences, uint32_t fenceCount, uint64_t timeoutNS )
{
	vkWaitForFences( g_vk.device.device, fenceCount, pFences, VK_TRUE, timeoutNS );
}

void WaitForFence( const GfxFence* pFences, uint32_t fenceCount )
{
	WaitForFence( pFences, fenceCount, std::numeric_limits<uint64_t>::max() );
}