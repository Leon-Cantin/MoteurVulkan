#include "frame_graph.h"

#include "framebuffer.h"
#include "vk_image.h"
#include "vk_framework.h"
#include "vk_debug.h"

#include <vector>
#include <map>

GfxImage render_targets[RT_COUNT];

constexpr VkFormat RT_FORMAT_SHADOW_DEPTH = VK_FORMAT_D32_SFLOAT;
constexpr VkExtent2D RT_EXTENT_SHADOW = { 1024, 1024 };

const GfxImage* GetRenderTarget(eRenderTarget render_target_id)
{
	return &render_targets[render_target_id];
}

constexpr uint32_t MAX_ATTACHMENTS_COUNT = 8;
constexpr uint32_t MAX_READ_TARGETS = 4;

struct FG_Pass_Resource
{	
	eRenderTarget e_render_targets[MAX_ATTACHMENTS_COUNT];
	VkAttachmentDescription descriptions [MAX_ATTACHMENTS_COUNT];
	VkAttachmentReference references [MAX_ATTACHMENTS_COUNT];
	uint32_t attachmentCount = 0;

	eRenderTarget read_targets[MAX_READ_TARGETS];
	uint32_t read_targets_count = 0;
};

struct FG_RT_Resources
{
	eRenderTarget id;
	VkFormat format;
	VkExtent2D extent;
	VkImageUsageFlagBits usage_flags;
	VkImageAspectFlagBits aspect_flags;
	VkImageLayout image_layout;
	bool skipImageCreation = false;
};

void CreateImage(VkFormat format, VkExtent2D extent, GfxImage* image, VkImageUsageFlagBits usage_flags, VkImageAspectFlagBits aspect_flags, VkImageLayout image_layout)
{
	image->extent = extent;
	image->format = format;
	create_image(extent.width, extent.height, 1, format, usage_flags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image->image, image->memory);
	image->imageView = createImageView(image->image, format, aspect_flags, 1);
	transitionImageLayout( image->image, format, VK_IMAGE_LAYOUT_UNDEFINED, image_layout, 1, 1);
}

void CreateRTCommon(FG_Pass_Resource& resource, VkFormat format, eRenderTarget render_target, VkImageLayout optimalLayout)
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

void CreateColor( FG_Pass_Resource& resource, VkFormat format, eRenderTarget render_target)
{	
	CreateRTCommon(resource, format, render_target, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}

void CreateDepth(FG_Pass_Resource& resource, VkFormat format, eRenderTarget render_target)
{
	CreateRTCommon(resource, format, render_target, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void ClearLast( FG_Pass_Resource& resource)
{
	assert(resource.attachmentCount > 0);

	resource.descriptions[resource.attachmentCount-1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
}

void TransitionToReadOnly(FG_Pass_Resource& resource)
{
	assert(resource.attachmentCount > 0);

	resource.descriptions[resource.attachmentCount - 1].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void ReadResource(FG_Pass_Resource& resource, eRenderTarget render_target)
{
	assert(resource.read_targets_count < MAX_READ_TARGETS);
	resource.read_targets[resource.read_targets_count++] = render_target;
}

static int32_t FindResourceIndex(const FG_Pass_Resource& pass, eRenderTarget render_target)
{
	for (int32_t i = 0; i < pass.attachmentCount; ++i)
	{
		if (pass.e_render_targets[i] == render_target)
			return i;
	}

	return -1;
}

void ComposeGraph( std::vector<FG_Pass_Resource>& passes, FG_RT_Resources* rt_resources)
{
	std::map< eRenderTarget, FG_Pass_Resource*> lastPass;

	for(uint32_t i = 0; i < passes.size(); ++i)
	{
		FG_Pass_Resource& pass = passes[i];

		//RenderTargets
		for (uint32_t resource_index = 0; resource_index < pass.attachmentCount; ++resource_index)
		{
			eRenderTarget render_target = pass.e_render_targets[resource_index];
			VkAttachmentDescription& description = pass.descriptions[resource_index];
			VkAttachmentReference& reference = pass.references[resource_index];
			
			auto it_found = lastPass.find(render_target);
			if (it_found == lastPass.end())
			{
				lastPass[render_target] = &pass;
				
				//Create a new image when we encounter it for the first time.
				FG_RT_Resources* rt_resource = &rt_resources[render_target];
				if (!rt_resource->skipImageCreation)
				{
					GfxImage* rt_image = &render_targets[render_target];					
					CreateImage(rt_resource->format, rt_resource->extent, rt_image, rt_resource->usage_flags, rt_resource->aspect_flags, rt_resource->image_layout);
				}
			}
			else
			{				
				FG_Pass_Resource* lastPassWithResource = it_found->second;
				int32_t otherPassReferenceIndex = FindResourceIndex(*lastPassWithResource, render_target);
				//The last pass will have to transition into this pass' layout
				lastPassWithResource->descriptions[otherPassReferenceIndex].finalLayout = reference.layout;
				//This pass' initial layout should be this one's layout
				description.initialLayout = reference.layout;
				//We should load since the other pass wrote into this
				description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

				lastPass[render_target] = &pass;
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
				FG_Pass_Resource* lastPassWithResource = it_found->second;
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

	//Transition the last pass with RT_SCENE to present
	auto it_found = lastPass.find(RT_SCENE_COLOR);
	FG_Pass_Resource* lastPassWithResource = it_found->second;
	int32_t otherPassReferenceIndex = FindResourceIndex(*lastPassWithResource, RT_SCENE_COLOR);
	lastPassWithResource->descriptions[otherPassReferenceIndex].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
}

void CreateRenderPass(const FG_Pass_Resource& pass_resource, const char* name, RenderPass* o_renderPass)
{
	assert(pass_resource.attachmentCount > 0);
	bool containsDepth = pass_resource.references[pass_resource.attachmentCount - 1].layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	uint32_t colorCount = pass_resource.attachmentCount - (containsDepth ? 1 : 0);

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = colorCount;
	subpass.pColorAttachments = pass_resource.references;
	subpass.pDepthStencilAttachment = containsDepth ? &pass_resource.references[colorCount] : VK_NULL_HANDLE;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = pass_resource.attachmentCount;
	render_pass_info.pAttachments = pass_resource.descriptions;
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
		o_renderPass->colorFormats[i] = pass_resource.descriptions[i].format;
	o_renderPass->depthFormat = containsDepth ? pass_resource.descriptions[colorCount].format : VK_FORMAT_UNDEFINED;

	MarkVkObject((uint64_t)o_renderPass->vk_renderpass, VK_OBJECT_TYPE_RENDER_PASS, name);

	//TODO: SO FAR WE ONLY DO THIS FOR THE SHADOW PASS. PLEASE DO THIS FOR EVERYTHING
	if (pass_resource.attachmentCount == 1)
	{
		GfxImage* depthImage = &render_targets[pass_resource.e_render_targets[0]];
		createFrameBuffer(nullptr, 0, depthImage, RT_EXTENT_SHADOW, o_renderPass->vk_renderpass, &o_renderPass->frameBuffer);
	}
}

void CreateGraph(VkFormat swapchainFormat, std::vector<RenderPass>* o_renderPasses)
{
	std::vector<FG_Pass_Resource> passes;
	FG_RT_Resources rt_resources[RT_COUNT];
	rt_resources[RT_SCENE_COLOR].skipImageCreation = true;
	rt_resources[RT_SCENE_DEPTH].skipImageCreation = true;
	rt_resources[RT_SHADOW_MAP] = { RT_SHADOW_MAP, RT_FORMAT_SHADOW_DEPTH , RT_EXTENT_SHADOW, (VkImageUsageFlagBits)(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT), VK_IMAGE_ASPECT_DEPTH_BIT,  VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	FG_Pass_Resource shadowPass;
	CreateDepth(shadowPass, VK_FORMAT_D32_SFLOAT, RT_SHADOW_MAP);
	ClearLast(shadowPass);
	passes.push_back(shadowPass);

	FG_Pass_Resource geoPass;
	CreateColor(geoPass, swapchainFormat, RT_SCENE_COLOR);
	CreateDepth(geoPass, VK_FORMAT_D32_SFLOAT, RT_SCENE_DEPTH);
	ClearLast(geoPass);
	ReadResource(geoPass, RT_SHADOW_MAP);
	passes.push_back(geoPass);

	FG_Pass_Resource skyPass;
	CreateColor(skyPass, swapchainFormat, RT_SCENE_COLOR);
	CreateDepth(skyPass, VK_FORMAT_D32_SFLOAT, RT_SCENE_DEPTH);
	passes.push_back(skyPass);

	FG_Pass_Resource textPass;
	CreateColor(textPass, swapchainFormat, RT_SCENE_COLOR);
	//TODO currently using the depth just for the renderpass to be compatible with the rest. should remove that
	CreateDepth(textPass, VK_FORMAT_D32_SFLOAT, RT_SCENE_DEPTH);
	passes.push_back(textPass);

	ComposeGraph(passes, rt_resources);

	RenderPass shadowPassF;
	RenderPass geoPassF;
	RenderPass skyPassF;
	RenderPass textPassF;
	CreateRenderPass(passes[0], "shadow_pass", &shadowPassF);
	CreateRenderPass(passes[1], "geometry_pass", &geoPassF);
	CreateRenderPass(passes[2], "skybox_pass", &skyPassF);
	CreateRenderPass(passes[3], "text_pass", &textPassF);

	o_renderPasses->push_back(shadowPassF);
	o_renderPasses->push_back(geoPassF);
	o_renderPasses->push_back(skyPassF);
	o_renderPasses->push_back(textPassF);
}

void FG_CleanupResources()
{
	for (GfxImage& image : render_targets)
	{
		if (image.image)
			DestroyImage(image);
	}
}