#include "bullet_debug_draw_pass.h"
#include "..\shaders\shadersCommon.h"
#include "file_system.h"
#include "retro_physics.h"
#include "renderer.h"

GfxModel debug_models[SIMULTANEOUS_FRAMES];
uint32_t currentFrameIndex;
size_t vertexOffset;
size_t indexOffset;

constexpr uint32_t maxVertices = 5024;
constexpr uint32_t maxIndices = maxVertices * 3;

void CreateBtDebudModels()
{
	std::vector<VIDesc> modelVIDescs = {
		{ ( VIDataType )eVIDataType::POSITION, eVIDataElementType::FLOAT, 3 },
		{ ( VIDataType )eVIDataType::COLOR, eVIDataElementType::FLOAT, 3 },
	};

	for( GfxModel& debug_model : debug_models  )
		debug_model = CreateGfxModel( modelVIDescs, maxVertices, maxIndices, sizeof( uint32_t ) );
}

GpuPipelineLayout GetBtDebugPipelineLayout()
{
	return GpuPipelineLayout();
}

GpuPipelineStateDesc GetBtDebugPipelineState()
{
	GpuPipelineStateDesc gpuPipelineState = {};
	GetBindingDescription( VIBindings_PosColUV, &gpuPipelineState.viState );

	gpuPipelineState.shaders = {
		{ FS::readFile( "shaders/line_draw.vert.spv" ), "main", GFX_SHADER_STAGE_VERTEX_BIT },
		{ FS::readFile( "shaders/line_draw.frag.spv" ), "main", GFX_SHADER_STAGE_FRAGMENT_BIT } };

	gpuPipelineState.rasterizationState.backFaceCulling = false;
	gpuPipelineState.rasterizationState.depthBiased = false;

	gpuPipelineState.depthStencilState.depthRead = false;
	gpuPipelineState.depthStencilState.depthWrite = false;
	gpuPipelineState.depthStencilState.depthCompareOp = GfxCompareOp::LESS;

	gpuPipelineState.blendEnabled = false;
	gpuPipelineState.primitiveTopology = GfxPrimitiveTopology::LINE_LIST;

	gpuPipelineState.lineWidth = 2.0f;
	gpuPipelineState.polygonMode = GfxPolygonMode::FILL;

	return gpuPipelineState;
}

static void CmdBeginRenderPass( VkCommandBuffer commandBuffer, uint32_t currentFrame, const RenderPass * renderpass, const Technique * technique )
{
	CmdBeginLabel( commandBuffer, "Bullet debug", glm::vec4( 0.3f, 0.2f, 0.8f, 1.0f ) );

	const FrameBuffer& frameBuffer = renderpass->outputFrameBuffer[currentFrame];
	BeginRenderPass( commandBuffer, *renderpass, frameBuffer );

	BeginTechnique( commandBuffer, technique, currentFrame );
}

static void CmdEndRenderPass( VkCommandBuffer vkCommandBuffer )
{
	EndRenderPass( vkCommandBuffer );
	CmdEndLabel( vkCommandBuffer );
}

void BtDebugRecordDrawCommandsBuffer( GfxCommandBuffer graphicsCommandBuffer, const FG::TaskInputData& inputData )
{
	assert( currentFrameIndex == inputData.currentFrame );

	const SceneFrameData* frameData = static_cast< const SceneFrameData* >(inputData.userData);
	CmdBeginRenderPass( graphicsCommandBuffer, inputData.currentFrame, inputData.renderpass, inputData.technique );

	CmdDrawIndexed( graphicsCommandBuffer, VIBindingLayout_PosCol, debug_models[inputData.currentFrame]);

	CmdEndRenderPass( graphicsCommandBuffer );

	currentFrameIndex = (currentFrameIndex + 1) % SIMULTANEOUS_FRAMES;
	vertexOffset = 0;
	indexOffset = 0;
}


void phs::BulletDebugDraw::drawLine( const btVector3& from, const btVector3& to, const btVector3& color )
{
	constexpr uint32_t num_vertices = 2;
	std::array<glm::vec3, num_vertices> text_vertex_positions;
	std::array<glm::vec3, num_vertices> text_vertex_color;
	std::array<uint32_t, num_vertices>	text_indices;

	assert( vertexOffset + num_vertices < maxVertices );
	assert( indexOffset + num_vertices < maxIndices );

	text_vertex_positions[0].x = from.x();
	text_vertex_positions[0].y = from.y();
	text_vertex_positions[0].z = from.z();

	text_vertex_positions[1].x = to.x();
	text_vertex_positions[1].y = to.y();
	text_vertex_positions[1].z = to.z();

	text_vertex_color[0].x = color.x();
	text_vertex_color[0].y = color.y();
	text_vertex_color[0].z = color.z();

	text_vertex_color[1] = text_vertex_color[0];

	text_indices[0] = vertexOffset;
	text_indices[1] = vertexOffset + 1;

	GfxDeviceSize bufferSize = sizeof( text_vertex_positions[0] ) * num_vertices;
	UpdateGpuBuffer( &GetVertexInput( debug_models[currentFrameIndex], eVIDataType::POSITION )->buffer, text_vertex_positions.data(), bufferSize, vertexOffset * sizeof(glm::vec3) );

	bufferSize = sizeof( text_vertex_color[0] ) * num_vertices;
	UpdateGpuBuffer( &GetVertexInput( debug_models[currentFrameIndex], eVIDataType::COLOR )->buffer, text_vertex_color.data(), bufferSize, vertexOffset * sizeof( glm::vec3 ) );

	bufferSize = sizeof( text_indices[0] ) * num_vertices;
	UpdateGpuBuffer( &debug_models[currentFrameIndex].indexBuffer, text_indices.data(), bufferSize, indexOffset * sizeof( text_indices[0] ) );

	vertexOffset += num_vertices;
	indexOffset += num_vertices;
}

void phs::BulletDebugDraw::drawContactPoint( const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color )
{

}

void phs::BulletDebugDraw::reportErrorWarning( const char* warningString )
{

}

void phs::BulletDebugDraw::draw3dText( const btVector3& location, const char* textString )
{

}

void phs::BulletDebugDraw::setDebugMode( int debugMode )
{
	m_debug_mode = debugMode;
}

int phs::BulletDebugDraw::getDebugMode() const
{
	return m_debug_mode;
}