#pragma once
#include "vk_globals.h"

#include <vector>

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

void BeginRenderPass(GfxCommandBuffer commandBuffer, const RenderPass& renderpass, const FrameBuffer& framebuffer);
void EndRenderPass(GfxCommandBuffer commandBuffer);