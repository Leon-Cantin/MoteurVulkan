#include "renderpass.h"

#include "vk_shader.h"

void CreatePipeline(
	const VICreation& viCreation,
	const std::vector<ShaderCreation>& shaders,
	VkExtent2D framebufferExtent,
	VkRenderPass renderPass,
	VkPipelineLayout pipelineLayout,
	RasterizationState rasterizationState,
	DepthStencilState depthStencilState,
	bool blendEnabled,
	VkPrimitiveTopology primitiveTopology,
	VkPipeline* o_pipeline)
{

	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = viCreation.vibDescriptionsCount;
	vertex_input_info.pVertexBindingDescriptions = viCreation.vibDescription; // Optional
	vertex_input_info.vertexAttributeDescriptionCount = viCreation.visDescriptionsCount;
	vertex_input_info.pVertexAttributeDescriptions = viCreation.visDescriptions; // Optional

																				   //Input assembly
	VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = primitiveTopology;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineShaderStageCreateInfo shader_stages[8];
	uint32_t shadersCount = 0;

	for( uint8_t i = 0; i < shaders.size(); ++i )
	{
		VkPipelineShaderStageCreateInfo& shaderStageInfo = shader_stages[shadersCount++];
		shaderStageInfo = {};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = shaders[i].flags;
		shaderStageInfo.module = create_shader_module( shaders[i].code, shaders[i].length );
		shaderStageInfo.pName = shaders[i].entryPoint;
	}

	//Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = rasterizationState.backFaceCulling ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = rasterizationState.depthBiased;
	rasterizer.depthBiasConstantFactor = rasterizationState.depthBiased ? 1.25f : 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = rasterizationState.depthBiased ? 1.75f : 0.0f; // Optional

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
	depthStencil.depthTestEnable = depthStencilState.depthRead;
	depthStencil.depthWriteEnable = depthStencilState.depthWrite;
	depthStencil.depthCompareOp = depthStencilState.depthCompareOp,
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional

	//Color blending
	VkPipelineColorBlendAttachmentState color_blend_attachement = {};
	color_blend_attachement.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	if (blendEnabled)
	{
		color_blend_attachement.blendEnable = VK_TRUE;
		color_blend_attachement.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachement.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		color_blend_attachement.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachement.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachement.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
		color_blend_attachement.alphaBlendOp = VK_BLEND_OP_ADD;
	}
	else
	{
		color_blend_attachement.blendEnable = VK_FALSE;
		color_blend_attachement.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		color_blend_attachement.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		color_blend_attachement.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		color_blend_attachement.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		color_blend_attachement.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		color_blend_attachement.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
	}

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
	pipeline_info.stageCount = shadersCount;
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


	for( uint8_t i = 0; i < shadersCount; ++i )
		vkDestroyShaderModule( g_vk.device, shader_stages[i].module, nullptr);
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