#pragma once
#include "vk_globals.h"

#include "framebuffer.h"

#include <vector>

struct RenderPass {
	VkRenderPass vk_renderpass;
	std::vector<VkFormat> colorFormats;
	VkFormat depthFormat;
	FrameBuffer frameBuffer;
	FrameBuffer outputFrameBuffer[SIMULTANEOUS_FRAMES];
};

void BeginRenderPass(VkCommandBuffer commandBuffer, const RenderPass& renderpass, VkFramebuffer framebuffer, VkExtent2D extent);
void EndRenderPass(VkCommandBuffer commandBuffer);

struct RenderPassChain {
	std::vector<VkRenderPass> chain;
};

extern RenderPassChain renderPassChain;