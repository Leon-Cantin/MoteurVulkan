#include "renderpass.h"

void BeginRenderPass(VkCommandBuffer commandBuffer, const RenderPass& renderpass, VkFramebuffer framebuffer, VkExtent2D extent)
{
	VkRenderPassBeginInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = renderpass.vk_renderpass;
	render_pass_info.framebuffer = framebuffer;
	render_pass_info.renderArea.offset = { 0, 0 };
	render_pass_info.renderArea.extent = extent;

	uint32_t colorCount = static_cast<uint32_t>(renderpass.colorFormats.size());
	bool hasDepth = renderpass.depthFormat != VK_FORMAT_UNDEFINED;
	uint32_t totalCount = colorCount + (hasDepth ? 1 : 0);
	std::array<VkClearValue, 5> clearValues = { 0.0f, 0.0f, 0.0f, 1.0f };
	assert(totalCount <= 5);
	if (hasDepth)
		clearValues[colorCount] = { 1.0f, 0 };
	render_pass_info.clearValueCount = totalCount;
	render_pass_info.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
}

void EndRenderPass(VkCommandBuffer commandBuffer)
{
	vkCmdEndRenderPass(commandBuffer);
}