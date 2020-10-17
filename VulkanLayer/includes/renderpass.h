#pragma once
#include "vk_globals.h"

#include "vk_gpu_pipeline.h"

#include <vector>

struct FrameBuffer
{
	VkFramebuffer frameBuffer;
	VkExtent2D extent;
	uint32_t layerCount;
	uint32_t colorCount;
	uint32_t depthCount;
};

struct RenderPass {
	VkRenderPass vk_renderpass;
	std::vector<VkFormat> colorFormats;
	VkFormat depthFormat;
	FrameBuffer outputFrameBuffer[SIMULTANEOUS_FRAMES];
};

struct AttachementDescription
{
	GfxFormat format;
	GfxLoadOp loadOp;
	GfxAccess access;
	GfxLayout layout;

	GfxAccess oldAccess;
	GfxLayout oldLayout;

	GfxAccess finalAccess;
	GfxLayout finalLayout;
};

//TODO: IF a render pass contains the framebuffers and they are associated with it, maybe we should create and destroy them here too.
RenderPass CreateRenderPass( const char* name, const AttachementDescription* colorAttachementDescriptions, uint32_t colorAttachementCount, const AttachementDescription* depthStencilAttachement );
void Destroy( RenderPass* renderPass );

void BeginRenderPass(VkCommandBuffer commandBuffer, const RenderPass& renderpass, const FrameBuffer& framebuffer);
void EndRenderPass(VkCommandBuffer commandBuffer);