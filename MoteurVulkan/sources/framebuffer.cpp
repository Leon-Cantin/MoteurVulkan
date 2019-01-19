#include "framebuffer.h"

void createFrameBuffer(GfxImage* colors, uint32_t colorCount, GfxImage* depth, VkExtent2D extent, VkRenderPass renderPass, FrameBuffer* o_frameBuffer)
{
	uint32_t attachementsCount = colorCount + (depth ? 1 : 0);
	assert(attachementsCount <= 5);
	std::array<VkImageView, 5> attachments;
	for (uint32_t i = 0; i < colorCount; ++i)
		attachments[i] = colors[i].imageView;
	if (depth)
		attachments[colorCount] = depth->imageView;

	VkFramebufferCreateInfo framebuffer_info = {};
	framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer_info.renderPass = renderPass;
	framebuffer_info.attachmentCount = attachementsCount;
	framebuffer_info.pAttachments = attachments.data();
	framebuffer_info.width = extent.width;
	framebuffer_info.height = extent.height;
	framebuffer_info.layers = 1;
	framebuffer_info.flags = 0;// Optional

	o_frameBuffer->extent = extent;
	o_frameBuffer->colorCount = colorCount;
	o_frameBuffer->depthCount = depth ? 1 : 0;
	o_frameBuffer->layerCount = 1;

	if (vkCreateFramebuffer(g_vk.device, &framebuffer_info, nullptr, &o_frameBuffer->frameBuffer))
		throw std::runtime_error("failed to create framebuffer!");
}