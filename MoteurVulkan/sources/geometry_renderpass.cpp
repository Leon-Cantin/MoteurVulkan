#include "geometry_renderpass.h"
#include "..\shaders\shadersCommon.h"

#include "vk_framework.h"
#include "vk_shader.h"
#include "descriptors.h"
#include "vk_commands.h"
#include "vk_debug.h"

#include <vector>

VkDescriptorSetLayout geoDescriptorSetLayout;
VkDescriptorSetLayout geoInstanceDescriptorSetLayout;
VkPipelineLayout geoPipelineLayout;
const RenderPass* geometryRenderPass;
VkPipeline geoGraphicsPipeline;

std::array<VkDescriptorSet, SIMULTANEOUS_FRAMES> geoDescriptorSets;
std::array<VkDescriptorSet, SIMULTANEOUS_FRAMES> geoInstanceDescriptorSet;

static void createGeoGraphicsPipeline(const VkVertexInputBindingDescription * vibDescription, const VkVertexInputAttributeDescription* visDescriptions, uint32_t visDescriptionsCount, std::vector<char>& vertShaderCode, std::vector<char>& fragShaderCode, VkExtent2D framebufferExtent, VkRenderPass renderPass,
	VkPipelineLayout pipelineLayout, VkPipeline* o_pipeline)
{

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
	VkShaderModule frag_shader_module = create_shader_module(fragShaderCode.data(), fragShaderCode.size());

	VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
	vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_info.module = vert_shader_module;
	vert_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
	frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_info.module = frag_shader_module;
	frag_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_stage_info };

	//Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

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
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
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
	pipeline_info.stageCount = 2;
	pipeline_info.pStages = shader_stages;

	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly;
	pipeline_info.pViewportState = &viewport_state_info;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pDepthStencilState = &depthStencil; //optional
	pipeline_info.pColorBlendState = &color_blending_info;
	pipeline_info.pDynamicState = nullptr; //optional

	pipeline_info.layout = pipelineLayout;

	//Compatible with other render passes that are compatible together
	/*Two render passes are compatible if their corresponding color, input, resolve, and depth / stencil attachment references
	are compatible and if they are otherwise identical except for:
	ÅEInitial and final image layout in attachment descriptions
	ÅELoad and store operations in attachment descriptions
	ÅEImage layout in attachment references*/
	pipeline_info.renderPass = renderPass;
	pipeline_info.subpass = 0;

	pipeline_info.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipeline_info.basePipelineIndex = -1; // Optional

	if (vkCreateGraphicsPipelines(g_vk.device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, o_pipeline) != VK_SUCCESS)
		throw std::runtime_error("failed to create graphics pipeline!");

	vkDestroyShaderModule(g_vk.device, vert_shader_module, nullptr);
	vkDestroyShaderModule(g_vk.device, frag_shader_module, nullptr);
}

void createGeoGraphicPipeline( VkExtent2D extent )
{
	auto bindingDescription = Vertex::get_binding_description();
	auto attributeDescriptions = Vertex::get_attribute_descriptions();

	std::vector<char> vertShaderCode = readFile("shaders/triangle.vert.spv");
	std::vector<char> fragShaderCode = readFile("shaders/triangle.frag.spv");

	//Pipeline layout
	std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = { geoDescriptorSetLayout, geoInstanceDescriptorSetLayout };
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 2;
	pipeline_layout_info.pSetLayouts = descriptorSetLayouts.data();
	pipeline_layout_info.pushConstantRangeCount = 0;
	pipeline_layout_info.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(g_vk.device, &pipeline_layout_info, nullptr, &geoPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	createGeoGraphicsPipeline(&bindingDescription, attributeDescriptions.data(), static_cast<uint32_t>(attributeDescriptions.size()), vertShaderCode, fragShaderCode,
		extent, geometryRenderPass->vk_renderpass, geoPipelineLayout, &geoGraphicsPipeline);
}

void AddGeometryRenderPass(const RenderPass* renderpass)
{
	geometryRenderPass = renderpass;
}

void CreateGeometryDescriptorSet(VkDescriptorPool descriptorPool, VkBuffer* sceneUniformBuffers, VkBuffer* instanceUniformBuffers, VkBuffer* lightBuffers, VkImageView textureView,
	VkImageView normalTextureView, VkSampler sampler, VkImageView shadowTextureView, VkSampler shadowSampler)
{
	std::array<DescriptorSet, SIMULTANEOUS_FRAMES> descriptorSets;

	//Per render pass descriptor est
	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
	{
		DescriptorSet& geoDescriptorSet = descriptorSets[i] = {};
		geoDescriptorSet.descriptors.resize(5);
		geoDescriptorSet.descriptors[0] = { {sceneUniformBuffers[i], 0, VK_WHOLE_SIZE}, {}, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0 };
		geoDescriptorSet.descriptors[1] = { {lightBuffers[i], 0, VK_WHOLE_SIZE}, {}, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 };
		geoDescriptorSet.descriptors[2] = { {},  { sampler, textureView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 };
		geoDescriptorSet.descriptors[3] = { {},  { sampler, normalTextureView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 };
		geoDescriptorSet.descriptors[4] = { {},  { shadowSampler, shadowTextureView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 };
		geoDescriptorSet.layout = geoDescriptorSetLayout;
	}
	createDescriptorSets(descriptorPool, descriptorSets.size(), descriptorSets.data());

	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		geoDescriptorSets[i] = descriptorSets[i].set;

	//Per instance descriptor set
	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
	{
		DescriptorSet& geoDescriptorSet = descriptorSets[i] = {};
		geoDescriptorSet.descriptors.resize(1);
		geoDescriptorSet.descriptors[0] = { {instanceUniformBuffers[i], 0, VK_WHOLE_SIZE}, {}, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 0 };
		geoDescriptorSet.layout = geoInstanceDescriptorSetLayout;
	}
	createDescriptorSets(descriptorPool, descriptorSets.size(), descriptorSets.data());

	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		geoInstanceDescriptorSet[i] = descriptorSets[i].set;
}

void createGeoDescriptorSetLayout()
{
	const VkDescriptorSetLayoutBinding uboLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	const VkDescriptorSetLayoutBinding lightLayoutBinding = { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	const VkDescriptorSetLayoutBinding samplerLayoutBinding = { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };	
	const VkDescriptorSetLayoutBinding normalSamplerLayoutBinding = { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	const VkDescriptorSetLayoutBinding shadowSamplerLayoutBinding = { 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };

	const std::array<VkDescriptorSetLayoutBinding, 5> bindings = { uboLayoutBinding, samplerLayoutBinding, lightLayoutBinding, normalSamplerLayoutBinding, shadowSamplerLayoutBinding };
	CreateDesciptorSetLayout(bindings.data(), static_cast<uint32_t>(bindings.size()), &geoDescriptorSetLayout);

	const VkDescriptorSetLayoutBinding instanceUboLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC , 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	const std::array<VkDescriptorSetLayoutBinding, 1> bindings2 = { instanceUboLayoutBinding };
	CreateDesciptorSetLayout(bindings2.data(), static_cast<uint32_t>(bindings2.size()), &geoInstanceDescriptorSetLayout);
}

void CmdBeginGeometryRenderPass(VkCommandBuffer commandBuffer, VkExtent2D extent, uint32_t currentFrame)
{
	CmdBeginVkLabel(commandBuffer, "Geometry renderpass", glm::vec4(0.8f, 0.6f, 0.4f, 1.0f));
	BeginRenderPass(commandBuffer, *geometryRenderPass, geometryRenderPass->outputFrameBuffer[currentFrame].frameBuffer, extent);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, geoGraphicsPipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, geoPipelineLayout, RENDERPASS_SET, 1, &geoDescriptorSets[currentFrame], 0, nullptr);
}

void CmdEndGeometryRenderPass(VkCommandBuffer vkCommandBuffer)
{
	EndRenderPass(vkCommandBuffer);
	CmdEndVkLabel(vkCommandBuffer);
}

void CmdDrawModelAsset( VkCommandBuffer commandBuffer, const SceneInstanceSet* instanceSet, const ModelAsset& modelAsset, uint32_t currentFrame)
{
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, geoPipelineLayout, INSTANCE_SET, 1,
		&geoInstanceDescriptorSet[currentFrame], 1, &instanceSet->geometryBufferOffsets[currentFrame]);
	CmdDrawIndexed(commandBuffer, modelAsset);	
}

void ReloadGeometryShaders( VkExtent2D extent )
{
	vkDestroyPipeline(g_vk.device, geoGraphicsPipeline, nullptr);
	createGeoGraphicPipeline(extent);
}

void CleanupGeometryRenderpassAfterSwapchain()
{
	vkDestroyPipeline(g_vk.device, geoGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(g_vk.device, geoPipelineLayout, nullptr);
	//TODO: format could change after a swap chain recreation. The renderpas depends on it.
}

void CleanupGeometryRenderpass()
{
	vkDestroyDescriptorSetLayout(g_vk.device, geoDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(g_vk.device, geoInstanceDescriptorSetLayout, nullptr);
}
