#include "text_overlay.h"

#include "model_asset.h"
#include "vk_shader.h"
#include "vk_framework.h"
#include "stb_font_consolas_24_latin1.inl"
#include "vk_buffer.h"
#include "descriptors.h"
#include "swapchain.h"
#include "vk_debug.h"
#include "console_command.h"

VkDescriptorSetLayout textDescriptorSetLayout = VK_NULL_HANDLE;
VkPipelineLayout textPipelineLayout = VK_NULL_HANDLE;
const RenderPass* textRenderPass;
VkPipeline textGraphicsPipeline = VK_NULL_HANDLE;
VkDescriptorSet textDescriptorSet;
VkBuffer textVertexBuffer = VK_NULL_HANDLE;
VkDeviceMemory textVertexBufferMemory = VK_NULL_HANDLE;
VkBuffer textIndexBuffer = VK_NULL_HANDLE;
VkDeviceMemory textIndexBufferMemory = VK_NULL_HANDLE;
uint32_t maxTextCharCount = 0;
uint32_t currentTextCharCount = 0;

stb_fontchar stbFontData[STB_FONT_consolas_24_latin1_NUM_CHARS];
GfxImage g_fontImage;

typedef uint32_t Index_t;

const uint32_t verticesPerChar = 4;
const uint32_t indexesPerChar = 6;

void CreateTextDescriptorSetLayout()
{
	const VkDescriptorSetLayoutBinding samplerLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };

	const std::array<VkDescriptorSetLayoutBinding, 1> bindings = { samplerLayoutBinding };
	CreateDesciptorSetLayout(bindings.data(), static_cast<uint32_t>(bindings.size()), &textDescriptorSetLayout);
}

void CreateTextDescriptorSet(VkDescriptorPool descriptorPool, VkSampler trilinearSampler)
{
	DescriptorSet descriptorSet = {};
	descriptorSet.descriptors.resize(1);
	descriptorSet.descriptors[0] = { {}, { trilinearSampler, g_fontImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0 };
	descriptorSet.layout = textDescriptorSetLayout;
	createDescriptorSets(descriptorPool, 1, &descriptorSet);

	textDescriptorSet = descriptorSet.set;
}

void CreateTextGraphicsPipeline( VkExtent2D extent)
{
	auto bindingDescription = TextVertex::get_binding_description();
	auto attributeDescriptions = TextVertex::get_attribute_descriptions();

	std::vector<char> vertShaderCode = readFile("shaders/text.vert.spv");
	std::vector<char> fragShaderCode = readFile("shaders/text.frag.spv");

	createTextGraphicsPipeline(&bindingDescription, attributeDescriptions.data(), static_cast<uint32_t>(attributeDescriptions.size()), vertShaderCode, fragShaderCode,
		extent, textRenderPass->vk_renderpass, textDescriptorSetLayout, &textPipelineLayout, &textGraphicsPipeline);
}

void createTextGraphicsPipeline(const VkVertexInputBindingDescription * vibDescription, const VkVertexInputAttributeDescription* visDescriptions, uint32_t visDescriptionsCount, std::vector<char>& vertShaderCode, std::vector<char>& fragShaderCode, VkExtent2D framebufferExtent, VkRenderPass renderPass,
	VkDescriptorSetLayout descriptorSetLayout, VkPipelineLayout* o_pipelineLayout, VkPipeline* o_pipeline)
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
	depthStencil.depthTestEnable = VK_FALSE;
	depthStencil.depthWriteEnable = VK_FALSE;
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
	color_blend_attachement.blendEnable = VK_TRUE;
	color_blend_attachement.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachement.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachement.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachement.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachement.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
	color_blend_attachement.alphaBlendOp = VK_BLEND_OP_ADD;

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
	• Initial and final image layout in attachment descriptions
	• Load and store operations in attachment descriptions
	• Image layout in attachment references*/
	pipeline_info.renderPass = renderPass;
	pipeline_info.subpass = 0;

	pipeline_info.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipeline_info.basePipelineIndex = -1; // Optional

	if (vkCreateGraphicsPipelines(g_vk.device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, o_pipeline) != VK_SUCCESS)
		throw std::runtime_error("failed to create graphics pipeline!");

	vkDestroyShaderModule(g_vk.device, vert_shader_module, nullptr);
	vkDestroyShaderModule(g_vk.device, frag_shader_module, nullptr);
}

void CmdDrawText( VkCommandBuffer commandBuffer, VkExtent2D extent, size_t frameIndex)
{
	CmdBeginVkLabel(commandBuffer, "Text overlay Renderpass", glm::vec4(0.6f, 0.6f, 0.6f, 1.0f));
	BeginRenderPass(commandBuffer, *textRenderPass, textRenderPass->outputFrameBuffer[frameIndex].frameBuffer, extent);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, textGraphicsPipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, textPipelineLayout, 0, 1, &textDescriptorSet, 0, nullptr);

	VkBuffer vertexBuffers[] = { textVertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, textIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(commandBuffer, currentTextCharCount * indexesPerChar, 1, 0, 0, 0);

	EndRenderPass(commandBuffer);
	CmdEndVkLabel(commandBuffer);
}

void UpdateText( const TextZone * textZones, size_t textZonesCount, VkExtent2D surfaceExtent)
{
	const float charW = 1.5f / surfaceExtent.width;
	const float charH = 1.5f / surfaceExtent.height;

	std::vector<TextVertex> text_vertices;
	std::vector<Index_t>	text_indices;
	size_t totalCharCount = 0;
	for (size_t i = 0; i < textZonesCount; ++i)
		totalCharCount += textZones[i].text.size();

	assert(totalCharCount < maxTextCharCount);
	text_vertices.resize(verticesPerChar * totalCharCount);
	text_indices.resize(indexesPerChar * totalCharCount);
	currentTextCharCount = totalCharCount;	

	size_t currentCharCount = 0;
	for (size_t i = 0; i < textZonesCount; ++i)
	{
		const TextZone * textZone = &textZones[i];
		float x = textZone->x;
		float y = textZone->y;
		for (size_t j = 0; j < textZone->text.size(); ++j, ++currentCharCount)
		{
			const uint32_t firstChar = STB_FONT_consolas_24_latin1_FIRST_CHAR;
			const uint32_t vertexOffet = verticesPerChar * currentCharCount;
			const uint32_t indexOffset = indexesPerChar * currentCharCount;
			stb_fontchar *charData = &stbFontData[textZone->text[j] - firstChar];
			text_vertices[vertexOffet] = { { x + charData->x0 * charW, y + charData->y1 * charH, 0.0 }, { 0.5f, 0.0f, 0.0f }, { charData->s0, charData->t1 } };
			text_vertices[vertexOffet + 1] = { { x + charData->x1 * charW, y + charData->y1 * charH, 0.0 }, { 0.5f, 0.5f, 0.5f }, { charData->s1, charData->t1 } };
			text_vertices[vertexOffet + 2] = { { x + charData->x0 * charW, y + charData->y0 * charH, 0.0 }, { 0.0f, 0.0f, 0.5f }, { charData->s0, charData->t0 } };
			text_vertices[vertexOffet + 3] = { { x + charData->x1 * charW, y + charData->y0 * charH, 0.0 }, { 0.0f, 0.5f, 0.0f }, { charData->s1, charData->t0 } };

			text_indices[indexOffset] = vertexOffet;
			text_indices[indexOffset + 1] = vertexOffet + 1;
			text_indices[indexOffset + 2] = vertexOffet + 2;
			text_indices[indexOffset + 3] = vertexOffet + 1;
			text_indices[indexOffset + 4] = vertexOffet + 2;
			text_indices[indexOffset + 5] = vertexOffet + 3;

			x += charData->advance * charW;
		}
	}

	VkDeviceSize bufferSize = sizeof(text_vertices[0]) * text_vertices.size();
	UpdateBuffer( textVertexBufferMemory, text_vertices.data(), bufferSize);
	//copyDataToDeviceLocalMemory( textVertexBuffer, text_vertices.data(), bufferSize);
	/*VkMappedMemoryRange vertexMemoryRange = {};
	vertexMemoryRange.memory = textVertexBufferMemory;
	vertexMemoryRange.offset = 0;
	vertexMemoryRange.size = bufferSize;
	vertexMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	void* vdata;
	vkMapMemory(g_vk.device, textVertexBufferMemory, 0, bufferSize, 0, &vdata);
	memcpy(vdata, text_vertices.data(), bufferSize);
	vkFlushMappedMemoryRanges(g_vk.device, 1, &vertexMemoryRange);*/

	bufferSize = sizeof(text_indices[0]) * text_indices.size();
	UpdateBuffer(textIndexBufferMemory, text_indices.data(), bufferSize);
	//copyDataToDeviceLocalMemory( textIndexBuffer, text_indices.data(), bufferSize);
	/*VkMappedMemoryRange indexMemoryRange = {};
	indexMemoryRange.memory = textIndexBufferMemory;
	indexMemoryRange.offset = 0;
	indexMemoryRange.size = bufferSize;
	indexMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	void* idata;
	vkMapMemory(g_vk.device, textIndexBufferMemory, 0, bufferSize, 0, &idata);
	memcpy(idata, text_indices.data(), bufferSize);
	vkFlushMappedMemoryRanges(g_vk.device, 1, &indexMemoryRange);*/
}

void CreateTextVertexBuffer(size_t maxCharCount)
{
	maxTextCharCount = static_cast<uint32_t>(maxCharCount);

	
	VkDeviceSize bufferSize = sizeof(TextVertex) * maxCharCount * verticesPerChar;
	create_buffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, textVertexBuffer, textVertexBufferMemory);
	//createBufferToDeviceLocalMemory( bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &textVertexBuffer, &textVertexBufferMemory);

	bufferSize = sizeof(Index_t) * maxCharCount * indexesPerChar;
	create_buffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, textIndexBuffer, textIndexBufferMemory);
	//createBufferToDeviceLocalMemory( bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &textIndexBuffer, &textIndexBufferMemory);
}

void LoadFontTexture()
{
	const uint32_t fontWidth = STB_FONT_consolas_24_latin1_BITMAP_WIDTH;
	const uint32_t fontHeight = STB_FONT_consolas_24_latin1_BITMAP_WIDTH;

	static unsigned char font24pixels[fontWidth][fontHeight];
	stb_font_consolas_24_latin1(stbFontData, font24pixels, fontHeight);

	Load2DTexture(&font24pixels[0][0], fontWidth, fontHeight, 1, 1, VK_FORMAT_R8_UNORM, g_fontImage);
}

void AddTextRenderPass(const RenderPass* renderPass)
{
	textRenderPass = renderPass;
}

void InitTextRenderPass(const Swapchain& swapchain)
{
	CreateTextDescriptorSetLayout();
	CreateTextGraphicsPipeline(swapchain.extent);
}

void RecreateTextRenderPass(const Swapchain& swapchain)
{
	CreateTextGraphicsPipeline(swapchain.extent);
	//TODO:destroy and recreate the render pass after the swapchain is recreated in case the format changes
}

void CleanupTextRenderPassAfterSwapchain()
{
	vkDestroyPipeline(g_vk.device, textGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(g_vk.device, textPipelineLayout, nullptr);
}

void CleanupTextRenderPass()
{
	DestroyImage(g_fontImage);
	vkDestroyDescriptorSetLayout(g_vk.device, textDescriptorSetLayout, nullptr);
	vkDestroyBuffer(g_vk.device, textVertexBuffer, nullptr);
	vkFreeMemory(g_vk.device, textVertexBufferMemory, nullptr);
	vkDestroyBuffer(g_vk.device, textIndexBuffer, nullptr);
	vkFreeMemory(g_vk.device, textIndexBufferMemory, nullptr);
}