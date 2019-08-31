#include "frame_graph.h"

#include "framebuffer.h"
#include "vk_image.h"
#include "vk_debug.h"

#include <vector>
#include <map>

namespace FG
{
	GfxImage _output_buffers[SIMULTANEOUS_FRAMES];

	constexpr size_t MAX_RENDERTARGETS = 8;
	size_t RENDERTARGETS_COUNT;
	uint32_t RT_OUTPUT_TARGET;
	GfxImage _render_targets[MAX_RENDERTARGETS];


	const GfxImage* GetRenderTarget(uint32_t render_target_id)
	{
		return &_render_targets[render_target_id];
	}

	std::array<RenderPass, 8> _render_passes;
	uint32_t _render_passes_count = 0;

	std::array<Technique, 8> _techniques;
	uint32_t _techniques_count = 0;

	const RenderPass* GetRenderPass(uint32_t id)
	{
		return &_render_passes[id];
	}

	std::vector<RenderTargetCreationData> _rtCreationData;
	std::vector<RenderPassCreationData> _rpCreationData;

	void CreateImage(VkFormat format, VkExtent2D extent, GfxImage* image, VkImageUsageFlagBits usage_flags, VkImageAspectFlagBits aspect_flags, VkImageLayout image_layout)
	{
		image->extent = extent;
		image->format = format;
		create_image(extent.width, extent.height, 1, format, usage_flags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image->image, image->memory);
		image->imageView = createImageView(image->image, format, aspect_flags, 1);
		transitionImageLayout(image->image, format, VK_IMAGE_LAYOUT_UNDEFINED, image_layout, 1, 1);
	}

	void CreateRTCommon(RenderPassCreationData& resource, VkFormat format, uint32_t render_target, VkImageLayout optimalLayout)
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

	void CreateColor(RenderPassCreationData& resource, VkFormat format, uint32_t render_target)
	{
		CreateRTCommon(resource, format, render_target, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}

	void CreateDepth(RenderPassCreationData& resource, VkFormat format, uint32_t render_target)
	{
		CreateRTCommon(resource, format, render_target, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}

	void ClearLast(RenderPassCreationData& resource)
	{
		assert(resource.attachmentCount > 0);

		resource.descriptions[resource.attachmentCount - 1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	}

	void TransitionToReadOnly(RenderPassCreationData& resource)
	{
		assert(resource.attachmentCount > 0);

		resource.descriptions[resource.attachmentCount - 1].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	void ReadResource(RenderPassCreationData& resource, uint32_t render_target)
	{
		assert(resource.read_targets_count < MAX_READ_TARGETS);
		resource.read_targets[resource.read_targets_count++] = render_target;
	}

	static int32_t FindResourceIndex(const RenderPassCreationData& pass, uint32_t render_target)
	{
		for (int32_t i = 0; i < pass.attachmentCount; ++i)
		{
			if (pass.e_render_targets[i] == render_target)
				return i;
		}

		return -1;
	}

	static void ComposeGraph(std::vector<RenderPassCreationData>& passesCreationData, std::vector<RenderTargetCreationData>& rtCreationDatas)
	{
		std::map< uint32_t, RenderPassCreationData*> lastPass;

		for (uint32_t i = 0; i < passesCreationData.size(); ++i)
		{
			RenderPassCreationData& pass = passesCreationData[i];

			//RenderTargets
			for (uint32_t resource_index = 0; resource_index < pass.attachmentCount; ++resource_index)
			{
				uint32_t renderTargetId = pass.e_render_targets[resource_index];
				VkAttachmentDescription& description = pass.descriptions[resource_index];
				VkAttachmentReference& reference = pass.references[resource_index];

				auto it_found = lastPass.find(renderTargetId);
				if (it_found == lastPass.end())
				{
					lastPass[renderTargetId] = &pass;

					//Create a new image when we encounter it for the first time.
					RenderTargetCreationData* rtCreationData = &rtCreationDatas[renderTargetId];
					if (renderTargetId != RT_OUTPUT_TARGET)
					{
						GfxImage* rt_image = &_render_targets[renderTargetId];
						CreateImage(rtCreationData->format, rtCreationData->extent, rt_image, rtCreationData->usage_flags, rtCreationData->aspect_flags, rtCreationData->image_layout);
					}
				}
				else
				{
					RenderPassCreationData* lastPassWithResource = it_found->second;
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
				uint32_t render_target = pass.read_targets[resource_index];
				auto it_found = lastPass.find(render_target);
				if (it_found != lastPass.end())
				{
					RenderPassCreationData* lastPassWithResource = it_found->second;
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
		RenderPassCreationData* lastPassWithResource = it_found->second;
		int32_t otherPassReferenceIndex = FindResourceIndex(*lastPassWithResource, RT_OUTPUT_TARGET);
		lastPassWithResource->descriptions[otherPassReferenceIndex].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}

	static void CreateFrameBuffer(RenderPass* renderpass, const RenderPassCreationData& passCreationData, uint32_t colorCount, bool containsDepth)
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

	static void CreateRenderPass(const RenderPassCreationData& passCreationData, const char* name, RenderPass* o_renderPass)
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
		_render_targets[RT_OUTPUT_TARGET] = swapchain->images[0];
		for (uint32_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
			_output_buffers[i] = swapchain->images[i];

		for (uint32_t i = 0; i < RENDERTARGETS_COUNT; ++i)
		{
			if (_rtCreationData[i].swapChainSized)
				_rtCreationData[i].extent = swapchain->extent;
		}
	}

	void CreateGraph(const Swapchain* swapchain, std::vector<RenderPassCreationData> *inRpCreationData, std::vector<RenderTargetCreationData> *inRtCreationData, uint32_t backbufferId, VkDescriptorPool descriptorPool,
		void(*createTechniqueCallback)(const RenderPass*, const RenderPassCreationData*, Technique*) )
	{
		//Setup resources
		RENDERTARGETS_COUNT = inRtCreationData->size();
		RT_OUTPUT_TARGET = backbufferId;
		_rtCreationData = *inRtCreationData;
		CreateResourceCreationData(swapchain);

		//Setup passes
		_rpCreationData = *inRpCreationData;

		ComposeGraph(_rpCreationData, _rtCreationData);

		for (uint32_t i = 0; i < _rpCreationData.size(); ++i)
		{
			//Create the pass
			RenderPassCreationData* rpCreationData = &_rpCreationData[i];
			CreateRenderPass( *rpCreationData, rpCreationData->name, &_render_passes[_render_passes_count++] );

			//Create the descriptor set and layout
			createTechniqueCallback( GetRenderPass( i ), rpCreationData, &_techniques[_techniques_count++] );
		}
	}

	void Cleanup()
	{
		for (uint32_t i = 0; i < RENDERTARGETS_COUNT; ++i)
		{
			GfxImage& image = _render_targets[i];
			if (image.image && i != RT_OUTPUT_TARGET)
				DestroyImage(image);
		}
		RENDERTARGETS_COUNT = 0;

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
		_render_passes_count = 0;

		//Cleanup technique
		for( uint32_t i = 0; i < _techniques_count; ++i )
		{
			Technique& technique = _techniques[i];
			vkDestroyPipeline( g_vk.device, technique.pipeline, nullptr );
			vkDestroyPipelineLayout( g_vk.device, technique.pipelineLayout, nullptr );
			vkDestroyDescriptorSetLayout( g_vk.device, technique.renderpass_descriptor_layout, nullptr );
			vkDestroyDescriptorSetLayout( g_vk.device, technique.instance_descriptor_layout, nullptr );
			for( uint32_t j = 0; j < technique.instance_descriptor.size(); ++j )
				if( technique.instance_descriptor[j] )
					vkFreeDescriptorSets( g_vk.device, technique.parentDescriptorPool, 1, &technique.instance_descriptor[j] );
			for( uint32_t j = 0; j < technique.renderPass_descriptor.size(); ++j )
				if( technique.renderPass_descriptor[j] )
					vkFreeDescriptorSets( g_vk.device, technique.parentDescriptorPool, 1, &technique.renderPass_descriptor[j] );
			technique = {};
		}
		_techniques_count = 0;
	}

	void RecordDrawCommands(uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent)
	{
		for (uint32_t i = 0; i < _rpCreationData.size(); ++i)
		{
			//TODO: _render_passes[i] _techniques[i] doesn't mean is the right one
			_rpCreationData[i].frame_graph_node.RecordDrawCommands(currentFrame, frameData, graphicsCommandBuffer, extent, &_render_passes[i], &_techniques[i]);
		}
	}
}