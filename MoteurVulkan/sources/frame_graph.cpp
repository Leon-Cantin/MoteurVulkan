#include "frame_graph.h"

#include "framebuffer.h"
#include "vk_image.h"
#include "vk_framework.h"
#include "vk_debug.h"

#include "geometry_renderpass.h"
#include "shadow_renderpass.h"
#include "skybox.h"
#include "text_overlay.h"

#include <vector>
#include <map>

GfxImage _output_buffers[SIMULTANEOUS_FRAMES];
GfxImage _render_targets[RT_COUNT];

constexpr VkFormat RT_FORMAT_SHADOW_DEPTH = VK_FORMAT_D32_SFLOAT;
constexpr VkExtent2D RT_EXTENT_SHADOW = { 1024, 1024 };
constexpr eRenderTarget RT_OUTPUT_TARGET = RT_SCENE_COLOR;

const GfxImage* GetRenderTarget(eRenderTarget render_target_id)
{
	return &_render_targets[render_target_id];
}

std::array<RenderPass, 8> _render_passes;
uint32_t _render_passes_count = 0;

const RenderPass* GetRenderPass(uint32_t id)
{
	return &_render_passes[id];
}

struct FG_RenderTargetCreationData
{
	eRenderTarget id;
	VkFormat format;
	VkExtent2D extent;
	VkImageUsageFlagBits usage_flags;
	VkImageAspectFlagBits aspect_flags;
	VkImageLayout image_layout;
	bool swapChainSized = false;
};

FG_RenderTargetCreationData _rtCreationData[RT_COUNT];
std::vector<FG_RenderPassCreationData> _rpCreationData;

void CreateImage(VkFormat format, VkExtent2D extent, GfxImage* image, VkImageUsageFlagBits usage_flags, VkImageAspectFlagBits aspect_flags, VkImageLayout image_layout)
{
	image->extent = extent;
	image->format = format;
	create_image(extent.width, extent.height, 1, format, usage_flags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image->image, image->memory);
	image->imageView = createImageView(image->image, format, aspect_flags, 1);
	transitionImageLayout( image->image, format, VK_IMAGE_LAYOUT_UNDEFINED, image_layout, 1, 1);
}

void CreateRTCommon(FG_RenderPassCreationData& resource, VkFormat format, eRenderTarget render_target, VkImageLayout optimalLayout)
{
	assert(resource.attachmentCount < MAX_ATTACHMENTS_COUNT);

	const uint32_t attachement_id = resource.attachmentCount;
	VkAttachmentDescription& description = resource.descriptions[attachement_id];
	description.format = format;
	description.flags = 0;
	description.samples = VK_SAMPLE_COUNT_1_BIT;
	description.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	description.finalLayout = optimalLayout;

	VkAttachmentReference& reference = resource.references[attachement_id];
	reference.attachment = attachement_id;
	reference.layout = optimalLayout;

	resource.e_render_targets[attachement_id] = render_target;

	++resource.attachmentCount;
}

void FG_CreateColor( FG_RenderPassCreationData& resource, VkFormat format, eRenderTarget render_target)
{	
	CreateRTCommon(resource, format, render_target, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}

void FG_CreateDepth(FG_RenderPassCreationData& resource, VkFormat format, eRenderTarget render_target)
{
	CreateRTCommon(resource, format, render_target, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void FG_ClearLast( FG_RenderPassCreationData& resource)
{
	assert(resource.attachmentCount > 0);

	resource.descriptions[resource.attachmentCount-1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
}

void TransitionToReadOnly(FG_RenderPassCreationData& resource)
{
	assert(resource.attachmentCount > 0);

	resource.descriptions[resource.attachmentCount - 1].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void FG_ReadResource(FG_RenderPassCreationData& resource, eRenderTarget render_target)
{
	assert(resource.read_targets_count < MAX_READ_TARGETS);
	resource.read_targets[resource.read_targets_count++] = render_target;
}

static int32_t FindResourceIndex(const FG_RenderPassCreationData& pass, eRenderTarget render_target)
{
	for (int32_t i = 0; i < pass.attachmentCount; ++i)
	{
		if (pass.e_render_targets[i] == render_target)
			return i;
	}

	return -1;
}

void ComposeGraph( std::vector<FG_RenderPassCreationData>& passesCreationData, FG_RenderTargetCreationData* rtCreationDatas)
{
	std::map< eRenderTarget, FG_RenderPassCreationData*> lastPass;

	for(uint32_t i = 0; i < passesCreationData.size(); ++i)
	{
		FG_RenderPassCreationData& pass = passesCreationData[i];

		//RenderTargets
		for (uint32_t resource_index = 0; resource_index < pass.attachmentCount; ++resource_index)
		{
			eRenderTarget renderTargetId = pass.e_render_targets[resource_index];
			VkAttachmentDescription& description = pass.descriptions[resource_index];
			VkAttachmentReference& reference = pass.references[resource_index];
			
			auto it_found = lastPass.find(renderTargetId);
			if (it_found == lastPass.end())
			{
				lastPass[renderTargetId] = &pass;
				
				//Create a new image when we encounter it for the first time.
				FG_RenderTargetCreationData* rtCreationData = &rtCreationDatas[renderTargetId];
				if (renderTargetId != RT_OUTPUT_TARGET)
				{
					GfxImage* rt_image = &_render_targets[renderTargetId];					
					CreateImage(rtCreationData->format, rtCreationData->extent, rt_image, rtCreationData->usage_flags, rtCreationData->aspect_flags, rtCreationData->image_layout);
				}
			}
			else
			{				
				FG_RenderPassCreationData* lastPassWithResource = it_found->second;
				int32_t otherPassReferenceIndex = FindResourceIndex(*lastPassWithResource, renderTargetId);
				//The last pass will have to transition into this pass' layout
				lastPassWithResource->descriptions[otherPassReferenceIndex].finalLayout = reference.layout;
				//This pass' initial layout should be this one's layout
				description.initialLayout = reference.layout;
				//We should load since the other pass wrote into this
				description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

				lastPass[renderTargetId] = &pass;
			}
		}

		/*
		read resources
		Look at read resources and change the the final layout of the last appearence to be used
		as shader resource
		*/
		for (uint32_t resource_index = 0; resource_index < pass.read_targets_count; ++resource_index)
		{
			eRenderTarget render_target = pass.read_targets[resource_index];
			auto it_found = lastPass.find(render_target);
			if (it_found != lastPass.end())
			{
				FG_RenderPassCreationData* lastPassWithResource = it_found->second;
				int32_t otherPassReferenceIndex = FindResourceIndex(*lastPassWithResource, render_target);
				if (otherPassReferenceIndex >= 0)
				{
					//Last pass should transition to read resource at the end
					lastPassWithResource->descriptions[otherPassReferenceIndex].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				}
				else
				{
					//We are already in a read state
				}
				lastPass[render_target] = &pass;
			}
			else
			{
				throw std::runtime_error("This render pass does not exist already!");
			}
		}
	}

	//Transition the last pass with RT_OUTPUT_TARGET to present
	auto it_found = lastPass.find(RT_OUTPUT_TARGET);
	FG_RenderPassCreationData* lastPassWithResource = it_found->second;
	int32_t otherPassReferenceIndex = FindResourceIndex(*lastPassWithResource, RT_OUTPUT_TARGET);
	lastPassWithResource->descriptions[otherPassReferenceIndex].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
}

static void CreateFrameBuffer(RenderPass* renderpass, const FG_RenderPassCreationData& passCreationData, uint32_t colorCount, bool containsDepth)
{
	int32_t outputBufferIndex = FindResourceIndex(passCreationData, RT_OUTPUT_TARGET);
	VkExtent2D extent = _render_targets[passCreationData.e_render_targets[0]].extent;
	GfxImage colorImages[MAX_ATTACHMENTS_COUNT];
	for (uint32_t i = 0; i < colorCount; ++i)
		colorImages[i] = _render_targets[passCreationData.e_render_targets[i]];
	GfxImage* depthImage = containsDepth ? &_render_targets[passCreationData.e_render_targets[colorCount]] : nullptr;

	if (outputBufferIndex < 0)
	{
		createFrameBuffer(colorImages, colorCount, depthImage, extent, renderpass->vk_renderpass, &renderpass->frameBuffer);
	}
	else //If we use the output buffer create one frame buffer per possible image
	{		
		for (uint32_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		{
			colorImages[outputBufferIndex] = _output_buffers[i];
			createFrameBuffer(colorImages, colorCount, depthImage, extent, renderpass->vk_renderpass, &renderpass->outputFrameBuffer[i]);
		}
	}
}

static void CreateRenderPass(const FG_RenderPassCreationData& passCreationData, const char* name, RenderPass* o_renderPass)
{
	assert(passCreationData.attachmentCount > 0);
	bool containsDepth = passCreationData.references[passCreationData.attachmentCount - 1].layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	uint32_t colorCount = passCreationData.attachmentCount - (containsDepth ? 1 : 0);

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = colorCount;
	subpass.pColorAttachments = passCreationData.references;
	subpass.pDepthStencilAttachment = containsDepth ? &passCreationData.references[colorCount] : VK_NULL_HANDLE;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = passCreationData.attachmentCount;
	render_pass_info.pAttachments = passCreationData.descriptions;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	render_pass_info.dependencyCount = 1;
	render_pass_info.pDependencies = &dependency;

	/*A render pass represents a collection of attachments, subpasses, and dependencies between the subpasses,
	and describes how the attachments are used over the course of the subpasses.*/
	if (vkCreateRenderPass(g_vk.device, &render_pass_info, nullptr, &o_renderPass->vk_renderpass) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass!");

	o_renderPass->colorFormats.resize(colorCount);
	for (uint32_t i = 0; i < colorCount; ++i)
		o_renderPass->colorFormats[i] = passCreationData.descriptions[i].format;
	o_renderPass->depthFormat = containsDepth ? passCreationData.descriptions[colorCount].format : VK_FORMAT_UNDEFINED;

	MarkVkObject((uint64_t)o_renderPass->vk_renderpass, VK_OBJECT_TYPE_RENDER_PASS, name);
	
	//Create the frame buffer of the render pass
	CreateFrameBuffer(o_renderPass, passCreationData, colorCount, containsDepth);
}

static void CreateResourceCreationData(const Swapchain* swapchain)
{
	//setup hacks for outside resources
	VkFormat swapchainFormat = swapchain->surfaceFormat.format;
	VkExtent2D swapchainExtent = swapchain->extent;
	_render_targets[RT_SCENE_COLOR] = swapchain->images[0];
	for (uint32_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		_output_buffers[i] = swapchain->images[i];

	//Setup resources
	_rtCreationData[RT_SCENE_COLOR] = { RT_SCENE_COLOR, swapchainFormat , swapchainExtent, (VkImageUsageFlagBits)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT), VK_IMAGE_ASPECT_COLOR_BIT,  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, true };
	_rtCreationData[RT_SCENE_DEPTH] = { RT_SCENE_DEPTH, VK_FORMAT_D32_SFLOAT , swapchainExtent, (VkImageUsageFlagBits)(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT), VK_IMAGE_ASPECT_DEPTH_BIT,  VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, true };
	_rtCreationData[RT_SHADOW_MAP] = { RT_SHADOW_MAP, RT_FORMAT_SHADOW_DEPTH , RT_EXTENT_SHADOW, (VkImageUsageFlagBits)(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT), VK_IMAGE_ASPECT_DEPTH_BIT,  VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
}

void FG_CreateGraph(const Swapchain* swapchain, std::vector<FG_RenderPassCreationData> *inRpCreationData)
{
	VkFormat swapchainFormat = swapchain->surfaceFormat.format;
	CreateResourceCreationData(swapchain);

	//Setup passes
	_rpCreationData = *inRpCreationData;

	ComposeGraph(_rpCreationData, _rtCreationData);
	
	for (uint32_t i = 0; i < _rpCreationData.size(); ++i)
	{
		CreateRenderPass(_rpCreationData[i], _rpCreationData[i].name, &_render_passes[_render_passes_count++]);
		_rpCreationData[i].frame_graph_node.Initialize(GetRenderPass(i), swapchain);
	}
}

void FG_RecreateAfterSwapchain(const Swapchain* swapchain)
{
	CreateResourceCreationData(swapchain);

	//Recreate those are not the frame buffer nor sampled
	for (uint32_t i = 0; i < RT_COUNT; ++i)
	{
		if ((eRenderTarget)(i) != RT_OUTPUT_TARGET && _rtCreationData[i].swapChainSized)
		{
			FG_RenderTargetCreationData* rtCreationData = &_rtCreationData[i];
			CreateImage(rtCreationData->format, rtCreationData->extent, &_render_targets[i], rtCreationData->usage_flags, rtCreationData->aspect_flags, rtCreationData->image_layout);
		}
	}

	//Recreate the frame buffers
	for (uint32_t i = 0; i < _render_passes_count; ++i)
	{
		FG_RenderPassCreationData& passCreationData = _rpCreationData[i];
		RenderPass& renderpass = _render_passes[i];

		assert(passCreationData.attachmentCount > 0);
		bool containsDepth = passCreationData.references[passCreationData.attachmentCount - 1].layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		uint32_t colorCount = passCreationData.attachmentCount - (containsDepth ? 1 : 0);

		CreateFrameBuffer(&renderpass, passCreationData, colorCount, containsDepth);
	}


	//Could be avoided with dynamic state, we only need to redo scissor and viewport
	for (uint32_t i = 0; i < _rpCreationData.size(); ++i)
	{
		if (_rpCreationData[i].frame_graph_node.RecreateAfterSwapchain)
			_rpCreationData[i].frame_graph_node.RecreateAfterSwapchain(swapchain);
	}
}

void FG_CleanupAfterSwapchain()
{
	for (uint32_t i = 0; i < _render_passes_count; ++i)
	{
		RenderPass& renderpass = _render_passes[i];
		if (renderpass.frameBuffer.frameBuffer)
		{
			vkDestroyFramebuffer(g_vk.device, renderpass.frameBuffer.frameBuffer, nullptr);
			renderpass.frameBuffer.frameBuffer = VK_NULL_HANDLE;
		}
		for (uint32_t fb_index = 0; fb_index < SIMULTANEOUS_FRAMES; ++fb_index)
		{
			vkDestroyFramebuffer(g_vk.device, renderpass.outputFrameBuffer[fb_index].frameBuffer, nullptr);
			renderpass.outputFrameBuffer[fb_index].frameBuffer = VK_NULL_HANDLE;
		}
	}

	//destroy all images that are not the frame buffer or sampled (to avoid recreating descriptor sets)
	for (uint32_t i = 0; i < RT_COUNT; ++i)
	{
		if( (eRenderTarget)(i) != RT_OUTPUT_TARGET && _rtCreationData[i].swapChainSized )
			DestroyImage(_render_targets[RT_SCENE_DEPTH]);
	}

	for (uint32_t i = 0; i < _rpCreationData.size(); ++i)
	{
		if (_rpCreationData[i].frame_graph_node.CleanupAfterSwapchain)
			_rpCreationData[i].frame_graph_node.CleanupAfterSwapchain();
	}
}

void FG_CleanupResources()
{
	for (uint32_t i = 0; i < _rpCreationData.size(); ++i)
	{
		_rpCreationData[i].frame_graph_node.Cleanup();
	}

	for (uint32_t i = 0; i < RT_COUNT; ++i)
	{
		GfxImage& image = _render_targets[i];
		if (image.image && i != RT_OUTPUT_TARGET)
			DestroyImage(image);
	}

	for (uint32_t i = 0; i < _render_passes_count; ++i)
	{
		RenderPass& renderpass = _render_passes[i];
		if (renderpass.frameBuffer.frameBuffer)
		{
			vkDestroyFramebuffer(g_vk.device, renderpass.frameBuffer.frameBuffer, nullptr);
			renderpass.frameBuffer.frameBuffer = VK_NULL_HANDLE;
		}
		for (uint32_t fb_index = 0; fb_index < SIMULTANEOUS_FRAMES; ++fb_index)
		{
			vkDestroyFramebuffer(g_vk.device, renderpass.outputFrameBuffer[fb_index].frameBuffer, nullptr);
			renderpass.outputFrameBuffer[fb_index].frameBuffer = VK_NULL_HANDLE;
		}
		vkDestroyRenderPass(g_vk.device, renderpass.vk_renderpass, nullptr);
		renderpass.vk_renderpass = VK_NULL_HANDLE;
	}
}

void FG_RecordDrawCommands(uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent)
{
	for (uint32_t i = 0; i < _rpCreationData.size(); ++i)
	{
		_rpCreationData[i].frame_graph_node.RecordDrawCommands(currentFrame, frameData, graphicsCommandBuffer, extent);
	}
}