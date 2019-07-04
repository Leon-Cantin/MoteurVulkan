#include "text_overlay.h"

#include "model_asset.h"
#include "vk_shader.h"
#include "file_system.h"
#include "stb_font_consolas_24_latin1.inl"
#include "vk_buffer.h"
#include "descriptors.h"
#include "swapchain.h"
#include "vk_debug.h"
#include "console_command.h"
#include "material.h"

GfxMaterial textMaterial;
const RenderPass* textRenderPass;
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

static void CreateTextDescriptorSetLayout(Technique* technique)
{
	const VkDescriptorSetLayoutBinding samplerLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };

	const std::array<VkDescriptorSetLayoutBinding, 1> bindings = { samplerLayoutBinding };
	CreateDesciptorSetLayout(bindings.data(), static_cast<uint32_t>(bindings.size()), &technique->renderpass_descriptor_layout);
}

void CreateTextDescriptorSet(VkDescriptorPool descriptorPool, VkSampler trilinearSampler)
{
	Technique* technique = &textMaterial.techniques[0];
	DescriptorSet descriptorSet = {};
	descriptorSet.descriptors.resize(1);
	descriptorSet.descriptors[0] = { {}, { trilinearSampler, g_fontImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0 };
	descriptorSet.layout = technique->renderpass_descriptor_layout;
	createDescriptorSets(descriptorPool, 1, &descriptorSet);

	technique->renderPass_descriptor[0] = descriptorSet.set;
}

static void CreateTextTechnique( VkExtent2D extent, Technique* technique)
{
	//Create layout
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1; // Optional
	pipeline_layout_info.pSetLayouts = &technique->renderpass_descriptor_layout; // Optional
	pipeline_layout_info.pushConstantRangeCount = 0; // Optional
	pipeline_layout_info.pPushConstantRanges = nullptr; // Optional

	if( vkCreatePipelineLayout( g_vk.device, &pipeline_layout_info, nullptr, &technique->pipelineLayout ) != VK_SUCCESS ) {
		throw std::runtime_error( "failed to create pipeline layout!" );
	}

	//Create PSO
	auto bindingDescription = TextVertex::get_binding_description();
	auto attributeDescriptions = TextVertex::get_attribute_descriptions();

	std::vector<char> vertShaderCode = FS::readFile("shaders/text.vert.spv");
	std::vector<char> fragShaderCode = FS::readFile("shaders/text.frag.spv");

	VICreation viState = { &bindingDescription, 1, attributeDescriptions.data(), static_cast< uint32_t >(attributeDescriptions.size()) };
	//TODO: these cast are dangerous for alligment
	std::vector<ShaderCreation> shaderState = {
		{ reinterpret_cast< uint32_t* >(vertShaderCode.data()), vertShaderCode.size(), "main", VK_SHADER_STAGE_VERTEX_BIT },
		{ reinterpret_cast< uint32_t* >(fragShaderCode.data()), fragShaderCode.size(), "main", VK_SHADER_STAGE_FRAGMENT_BIT } };
	RasterizationState rasterizationState;
	rasterizationState.backFaceCulling = false;
	rasterizationState.depthBiased = false;
	DepthStencilState depthStencilState;
	depthStencilState.depthRead = false;
	depthStencilState.depthWrite = false;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;

	CreatePipeline(
		viState,
		shaderState,
		extent,
		textRenderPass->vk_renderpass,
		technique->pipelineLayout,
		rasterizationState,
		depthStencilState,
		true,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		&technique->pipeline );
}

void CmdDrawText( VkCommandBuffer commandBuffer, VkExtent2D extent, size_t frameIndex)
{
	CmdBeginVkLabel(commandBuffer, "Text overlay Renderpass", glm::vec4(0.6f, 0.6f, 0.6f, 1.0f));
	BeginRenderPass(commandBuffer, *textRenderPass, textRenderPass->outputFrameBuffer[frameIndex].frameBuffer, extent);

	Technique* technique = &textMaterial.techniques[0];
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, technique->pipeline );
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, technique->pipelineLayout, 0, 1, &technique->renderPass_descriptor[0], 0, nullptr);

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

static void CreateTextPipeline(const Swapchain& swapchain)
{
	Technique* technique = &textMaterial.techniques[0];
	CreateTextDescriptorSetLayout( technique );
	CreateTextTechnique( swapchain.extent, technique );
}

void RecreateTextRenderPassAfterSwapchain(const Swapchain* swapchain)
{
	Technique* technique = &textMaterial.techniques[0];
	CreateTextTechnique( swapchain->extent, technique );
	//TODO:destroy and recreate the render pass after the swapchain is recreated in case the format changes
}

void CleanupTextRenderPassAfterSwapchain()
{
	Technique* technique = &textMaterial.techniques[0];
	vkDestroyPipeline(g_vk.device, technique->pipeline, nullptr);
	vkDestroyPipelineLayout(g_vk.device, technique->pipelineLayout, nullptr);
}

void CleanupTextRenderPass()
{
	Technique* technique = &textMaterial.techniques[0];
	DestroyImage(g_fontImage);
	vkDestroyDescriptorSetLayout(g_vk.device, technique->renderpass_descriptor_layout, nullptr);
	vkDestroyBuffer(g_vk.device, textVertexBuffer, nullptr);
	vkFreeMemory(g_vk.device, textVertexBufferMemory, nullptr);
	vkDestroyBuffer(g_vk.device, textIndexBuffer, nullptr);
	vkFreeMemory(g_vk.device, textIndexBufferMemory, nullptr);
}

void InitializeTextRenderPass(const RenderPass* renderpass, const Swapchain* swapchain)
{
	textRenderPass = renderpass;
	CreateTextPipeline(*swapchain);
}

void TextRecordDrawCommandsBuffer(uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent)
{
	CmdDrawText(graphicsCommandBuffer, extent, currentFrame);
}