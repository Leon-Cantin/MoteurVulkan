#pragma once
#include "vk_globals.h"

#include <vector>

struct RenderPass {
	VkRenderPass vk_renderpass;
	std::vector<VkFormat> colorFormats;
	VkFormat depthFormat;
};

void CreateRenderPass(const std::vector<VkFormat>& colorFormats, const VkFormat depthFormat, RenderPass * o_renderPass);
void CreateLastRenderPass(const std::vector<VkFormat>& colorFormats, const VkFormat depthFormat, RenderPass * o_renderPass);

void BeginRenderPass(VkCommandBuffer commandBuffer, const RenderPass& renderpass, VkFramebuffer framebuffer, VkExtent2D extent);
void EndRenderPass(VkCommandBuffer commandBuffer);

struct RenderPassChain {
	std::vector<VkRenderPass> chain;
};

extern RenderPassChain renderPassChain;