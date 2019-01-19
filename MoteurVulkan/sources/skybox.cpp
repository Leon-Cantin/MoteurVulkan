#include "skybox.h"

#include "vk_shader.h"
#include "vk_framework.h"
#include "vk_buffer.h"
#include "descriptors.h"
#include "vk_debug.h"

#include <glm/gtc/matrix_transform.hpp>
#include <array>

VkDescriptorSetLayout skyboxDescriptorSetLayout;
VkPipelineLayout skyboxPipelineLayout;
RenderPass skyboxRenderPass;
VkPipeline skyboxGraphicsPipeline;
std::array<VkDescriptorSet, SIMULTANEOUS_FRAMES> skyboxDescriptorSets;
PerFrameBuffer skyboxUniformBuffer;

//TODO I want to use a mat3 but the mem requirement size is at 48 instead of 36, it ends up broken
// when received by the shader
// https://www.khronos.org/registry/vulkan/specs/1.0-extensions/html/vkspec.html#interfaces-resources  14.5.4
struct SkyboxUniformBufferObject {
	glm::mat4 inv_view_matrix;
};

void createSkyboxDescriptorSetLayout()
{
	const VkDescriptorSetLayoutBinding uboLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr };
	const VkDescriptorSetLayoutBinding samplerLayoutBinding = { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };

	const std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
	CreateDesciptorSetLayout(bindings.data(), static_cast<uint32_t>(bindings.size()), &skyboxDescriptorSetLayout);
}

void CreateSkyboxDescriptorSet(VkDescriptorPool descriptorPool, VkImageView skyboxImageView, VkSampler trilinearSampler)
{
	std::vector<DescriptorSet> descriptorSets;
	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
	{
		BufferDescriptor skyboxCommonMatricesBufferDesc = { {skyboxUniformBuffer.buffers[i], 0, VK_WHOLE_SIZE}, 0 };
		ImageDescriptor skyboxTextureDesc = { {trilinearSampler, skyboxImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}, 1 };
		DescriptorSet skyboxDescriptorSet;
		skyboxDescriptorSet.bufferDescriptors.push_back(skyboxCommonMatricesBufferDesc);
		skyboxDescriptorSet.imageSamplerDescriptors.push_back(skyboxTextureDesc);
		skyboxDescriptorSet.layout = skyboxDescriptorSetLayout;
		descriptorSets.push_back(skyboxDescriptorSet);

	}
	createDescriptorSets(descriptorPool, descriptorSets.size(), descriptorSets.data());
	for(uint32_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		skyboxDescriptorSets[i] = descriptorSets[i].set;
}

static void createSkyboxGraphicsPipeline(std::vector<char>& vertShaderCode, std::vector<char>& fragShaderCode, VkExtent2D framebufferExtent, VkRenderPass renderPass,
	VkDescriptorSetLayout descriptorSetLayout, VkPipelineLayout* o_pipelineLayout, VkPipeline* o_pipeline)
{
	//Vertex Input
	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 0;
	vertex_input_info.pVertexBindingDescriptions = VK_NULL_HANDLE; // Optional
	vertex_input_info.vertexAttributeDescriptionCount = 0;
	vertex_input_info.pVertexAttributeDescriptions = VK_NULL_HANDLE; // Optional

	//Input assembly
	VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
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
	rasterizer.depthClampEnable = VK_TRUE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
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
	depthStencil.depthWriteEnable = VK_FALSE;
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

	//Pipeline layout
	//Describe complete set of resources available (image, sampler, ubo, constants, ...)
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1; // Optional
	pipeline_layout_info.pSetLayouts = &descriptorSetLayout; // Optional
	pipeline_layout_info.pushConstantRangeCount = 0; // Optional
	pipeline_layout_info.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(g_vk.device, &pipeline_layout_info, nullptr, o_pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

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

	pipeline_info.layout = *o_pipelineLayout;

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

void create_skybox_graphics_pipeline(VkExtent2D extent)
{
	std::vector<char> vertShaderCode = readFile("shaders/skybox.vert.spv");
	std::vector<char> fragShaderCode = readFile("shaders/skybox.frag.spv");
	createSkyboxGraphicsPipeline(vertShaderCode, fragShaderCode, extent, skyboxRenderPass.vk_renderpass,
		skyboxDescriptorSetLayout, &skyboxPipelineLayout, &skyboxGraphicsPipeline);
}


void create_skybox_render_pass(VkFormat colorFormat)
{
	std::vector<VkFormat> colorImages = { colorFormat };
	CreateLastRenderPass(colorImages, VK_FORMAT_D32_SFLOAT, &skyboxRenderPass);
	MarkVkObject((uint64_t)skyboxRenderPass.vk_renderpass, VK_OBJECT_TYPE_RENDER_PASS, "Skybox Renderpass");
}

void createSkyboxUniformBuffers()
{
	//TODO: do something so I can use mat3 instead of mat4
	CreatePerFrameBuffer(sizeof(SkyboxUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&skyboxUniformBuffer);
}

void UpdateSkyboxUniformBuffers(size_t currentFrame, const glm::mat4& world_view_matrix)
{
	SkyboxUniformBufferObject subo = {};
	subo.inv_view_matrix = glm::mat3(glm::scale(transpose(world_view_matrix), glm::vec3(1.0f, -1.0f, 1.0f)));

	UpdatePerFrameBuffer( &skyboxUniformBuffer, &subo, sizeof(subo), currentFrame);
}

void CmdDrawSkybox(VkCommandBuffer commandBuffer, VkFramebuffer frameBuffer, VkExtent2D extent, size_t currentFrame)
{
	CmdBeginVkLabel(commandBuffer, "Skybox Renderpass", glm::vec4(0.2f, 0.2f, 0.9f, 1.0f));
	BeginRenderPass(commandBuffer, skyboxRenderPass, frameBuffer, extent);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxGraphicsPipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipelineLayout, 0, 1, &skyboxDescriptorSets[currentFrame], 0, nullptr);

	vkCmdDraw(commandBuffer, 4, 1, 0, 0);
	EndRenderPass(commandBuffer);
	CmdEndVkLabel(commandBuffer);
}

void CleanupSkyboxAfterSwapchain()
{
	vkDestroyPipeline(g_vk.device, skyboxGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(g_vk.device, skyboxPipelineLayout, nullptr);
	vkDestroyRenderPass(g_vk.device, skyboxRenderPass.vk_renderpass, nullptr);
}

void CleanupSkybox()
{
	vkDestroyDescriptorSetLayout(g_vk.device, skyboxDescriptorSetLayout, nullptr);
	DestroyPerFrameBuffer(&skyboxUniformBuffer);
}

void ReloadSkyboxShaders(VkExtent2D extent)
{
	vkDestroyPipeline(g_vk.device, skyboxGraphicsPipeline, nullptr);
	create_skybox_graphics_pipeline(extent);
}

