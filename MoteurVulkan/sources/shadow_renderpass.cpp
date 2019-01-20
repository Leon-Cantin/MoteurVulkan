#include "shadow_renderpass.h"

#include "descriptors.h"
#include "glm/gtc/matrix_transform.hpp"
#include "renderpass.h"
#include "vk_debug.h"
#include "vk_commands.h"
#include "framebuffer.h"
#include "vk_shader.h"
#include "vk_framework.h"
#include "vk_buffer.h"
#include "scene_instance.h"
#include "..\shaders\shadersCommon.h"

#include <vector>
#include <array>

VkDescriptorSetLayout shadowDescriptorSetLayout;
VkDescriptorSetLayout shadowInstanceDescriptorSetLayout;
RenderPass shadowRenderPass;
GfxImage shadowDepthImage;
FrameBuffer shadowFrameBuffer;
const VkFormat shadowDepthFormat = VK_FORMAT_D32_SFLOAT;
const VkExtent2D shadowFrameBufferExtent = { 1024, 1024 };
VkPipelineLayout shadowPipelineLayout;
VkPipeline shadowPipeline;

std::array<VkDescriptorSet, SIMULTANEOUS_FRAMES> shadowDescriptorSets;
std::array<VkDescriptorSet, SIMULTANEOUS_FRAMES> shadowInstanceDescriptorSet;

PerFrameBuffer shadowSceneUniformBuffer;

const GfxImage* GetShadowDepthImage()
{
	return &shadowDepthImage;
}

void computeShadowMatrix(const glm::vec3& light_location, glm::mat4* view, glm::mat4* projection)
{
	*view = glm::lookAt(light_location, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	//TODO: encapsulate projection computation
	*projection = glm::perspective(glm::radians(65.0f), 1.0f, 0.1f, 100.0f);
	(*projection)[1][1] *= -1;//Compensate for OpenGL Y coordinate being inverted

	//return light_projection_matrix * light_view_matrix;
}

void CreateShadowFrameBuffer()
{
	shadowDepthImage.extent = shadowFrameBufferExtent;
	shadowDepthImage.format = shadowDepthFormat;
	create_image(shadowDepthImage.extent.width, shadowDepthImage.extent.height, 1, shadowDepthImage.format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shadowDepthImage.image, shadowDepthImage.memory);
	shadowDepthImage.imageView = createImageView(shadowDepthImage.image, shadowDepthImage.format, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
	transitionImageLayout(shadowDepthImage.image, shadowDepthImage.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 1);

	createFrameBuffer(nullptr, 0, &shadowDepthImage, shadowFrameBufferExtent, shadowRenderPass.vk_renderpass, &shadowFrameBuffer);
}

void UpdateShadowUniformBuffers(size_t currentFrame, const SceneMatricesUniform* sceneUniforms)
{
	//per Instance data should be updated by the geometry render pass
	//We only update the scene related data
	UpdatePerFrameBuffer(&shadowSceneUniformBuffer, sceneUniforms, sizeof(SceneMatricesUniform), currentFrame);
}

void CmdBeginShadowPass(VkCommandBuffer commandBuffer, size_t currentFrame)
{
	CmdBeginVkLabel(commandBuffer, "Shadow Renderpass", glm::vec4(0.5f, 0.2f, 0.4f, 1.0f));
	BeginRenderPass(commandBuffer, shadowRenderPass, shadowFrameBuffer.frameBuffer, shadowFrameBuffer.extent);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipelineLayout, RENDERPASS_SET, 1, &shadowDescriptorSets[currentFrame], 0, nullptr);
}

void CmdDrawShadowPass(VkCommandBuffer commandBuffer, const SceneInstanceSet* instanceSet, const ModelAsset* modelAsset, uint32_t currentFrame)
{
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipelineLayout, INSTANCE_SET, 1,
		&shadowInstanceDescriptorSet[currentFrame], 1, &instanceSet->geometryBufferOffsets[currentFrame]);
	CmdDrawIndexed(commandBuffer, *modelAsset);
}

void CmdEndShadowPass(VkCommandBuffer commandBuffer)
{
	EndRenderPass(commandBuffer);
	CmdEndVkLabel(commandBuffer);
}

static void CreateShadowGraphicsPipeline(const VkVertexInputBindingDescription * vibDescription, const VkVertexInputAttributeDescription* visDescriptions, uint32_t visDescriptionsCount,
	std::vector<char>& vertShaderCode, std::vector<char>& fragShaderCode, VkExtent2D framebufferExtent, VkRenderPass renderPass,
	VkPipelineLayout* o_pipelineLayout, VkPipeline* o_pipeline)
{
	//Pipeline layout
	//Describe complete set of resources available (image, sampler, ubo, constants, ...)
	VkDescriptorSetLayout descriptorSetLayouts [] = { shadowDescriptorSetLayout, shadowInstanceDescriptorSetLayout };
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 2; // Optional
	pipeline_layout_info.pSetLayouts = descriptorSetLayouts; // Optional
	pipeline_layout_info.pushConstantRangeCount = 0; // Optional
	pipeline_layout_info.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(g_vk.device, &pipeline_layout_info, nullptr, o_pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = vibDescription; // Optional
	vertex_input_info.vertexAttributeDescriptionCount = visDescriptionsCount;
	vertex_input_info.pVertexAttributeDescriptions = visDescriptions; // Optional

																				   //Input assembly
	VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	VkShaderModule vert_shader_module = create_shader_module(vertShaderCode.data(), vertShaderCode.size());

	VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
	vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_info.module = vert_shader_module;
	vert_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info };

	//Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_TRUE;
	rasterizer.depthBiasConstantFactor = 1.25f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 1.75f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional

	//Color blending
	VkPipelineColorBlendAttachmentState color_blend_attachement = {};
	color_blend_attachement.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachement.blendEnable = VK_FALSE;
	color_blend_attachement.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	color_blend_attachement.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	color_blend_attachement.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	color_blend_attachement.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	color_blend_attachement.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	color_blend_attachement.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo color_blending_info = {};
	color_blending_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending_info.logicOpEnable = VK_FALSE;
	color_blending_info.logicOp = VK_LOGIC_OP_COPY; // Optional
	color_blending_info.attachmentCount = 1;
	color_blending_info.pAttachments = &color_blend_attachement;
	color_blending_info.blendConstants[0] = 0.0f; // Optional
	color_blending_info.blendConstants[1] = 0.0f; // Optional
	color_blending_info.blendConstants[2] = 0.0f; // Optional
	color_blending_info.blendConstants[3] = 0.0f; // Optional

												  //Viewport and scissors
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(framebufferExtent.width);
	viewport.height = static_cast<float>(framebufferExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = framebufferExtent;

	VkPipelineViewportStateCreateInfo viewport_state_info = {};
	viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_info.viewportCount = 1;
	viewport_state_info.pViewports = &viewport;
	viewport_state_info.scissorCount = 1;
	viewport_state_info.pScissors = &scissor;

	//Pipeline
	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = 1;
	pipeline_info.pStages = shader_stages;

	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly;
	pipeline_info.pViewportState = &viewport_state_info;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pDepthStencilState = &depthStencil; //optional
	pipeline_info.pColorBlendState = &color_blending_info;
	pipeline_info.pDynamicState = nullptr; //optional

	pipeline_info.layout = *o_pipelineLayout;

	//Compatible with other render passes that are compatible together
	/*Two render passes are compatible if their corresponding color, input, resolve, and depth / stencil attachment references
	are compatible and if they are otherwise identical except for:
	�EInitial and final image layout in attachment descriptions
	�ELoad and store operations in attachment descriptions
	�EImage layout in attachment references*/
	pipeline_info.renderPass = renderPass;
	pipeline_info.subpass = 0;

	pipeline_info.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipeline_info.basePipelineIndex = -1; // Optional

	if (vkCreateGraphicsPipelines(g_vk.device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, o_pipeline) != VK_SUCCESS)
		throw std::runtime_error("failed to create graphics pipeline!");

	vkDestroyShaderModule(g_vk.device, vert_shader_module, nullptr);
}

void CreateShadowGraphicPipeline()
{
	auto bindingDescription = Vertex::get_binding_description();
	auto attributeDescriptions = Vertex::get_attribute_descriptions();

	std::vector<char> vertShaderCode = readFile("shaders/shadows.vert.spv");
	std::vector<char> fragShaderCode = readFile("shaders/shadows.frag.spv");

	CreateShadowGraphicsPipeline(&bindingDescription, attributeDescriptions.data(), static_cast<uint32_t>(attributeDescriptions.size()), vertShaderCode, fragShaderCode,
		shadowFrameBufferExtent, shadowRenderPass.vk_renderpass, &shadowPipelineLayout, &shadowPipeline);
}

static void CreateShadowRenderPass( const VkFormat depthFormat, RenderPass * o_renderPass)
{
	//Attachements
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 0;
	subpass.pColorAttachments = nullptr;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 0; //Take the last spot
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = 1;
	render_pass_info.pAttachments = &depthAttachment;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	render_pass_info.dependencyCount = 1;
	render_pass_info.pDependencies = &dependency;

	/*A render pass represents a collection of attachments, subpasses, and dependencies between the subpasses,
	and describes how the attachments are used over the course of the subpasses.*/
	if (vkCreateRenderPass(g_vk.device, &render_pass_info, nullptr, &o_renderPass->vk_renderpass) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass!");

	o_renderPass->colorFormats = std::vector<VkFormat>();
	o_renderPass->depthFormat = depthFormat;
}

void CreateShadowRenderPass()
{
	CreateShadowRenderPass(shadowDepthFormat, &shadowRenderPass);
	MarkVkObject((uint64_t)shadowRenderPass.vk_renderpass, VK_OBJECT_TYPE_RENDER_PASS, "Shadow Renderpass");
}

void CreateShadowDescriptorSet(VkDescriptorPool descriptorPool, const VkBuffer*instanceUniformBuffer)
{
	std::vector<DescriptorSet> descriptorSets;
	descriptorSets.resize(SIMULTANEOUS_FRAMES);

	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
	{
		DescriptorSet& geoDescriptorSet = descriptorSets[i];
		geoDescriptorSet = {};
		geoDescriptorSet.bufferDescriptors.push_back({ {shadowSceneUniformBuffer.buffers[i], 0, VK_WHOLE_SIZE}, 0 });
		geoDescriptorSet.layout = shadowDescriptorSetLayout;
	}
	createDescriptorSets(descriptorPool, descriptorSets.size(), descriptorSets.data());
	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		shadowDescriptorSets[i] = descriptorSets[i].set;

	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
	{
		DescriptorSet& geoDescriptorSet = descriptorSets[i];
		geoDescriptorSet = {};
		geoDescriptorSet.dynamicBufferDescriptors.push_back({ {instanceUniformBuffer[i], 0, VK_WHOLE_SIZE}, 0 });
		geoDescriptorSet.layout = shadowInstanceDescriptorSetLayout;
	}
	createDescriptorSets(descriptorPool, descriptorSets.size(), descriptorSets.data());
	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		shadowInstanceDescriptorSet[i] = descriptorSets[i].set;
}

void CreateShadowDescriptorSetLayout()
{
	const VkDescriptorSetLayoutBinding sceneLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr };

	const std::array<VkDescriptorSetLayoutBinding, 1> bindings = { sceneLayoutBinding };
	CreateDesciptorSetLayout(bindings.data(), static_cast<uint32_t>(bindings.size()), &shadowDescriptorSetLayout);

	const VkDescriptorSetLayoutBinding instanceLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr };
	const std::array<VkDescriptorSetLayoutBinding, 1> instanceBindings = { instanceLayoutBinding };
	CreateDesciptorSetLayout(instanceBindings.data(), static_cast<uint32_t>(instanceBindings.size()), &shadowInstanceDescriptorSetLayout);
}

void CreateShadowPass()
{
	CreateShadowDescriptorSetLayout();
	CreateShadowRenderPass();
	CreateShadowGraphicPipeline();
	CreateShadowFrameBuffer();

	CreatePerFrameBuffer(sizeof(SceneMatricesUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&shadowSceneUniformBuffer);
}

void CleanupShadowPass()
{
	vkDestroyPipeline(g_vk.device, shadowPipeline, nullptr);
	vkDestroyPipelineLayout(g_vk.device, shadowPipelineLayout, nullptr);
	vkDestroyRenderPass(g_vk.device, shadowRenderPass.vk_renderpass, nullptr);
	vkDestroyDescriptorSetLayout(g_vk.device, shadowDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(g_vk.device, shadowInstanceDescriptorSetLayout, nullptr);

	DestroyPerFrameBuffer(&shadowSceneUniformBuffer);
	vkDestroyFramebuffer(g_vk.device, shadowFrameBuffer.frameBuffer, nullptr);		
	DestroyImage(shadowDepthImage);
}