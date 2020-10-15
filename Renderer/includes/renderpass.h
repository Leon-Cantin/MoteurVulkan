#pragma once
#include "vk_globals.h"

#include "framebuffer.h"
#include "vk_gpu_pipeline.h"

#include <vector>

struct RenderPass {
	VkRenderPass vk_renderpass;
	std::vector<VkFormat> colorFormats;
	VkFormat depthFormat;
	FrameBuffer outputFrameBuffer[SIMULTANEOUS_FRAMES];
};

enum class GfxFormat
{
	UNDEFINED = VK_FORMAT_UNDEFINED,
	R8_UNORM = VK_FORMAT_R8_UNORM,
	R8_UINT = VK_FORMAT_R8_UINT,
	R8G8_UNORM = VK_FORMAT_R8G8_UNORM,
	R8G8_UINT = VK_FORMAT_R8G8_UINT,
	R8G8B8_UNORM = VK_FORMAT_R8G8B8_UNORM,
	R8G8B8_UINT = VK_FORMAT_R8G8B8_UINT,
	B8G8R8_UNORM = VK_FORMAT_B8G8R8_UNORM,
	B8G8R8_UINT = VK_FORMAT_B8G8R8_UINT,
	R8G8B8A8_UNORM = VK_FORMAT_R8G8B8A8_UNORM,
	R8G8B8A8_UINT = VK_FORMAT_R8G8B8A8_UINT,
	B8G8R8A8_UNORM = VK_FORMAT_B8G8R8A8_UNORM,
	B8G8R8A8_UINT = VK_FORMAT_B8G8R8A8_UINT,
	D32_SFLOAT = VK_FORMAT_D32_SFLOAT,
	S8_UINT = VK_FORMAT_S8_UINT,
};

enum class GfxLayout
{
	UNDEFINED,
	GENERAL,
	COLOR,
	DEPTH_STENCIL,
	TRANSFER,
	PRESENT,
};

enum class GfxAccess
{
	READ,
	WRITE,
};

enum class GfxLoadOp
{
	DONT_CARE = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
	LOAD = VK_ATTACHMENT_LOAD_OP_LOAD,
	CLEAR = VK_ATTACHMENT_LOAD_OP_CLEAR,
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

RenderPass CreateRenderPass( const char* name, const AttachementDescription* colorAttachementDescriptions, uint32_t colorAttachementCount, const AttachementDescription* depthStencilAttachement );

void BeginRenderPass(VkCommandBuffer commandBuffer, const RenderPass& renderpass, VkFramebuffer framebuffer, VkExtent2D extent);
void EndRenderPass(VkCommandBuffer commandBuffer);