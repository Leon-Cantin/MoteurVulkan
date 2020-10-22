#include "vk_globals.h"
#include <stdexcept>
#include <array>

FrameBuffer CreateFrameBuffer( GfxImageView* colors, uint32_t colorCount, GfxImageView* opt_depth, VkExtent2D extent, const RenderPass& renderPass )
{
	FrameBuffer frameBuffer;

	uint32_t attachementsCount = colorCount + (opt_depth ? 1 : 0);
	assert(attachementsCount <= 5);
	std::array<VkImageView, 5> attachments;
	for (uint32_t i = 0; i < colorCount; ++i)
		attachments[i] = colors[i];
	if ( opt_depth )
		attachments[colorCount] = *opt_depth;

	VkFramebufferCreateInfo framebuffer_info = {};
	framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer_info.renderPass = renderPass.vk_renderpass;
	framebuffer_info.attachmentCount = attachementsCount;
	framebuffer_info.pAttachments = attachments.data();
	framebuffer_info.width = extent.width;
	framebuffer_info.height = extent.height;
	framebuffer_info.layers = 1;
	framebuffer_info.flags = 0;// Optional

	frameBuffer.extent = extent;
	frameBuffer.colorCount = colorCount;
	frameBuffer.depthCount = opt_depth ? 1 : 0;
	frameBuffer.layerCount = 1;

	if (vkCreateFramebuffer(g_gfx.device.device, &framebuffer_info, nullptr, &frameBuffer.frameBuffer))
		throw std::runtime_error("failed to create framebuffer!");

	return frameBuffer;
}

void Destroy( FrameBuffer* frameBuffer )
{
	vkDestroyFramebuffer( g_gfx.device.device, frameBuffer->frameBuffer, nullptr );
	*frameBuffer = {};
}