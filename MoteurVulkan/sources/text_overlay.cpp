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

GpuBuffer textVertexBuffer;
GpuBuffer textIndexBuffer;
uint32_t maxTextCharCount = 0;
uint32_t currentTextCharCount = 0;

stb_fontchar stbFontData[STB_FONT_consolas_24_latin1_NUM_CHARS];
GfxImage g_fontImage;

typedef uint32_t Index_t;

const uint32_t verticesPerChar = 4;
const uint32_t indexesPerChar = 6;

const GfxImage* GetTextImage()
{
	return &g_fontImage;
}

GpuPipelineLayout GetTextPipelineLayout()
{
	return GpuPipelineLayout();
}

GpuPipelineState GetTextPipelineState()
{
	GpuPipelineState gpuPipelineState = {};
	gpuPipelineState.viState.vibDescription[0] = TextVertex::get_binding_description();
	gpuPipelineState.viState.vibDescriptionsCount = 1;

	auto attributeDescriptions = TextVertex::get_attribute_descriptions();
	assert( attributeDescriptions.size() <= VI_STATE_MAX_DESCRIPTIONS );
	gpuPipelineState.viState.visDescriptionsCount = attributeDescriptions.size();
	for( size_t i = 0; i < attributeDescriptions.size(); ++i )
		gpuPipelineState.viState.visDescriptions[i] = attributeDescriptions[i];

	gpuPipelineState.shaders = {
		{ FS::readFile( "shaders/text.vert.spv" ), "main", VK_SHADER_STAGE_VERTEX_BIT },
		{ FS::readFile( "shaders/text.frag.spv" ), "main", VK_SHADER_STAGE_FRAGMENT_BIT } };

	gpuPipelineState.rasterizationState.backFaceCulling = false;
	gpuPipelineState.rasterizationState.depthBiased = false;

	gpuPipelineState.depthStencilState.depthRead = false;
	gpuPipelineState.depthStencilState.depthWrite = false;
	gpuPipelineState.depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;

	gpuPipelineState.framebufferExtent = { 0,0 }; //swapchain sized;
	gpuPipelineState.blendEnabled = true;
	gpuPipelineState.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	return gpuPipelineState;
}

static void CmdDrawText( VkCommandBuffer commandBuffer, VkExtent2D extent, size_t frameIndex, const RenderPass * renderpass, const Technique * technique )
{
	CmdBeginVkLabel(commandBuffer, "Text overlay Renderpass", glm::vec4(0.6f, 0.6f, 0.6f, 1.0f));
	BeginRenderPass(commandBuffer, *renderpass, renderpass->outputFrameBuffer[frameIndex].frameBuffer, extent);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, technique->pipeline );
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, technique->pipelineLayout, 0, 1, &technique->renderPass_descriptor[0], 0, nullptr);

	VkBuffer vertexBuffers[] = { textVertexBuffer.buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, textIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

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
	UpdateGpuBuffer( &textVertexBuffer, text_vertices.data(), bufferSize, 0 );

	bufferSize = sizeof(text_indices[0]) * text_indices.size();
	UpdateGpuBuffer( &textIndexBuffer, text_indices.data(), bufferSize, 0 );
}

//TODO: Use the system for vertex buffers
void CreateTextVertexBuffer(size_t maxCharCount)
{
	maxTextCharCount = static_cast<uint32_t>(maxCharCount);

	
	VkDeviceSize bufferSize = sizeof(TextVertex) * maxCharCount * verticesPerChar;
	CreateCommitedGpuBuffer( bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &textVertexBuffer );

	bufferSize = sizeof(Index_t) * maxCharCount * indexesPerChar;
	CreateCommitedGpuBuffer( bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &textIndexBuffer );
}

void LoadFontTexture()
{
	const uint32_t fontWidth = STB_FONT_consolas_24_latin1_BITMAP_WIDTH;
	const uint32_t fontHeight = STB_FONT_consolas_24_latin1_BITMAP_WIDTH;

	static unsigned char font24pixels[fontWidth][fontHeight];
	stb_font_consolas_24_latin1(stbFontData, font24pixels, fontHeight);

	Load2DTexture(&font24pixels[0][0], fontWidth, fontHeight, 1, 1, VK_FORMAT_R8_UNORM, g_fontImage);
}

void CleanupTextRenderPass()
{
	DestroyImage(g_fontImage);
	DestroyCommitedGpuBuffer( &textVertexBuffer );
	DestroyCommitedGpuBuffer( &textIndexBuffer );
}

void TextRecordDrawCommandsBuffer(uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent, const RenderPass * renderpass, const Technique * technique )
{
	CmdDrawText(graphicsCommandBuffer, extent, currentFrame, renderpass, technique);
}