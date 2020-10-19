#include "text_overlay.h"

#include "file_system.h"
#include "renderer.h"
#include "vk_commands.h"
#include "vk_debug.h"
#include "vk_vertex_input.h"
#include "gfx_heaps_batched_allocator.h"
#include "gfx_model.h"
#include "stb_font_consolas_24_latin1.inl"
#include "../shaders/shadersCommon.h"

GfxModel textModel;
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

GpuPipelineStateDesc GetTextPipelineState()
{
	GpuPipelineStateDesc gpuPipelineState = {};
	GetBindingDescription( VIBindings_PosColUV, &gpuPipelineState.viState );

	gpuPipelineState.shaders = {
		{ FS::readFile( "shaders/text.vert.spv" ), "main", GFX_SHADER_STAGE_VERTEX_BIT },
		{ FS::readFile( "shaders/text.frag.spv" ), "main", GFX_SHADER_STAGE_FRAGMENT_BIT } };

	gpuPipelineState.rasterizationState.backFaceCulling = false;
	gpuPipelineState.rasterizationState.depthBiased = false;

	gpuPipelineState.depthStencilState.depthRead = false;
	gpuPipelineState.depthStencilState.depthWrite = false;
	gpuPipelineState.depthStencilState.depthCompareOp = GfxCompareOp::LESS;

	gpuPipelineState.blendEnabled = true;
	gpuPipelineState.primitiveTopology = GfxPrimitiveTopology::TRIANGLE_LIST;
	return gpuPipelineState;
}

static void CmdDrawText( GfxCommandBuffer commandBuffer, VkExtent2D extent, size_t frameIndex, const RenderPass * renderpass, const Technique * technique )
{
	CmdBeginLabel(commandBuffer, "Text overlay Renderpass", glm::vec4(0.6f, 0.6f, 0.6f, 1.0f));
	const FrameBuffer& frameBuffer = renderpass->outputFrameBuffer[frameIndex];
	BeginRenderPass(commandBuffer, *renderpass, frameBuffer);

	CmdBindPipeline( commandBuffer, GfxPipelineBindPoint::GRAPHICS, technique->pipeline );
	CmdBindDescriptorTable( commandBuffer, GfxPipelineBindPoint::GRAPHICS, technique->pipelineLayout, RENDERPASS_SET, technique->descriptor_sets[RENDERPASS_SET].hw_descriptorSets[0] );

	CmdDrawIndexed( commandBuffer, VIBindings_PosColUV, textModel, currentTextCharCount * indexesPerChar );

	EndRenderPass(commandBuffer);
	CmdEndLabel(commandBuffer);
}

void UpdateText( const TextZone * textZones, size_t textZonesCount, VkExtent2D surfaceExtent)
{
	const float charW = 1.5f / surfaceExtent.width;
	const float charH = 1.5f / surfaceExtent.height;

	std::vector<glm::vec3> text_vertex_positions;
	std::vector<glm::vec3> text_vertex_color;
	std::vector<glm::vec2> text_vertex_uv;
	std::vector<Index_t>	text_indices;
	size_t totalCharCount = 0;
	for (size_t i = 0; i < textZonesCount; ++i)
		totalCharCount += textZones[i].text.size();

	assert(totalCharCount < maxTextCharCount);
	text_vertex_positions.resize(verticesPerChar * totalCharCount);
	text_vertex_color.resize( verticesPerChar * totalCharCount );
	text_vertex_uv.resize( verticesPerChar * totalCharCount );
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

			text_vertex_positions[vertexOffet] = { x + charData->x0 * charW, y + charData->y1 * charH, 0.0 };
			text_vertex_color[vertexOffet] = { 0.5f, 0.0f, 0.0f };
			text_vertex_uv[vertexOffet] = { charData->s0, charData->t1 };

			text_vertex_positions[vertexOffet + 1] = { x + charData->x1 * charW, y + charData->y1 * charH, 0.0 };
			text_vertex_color[vertexOffet + 1] = { 0.5f, 0.5f, 0.5f };
			text_vertex_uv[vertexOffet + 1] = { charData->s1, charData->t1 };

			text_vertex_positions[vertexOffet + 2] = { x + charData->x0 * charW, y + charData->y0 * charH, 0.0 };
			text_vertex_color[vertexOffet + 2] = { 0.0f, 0.0f, 0.5f };
			text_vertex_uv[vertexOffet + 2] = { charData->s0, charData->t0 };

			text_vertex_positions[vertexOffet + 3] = { x + charData->x1 * charW, y + charData->y0 * charH, 0.0 };
			text_vertex_color[vertexOffet + 3] = { 0.0f, 0.5f, 0.0f };
			text_vertex_uv[vertexOffet + 3] = { charData->s1, charData->t0 };

			text_indices[indexOffset] = vertexOffet;
			text_indices[indexOffset + 1] = vertexOffet + 1;
			text_indices[indexOffset + 2] = vertexOffet + 2;
			text_indices[indexOffset + 3] = vertexOffet + 1;
			text_indices[indexOffset + 4] = vertexOffet + 2;
			text_indices[indexOffset + 5] = vertexOffet + 3;

			x += charData->advance * charW;
		}
	}

	GfxDeviceSize bufferSize = sizeof( text_vertex_positions[0] ) * text_vertex_positions.size();
	UpdateGpuBuffer( &GetVertexInput( textModel, eVIDataType::POSITION )->buffer, text_vertex_positions.data(), bufferSize, 0 );

	bufferSize = sizeof( text_vertex_color[0] ) * text_vertex_color.size();
	UpdateGpuBuffer( &GetVertexInput( textModel, eVIDataType::COLOR )->buffer, text_vertex_color.data(), bufferSize, 0 );

	bufferSize = sizeof( text_vertex_uv[0] ) * text_vertex_uv.size();
	UpdateGpuBuffer( &GetVertexInput( textModel, eVIDataType::TEX_COORD )->buffer, text_vertex_uv.data(), bufferSize, 0 );

	bufferSize = sizeof(text_indices[0]) * text_indices.size();
	UpdateGpuBuffer( &textModel.indexBuffer, text_indices.data(), bufferSize, 0 );

	//textModel.indexCount = currentTextCharCount * indexesPerChar;
}

void CreateTextVertexBuffer(size_t maxCharCount)
{
	maxTextCharCount = static_cast< uint32_t >(maxCharCount);

	uint32_t maxVertices = maxCharCount * verticesPerChar;
	uint32_t maxIndices = maxCharCount * indexesPerChar;

	std::vector<VIDesc> modelVIDescs = {
		{ ( VIDataType )eVIDataType::POSITION, eVIDataElementType::FLOAT, 3 },
		{ ( VIDataType )eVIDataType::COLOR, eVIDataElementType::FLOAT, 3 },
		{ ( VIDataType )eVIDataType::TEX_COORD, eVIDataElementType::FLOAT, 2 },
	};
	
	textModel = CreateGfxModel( modelVIDescs, maxVertices, maxIndices, sizeof( uint32_t ) );
}

void LoadFontTexture()
{
	const uint32_t fontWidth = STB_FONT_consolas_24_latin1_BITMAP_WIDTH;
	const uint32_t fontHeight = STB_FONT_consolas_24_latin1_BITMAP_WIDTH;

	static unsigned char font24pixels[fontWidth][fontHeight];
	stb_font_consolas_24_latin1(stbFontData, font24pixels, fontHeight);

	GfxHeaps_CommitedResourceAllocator allocator = {};
	allocator.Prepare();
	Load2DTexture( &font24pixels[0][0], fontWidth, fontHeight, 1, GfxFormat::R8_UNORM, &g_fontImage, &allocator );
	allocator.Commit();
}

void CleanupTextRenderPass()
{
	DestroyImage( &g_fontImage );
	DestroyGfxModel( textModel );
}

void TextRecordDrawCommandsBuffer(uint32_t currentFrame, const SceneFrameData* frameData, GfxCommandBuffer graphicsCommandBuffer, VkExtent2D extent, const RenderPass * renderpass, const Technique * technique )
{
	CmdDrawText(graphicsCommandBuffer, extent, currentFrame, renderpass, technique);
}