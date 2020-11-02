#include "vk_globals.h"
#include "vk_vertex_input.h"
#include "vk_shader.h"
#include <assert.h>

uint32_t GetBindingDescription( const std::vector<VIBinding>& VIBindings, VIState* o_viCreation )
{
	uint32_t count = VIBindings.size();
	assert( count <= VI_STATE_MAX_DESCRIPTIONS );

	GetAPIVIBindingDescription( VIBindings.data(), count, o_viCreation->vibDescription, o_viCreation->visDescriptions );

	o_viCreation->vibDescriptionsCount = count;
	o_viCreation->visDescriptionsCount = count;

	return count;
}

void CreateGfxPipelineLayout( const GfxDescriptorTableDesc* descriptorTablesDescs, const GfxDescriptorTableLayout* descriptorTableLayouts, uint32_t descriptorTablesDescsCount, const GfxRootConstantRange* rootConstantRanges, uint32_t rootConstantRangesCount, GfxPipelineLayout* o_pipelineLayout )
{
	//Order the table layouts in the order they are bound in the pipeline
	VkDescriptorSetLayout orderedDescriptorTablesLayouts[8];
	assert( descriptorTablesDescsCount < 8 );
	for( uint32_t i = 0; i < descriptorTablesDescsCount; ++i )
	{
		const GfxDescriptorTableDesc& setDesc = descriptorTablesDescs[i];
		assert( i == setDesc.binding ); //We can't leave any hole in the pilpeline layout
		orderedDescriptorTablesLayouts[i] = descriptorTableLayouts[setDesc.binding];
	}

	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = descriptorTablesDescsCount;
	pipeline_layout_info.pSetLayouts = orderedDescriptorTablesLayouts;

	VkPushConstantRange pushConstantRanges[8];
	assert( rootConstantRangesCount < 8 );
	for( uint32_t i = 0; i < rootConstantRangesCount; ++i )
	{
		pushConstantRanges[i].stageFlags = rootConstantRanges[i].stageFlags;
		pushConstantRanges[i].offset = sizeof( root_constant_t ) * rootConstantRanges[i].offset;
		pushConstantRanges[i].size = sizeof( root_constant_t ) * rootConstantRanges[i].count;
	}
	pipeline_layout_info.pushConstantRangeCount = rootConstantRangesCount;
	pipeline_layout_info.pPushConstantRanges = pushConstantRanges;

	if( vkCreatePipelineLayout( g_gfx.device.device, &pipeline_layout_info, nullptr, o_pipelineLayout ) != VK_SUCCESS ) {
		throw std::runtime_error( "failed to create pipeline layout!" );
	}
}

void CreatePipeline( const GpuPipelineStateDesc& gpuPipelineDesc, const RenderPass& renderPass, GfxPipelineLayout pipelineLayout, GfxPipeline* o_pipeline )
{
	const VkExtent2D& viewportSize = renderPass.outputFrameBuffer[0].extent;//TODO: temp hack to get a relative size;

	//Vertex Input
	const VIState& viState = gpuPipelineDesc.viState;
	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = viState.vibDescriptionsCount;
	vertex_input_info.pVertexBindingDescriptions = viState.vibDescription; // Optional
	vertex_input_info.vertexAttributeDescriptionCount = viState.visDescriptionsCount;
	vertex_input_info.pVertexAttributeDescriptions = viState.visDescriptions; // Optional

	//Input assembly
	VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = ToVkPrimitiveTopology( gpuPipelineDesc.primitiveTopology );
	input_assembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineShaderStageCreateInfo shader_stages[8];
	uint32_t shadersCount = 0;

	for( uint8_t i = 0; i < gpuPipelineDesc.shaders.size(); ++i )
	{
		VkPipelineShaderStageCreateInfo& shaderStageInfo = shader_stages[shadersCount++];
		shaderStageInfo = {};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = ToVkShaderStageFlagBits( gpuPipelineDesc.shaders[i].flags );
		//TODO: these cast are dangerous for alligment
		shaderStageInfo.module = create_shader_module( reinterpret_cast< const uint32_t * >(gpuPipelineDesc.shaders[i].code.data()), gpuPipelineDesc.shaders[i].code.size() );
		shaderStageInfo.pName = gpuPipelineDesc.shaders[i].entryPoint;
	}

	//Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = gpuPipelineDesc.rasterizationState.backFaceCulling ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = gpuPipelineDesc.rasterizationState.depthBiased;
	rasterizer.depthBiasConstantFactor = gpuPipelineDesc.rasterizationState.depthBiased ? 1.25f : 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = gpuPipelineDesc.rasterizationState.depthBiased ? 1.75f : 0.0f; // Optional

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
	depthStencil.depthTestEnable = gpuPipelineDesc.depthStencilState.depthRead;
	depthStencil.depthWriteEnable = gpuPipelineDesc.depthStencilState.depthWrite;
	depthStencil.depthCompareOp = ToVkCompareOp( gpuPipelineDesc.depthStencilState.depthCompareOp );
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional

	//Color blending
	VkPipelineColorBlendAttachmentState color_blend_attachement = {};
	color_blend_attachement.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	if( gpuPipelineDesc.blendEnabled )
	{
		color_blend_attachement.blendEnable = VK_TRUE;
		color_blend_attachement.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachement.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		color_blend_attachement.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachement.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		color_blend_attachement.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
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
	viewport.width = static_cast< float >(viewportSize.width);
	viewport.height = static_cast< float >(viewportSize.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = viewportSize;

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
	pipeline_info.renderPass = renderPass.vk_renderpass;
	pipeline_info.subpass = 0;

	pipeline_info.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipeline_info.basePipelineIndex = -1; // Optional

	if( vkCreateGraphicsPipelines( g_gfx.device.device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, o_pipeline ) != VK_SUCCESS )
		throw std::runtime_error( "failed to create graphics pipeline!" );


	for( uint8_t i = 0; i < shadersCount; ++i )
		vkDestroyShaderModule( g_gfx.device.device, shader_stages[i].module, nullptr );
}

void Destroy( GfxPipeline* pipeline )
{
	vkDestroyPipeline( g_gfx.device.device, *pipeline, nullptr );
	*pipeline = VK_NULL_HANDLE;
}

void Destroy( GfxPipelineLayout* pipelineLayout )
{
	vkDestroyPipelineLayout( g_gfx.device.device, *pipelineLayout, nullptr );
	*pipelineLayout = VK_NULL_HANDLE;
}