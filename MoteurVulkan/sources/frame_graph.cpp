#include "frame_graph.h"

#include "vk_framework.h"
#include "vk_debug.h"

#include <vector>
#include <map>

enum eRenderTarget
{
	RT_SCENE_COLOR,
	RT_SCENE_DEPTH,
	RT_SHADOW_MAP
};

constexpr uint32_t MAX_ATTACHMENTS_COUNT = 8;
constexpr uint32_t MAX_READ_TARGETS = 4;
//TODO: ajouter une liste de noms string ou enum
struct FG_Pass_Resource
{	
	eRenderTarget e_render_targets[MAX_ATTACHMENTS_COUNT];
	VkAttachmentDescription descriptions [MAX_ATTACHMENTS_COUNT];
	VkAttachmentReference references [MAX_ATTACHMENTS_COUNT];
	uint32_t attachmentCount = 0;

	eRenderTarget read_targets[MAX_READ_TARGETS];
	uint32_t read_targets_count = 0;
};

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

void AddReadResource(FG_Pass_Resource& resource, VkFormat format, eRenderTarget render_target)
{
	assert(resource.attachmentCount < MAX_ATTACHMENTS_COUNT);

	const uint32_t attachement_id = resource.attachmentCount;
	VkAttachmentDescription& description = resource.descriptions[attachement_id];
	description.format = format;
	description.flags = 0;
	description.samples = VK_SAMPLE_COUNT_1_BIT;
	description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; //important
	description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	description.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;//important

	VkAttachmentReference& reference = resource.references[attachement_id];
	reference.attachment = attachement_id;
	reference.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;//important

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

void TransitionToPresent(FG_Pass_Resource& resource)
{
	assert(resource.attachmentCount > 0);

	resource.descriptions[resource.attachmentCount - 1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
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

void ComposeGraph( std::vector<FG_Pass_Resource>& passes)
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
			}
			else
			{				
				FG_Pass_Resource* lastPassWithResource = it_found->second;
				int32_t otherPassReferenceIndex = FindResourceIndex(*lastPassWithResource, render_target);
				//The last pass will have to transition into this pass' layout
				lastPassWithResource->descriptions[otherPassReferenceIndex].finalLayout = reference.layout;
				//This pass' initial layout should be this one's
				description.initialLayout = reference.layout;
				//We should load since the other pass wrote into this
				description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

				lastPass[render_target] = &pass;
			}
		}

		//read resources
		for (uint32_t resource_index = 0; resource_index < pass.read_targets_count; ++resource_index)
		{
			eRenderTarget render_target = pass.read_targets[resource_index];
			auto it_found = lastPass.find(render_target);
			if (it_found != lastPass.end())
			{
				FG_Pass_Resource* lastPassWithResource = it_found->second;
				int32_t otherPassReferenceIndex = FindResourceIndex(*lastPassWithResource, render_target);
				//Last pass should transition to read resource at the end
				lastPassWithResource->descriptions[otherPassReferenceIndex].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				//Add an attachment as read only
				AddReadResource(pass, lastPassWithResource->descriptions[otherPassReferenceIndex].format, render_target);
				lastPass[render_target] = &pass;
			}
			else
			{
				throw std::runtime_error("This render pass does not exist already!");
			}
		}
	}
}

void CreateRenderPass(const FG_Pass_Resource& pass_resource, const char* name, RenderPass* o_renderPass)
{
	assert(pass_resource.attachmentCount > 0);
	bool containsDepth = pass_resource.references[pass_resource.attachmentCount - 1 - pass_resource.read_targets_count].layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	uint32_t colorCount = pass_resource.attachmentCount - (containsDepth ? 1 : 0) - pass_resource.read_targets_count;

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
}

void CreateGraph(VkFormat swapchainFormat, std::vector<RenderPass>* o_renderPasses)
{
	std::vector<FG_Pass_Resource> passes;

	FG_Pass_Resource shadowPass;
	CreateDepth(shadowPass, VK_FORMAT_D32_SFLOAT, RT_SHADOW_MAP);
	ClearLast(shadowPass);
	//TODO: something better, mark the next one as reading this RT instead.
	TransitionToReadOnly(shadowPass);
	passes.push_back(shadowPass);

	FG_Pass_Resource geoPass;
	CreateColor(geoPass, swapchainFormat, RT_SCENE_COLOR);
	CreateDepth(geoPass, VK_FORMAT_D32_SFLOAT, RT_SCENE_DEPTH);
	ClearLast(geoPass);
	//ReadResource(geoPass, RT_SHADOW_MAP);
	passes.push_back(geoPass);

	FG_Pass_Resource skyPass;
	CreateColor(skyPass, swapchainFormat, RT_SCENE_COLOR);
	CreateDepth(skyPass, VK_FORMAT_D32_SFLOAT, RT_SCENE_DEPTH);
	passes.push_back(skyPass);

	FG_Pass_Resource textPass;
	CreateColor(textPass, swapchainFormat, RT_SCENE_COLOR);
	//TODO: something better, don't explicitly transition
	TransitionToPresent(textPass);
	//TODO currently using the depth just to be compatible with the rest. should remove that
	CreateDepth(textPass, VK_FORMAT_D32_SFLOAT, RT_SCENE_DEPTH);
	passes.push_back(textPass);

	ComposeGraph(passes);

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