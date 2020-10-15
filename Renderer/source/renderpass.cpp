#include "renderpass.h"
#include "vk_debug.h"

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

VkFormat ConvertToVkFormat( GfxFormat gfxFormat )
{
	return static_cast< VkFormat >(gfxFormat);
}

VkImageLayout ConvertToVkImageLayout( GfxLayout layout, GfxAccess access )
{
	switch( layout )
	{
	case GfxLayout::UNDEFINED:
		return VK_IMAGE_LAYOUT_UNDEFINED;
	case GfxLayout::GENERAL:
		return VK_IMAGE_LAYOUT_GENERAL;
	case GfxLayout::TRANSFER :
		return access == GfxAccess::READ ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	case GfxLayout::DEPTH_STENCIL :
		return access == GfxAccess::READ ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	case GfxLayout::COLOR :
		return access == GfxAccess::READ ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	case GfxLayout::PRESENT:
		return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	default:
		assert( false );
		return VK_IMAGE_LAYOUT_UNDEFINED;
	}
}

VkAttachmentLoadOp ConvertVkLoadOp( GfxLoadOp loadOp )
{
	return static_cast< VkAttachmentLoadOp >(loadOp);
}

RenderPass CreateRenderPass( const char* name, const AttachementDescription* colorAttachementDescriptions, uint32_t colorAttachementCount, const AttachementDescription* depthStencilAttachement )
{
	const uint32_t MAX_ATTACHEMENT = 16;
	assert( colorAttachementCount - 1 < MAX_ATTACHEMENT );

	VkAttachmentReference references[MAX_ATTACHEMENT];
	VkAttachmentDescription attachements[MAX_ATTACHEMENT];
	const bool hasDepth = depthStencilAttachement;

	//TODO: I can probably infer the load and store op using the access and layout
	for( uint32_t i = 0; i < colorAttachementCount; ++i )
	{
		const AttachementDescription& srcDescription = colorAttachementDescriptions[i];
		VkAttachmentDescription& description = attachements[i];
		description.format = ConvertToVkFormat( srcDescription.format );
		description.flags = 0;
		description.samples = VK_SAMPLE_COUNT_1_BIT;
		description.loadOp = ConvertVkLoadOp( srcDescription.loadOp );
		description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		description.initialLayout = ConvertToVkImageLayout( srcDescription.oldLayout, srcDescription.oldAccess );
		description.finalLayout = ConvertToVkImageLayout( srcDescription.finalLayout, srcDescription.finalAccess );

		VkAttachmentReference& reference = references[i];
		reference.attachment = i;
		reference.layout = ConvertToVkImageLayout( srcDescription.layout, srcDescription.access );
	}

	if( hasDepth )
	{
		VkAttachmentDescription& description = attachements[colorAttachementCount];
		description.format = ConvertToVkFormat( depthStencilAttachement->format );
		description.flags = 0;
		description.samples = VK_SAMPLE_COUNT_1_BIT;
		description.loadOp = ConvertVkLoadOp( depthStencilAttachement->loadOp );
		description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		description.initialLayout = ConvertToVkImageLayout( depthStencilAttachement->oldLayout, depthStencilAttachement->oldAccess );
		description.finalLayout = ConvertToVkImageLayout( depthStencilAttachement->layout, depthStencilAttachement->access );

		VkAttachmentReference& reference = references[colorAttachementCount];
		reference.attachment = colorAttachementCount;
		reference.layout = ConvertToVkImageLayout( depthStencilAttachement->layout, depthStencilAttachement->access );
	}

	bool containsDepth = depthStencilAttachement;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = colorAttachementCount;
	subpass.pColorAttachments = references;
	subpass.pDepthStencilAttachment = hasDepth ? &references[colorAttachementCount] : VK_NULL_HANDLE;

	//TODO: have less agressive dst external dependency (maybe src too)
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = 0;
	dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
	dependency.srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
	dependency.srcAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency.dstAccessMask = 0;
	dependency.dependencyFlags = 0;

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = colorAttachementCount + ( hasDepth ? 1 : 0 );
	render_pass_info.pAttachments = attachements;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	render_pass_info.dependencyCount = 1; //implicit dependency to VK_SUBPASS_EXTERNAL
	render_pass_info.pDependencies = &dependency;

	RenderPass renderPass;
	/*A render pass represents a collection of attachments, subpasses, and dependencies between the subpasses,
	and describes how the attachments are used over the course of the subpasses.*/
	if( vkCreateRenderPass( g_vk.device.device, &render_pass_info, nullptr, &renderPass.vk_renderpass ) != VK_SUCCESS )
		throw std::runtime_error( "failed to create render pass!" );

	renderPass.colorFormats.resize( colorAttachementCount );
	for( uint32_t i = 0; i < colorAttachementCount; ++i )
		renderPass.colorFormats[i] = attachements[i].format;
	renderPass.depthFormat = hasDepth ? attachements[colorAttachementCount].format : VK_FORMAT_UNDEFINED;

	MarkVkObject( ( uint64_t )renderPass.vk_renderpass, VK_OBJECT_TYPE_RENDER_PASS, name );

	return renderPass;
}