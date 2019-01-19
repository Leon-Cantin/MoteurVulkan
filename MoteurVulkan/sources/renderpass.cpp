#include "renderpass.h"

#include <array>
#include <cassert>

void CreateRenderPass(const std::vector<VkFormat>& colorFormats, const VkFormat depthFormat, RenderPass * o_renderPass)
{
	//Attachements
	std::vector<VkAttachmentDescription> color_attachements;
	std::vector<VkAttachmentReference> color_attachement_refs;
	color_attachements.resize(colorFormats.size());
	color_attachement_refs.resize(colorFormats.size());
	for (size_t i = 0; i < color_attachements.size(); ++i) {
		VkAttachmentDescription& color_attachement = color_attachements[i];
		color_attachement = {};
		color_attachement.format = colorFormats[i];
		color_attachement.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachement.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachement.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachement.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachement.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachement.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachement.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference& color_attachement_ref = color_attachement_refs[i];
		color_attachement_ref = {};
		color_attachement_ref.attachment = static_cast<uint32_t>(i);
		color_attachement_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = static_cast<uint32_t>(colorFormats.size());
	subpass.pColorAttachments = color_attachement_refs.data();

	VkAttachmentDescription depthAttachment = {};
	if (depthFormat != VK_FORMAT_UNDEFINED)
	{
		depthAttachment.format = depthFormat;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = static_cast<uint32_t>(colorFormats.size()); //Take the last spot
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		subpass.pDepthStencilAttachment = &depthAttachmentRef;
	}
	else
	{
		subpass.pDepthStencilAttachment = VK_NULL_HANDLE;
	}

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::vector<VkAttachmentDescription> attachments = color_attachements;

	if (depthFormat != VK_FORMAT_UNDEFINED)
		attachments.push_back(depthAttachment);

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
	render_pass_info.pAttachments = attachments.data();
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	render_pass_info.dependencyCount = 1;
	render_pass_info.pDependencies = &dependency;

	/*A render pass represents a collection of attachments, subpasses, and dependencies between the subpasses,
	and describes how the attachments are used over the course of the subpasses.*/
	if (vkCreateRenderPass(g_vk.device, &render_pass_info, nullptr, &o_renderPass->vk_renderpass) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass!");

	o_renderPass->colorFormats = colorFormats;
	o_renderPass->depthFormat = depthFormat;
}

void CreateLastRenderPass(const std::vector<VkFormat>& colorFormats, const VkFormat depthFormat, RenderPass * o_renderPass)
{
	//Attachements
	std::vector<VkAttachmentDescription> color_attachements;
	std::vector<VkAttachmentReference> color_attachement_refs;
	color_attachements.resize(colorFormats.size());
	color_attachement_refs.resize(colorFormats.size());
	for (size_t i = 0; i < color_attachements.size(); ++i) {
		VkAttachmentDescription& color_attachement = color_attachements[i];
		color_attachement = {};
		color_attachement.format = colorFormats[i];
		color_attachement.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachement.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		color_attachement.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachement.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachement.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachement.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		color_attachement.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; //TODO; make some kind of chain that automatically compute the right layout

		VkAttachmentReference& color_attachement_ref = color_attachement_refs[i];
		color_attachement_ref = {};
		color_attachement_ref.attachment = static_cast<uint32_t>(i);
		color_attachement_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = static_cast<uint32_t>(colorFormats.size());
	subpass.pColorAttachments = color_attachement_refs.data();

	VkAttachmentDescription depthAttachment = {};
	if (depthFormat != VK_FORMAT_UNDEFINED)
	{
		depthAttachment.format = depthFormat;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = static_cast<uint32_t>(colorFormats.size()); //Take the last spot
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		subpass.pDepthStencilAttachment = &depthAttachmentRef;
	}
	else
	{
		subpass.pDepthStencilAttachment = VK_NULL_HANDLE;
	}

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::vector<VkAttachmentDescription> attachments = color_attachements;

	if (depthFormat != VK_FORMAT_UNDEFINED)
		attachments.push_back(depthAttachment);

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
	render_pass_info.pAttachments = attachments.data();
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	render_pass_info.dependencyCount = 1;
	render_pass_info.pDependencies = &dependency;

	/*A render pass represents a collection of attachments, subpasses, and dependencies between the subpasses,
	and describes how the attachments are used over the course of the subpasses.*/
	if (vkCreateRenderPass(g_vk.device, &render_pass_info, nullptr, &o_renderPass->vk_renderpass) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass!");

	o_renderPass->colorFormats = colorFormats;
	o_renderPass->depthFormat = depthFormat;
}

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
	if(hasDepth)
		clearValues[colorCount] = { 1.0f, 0 };
	render_pass_info.clearValueCount = totalCount;
	render_pass_info.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
}

void EndRenderPass(VkCommandBuffer commandBuffer)
{
	vkCmdEndRenderPass(commandBuffer);
}