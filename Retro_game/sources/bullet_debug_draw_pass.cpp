#include "bullet_debug_draw_pass.h"
#include "..\shaders\shadersCommon.h"
#include "file_system.h"
#include "retro_physics.h"
#include "renderer.h"

struct BtDebugDrawState
{
	GfxModel debug_models[SIMULTANEOUS_FRAMES];
	uint32_t currentFrameIndex;
	size_t vertexOffset;
	size_t indexOffset;
} g_btDebugDrawState;

constexpr uint32_t maxVertices = 5024;
constexpr uint32_t maxIndices = maxVertices * 3;

void CreateBtDebudModels()
{
	std::vector<R_HW::VIDesc> modelVIDescs = {
		{ (R_HW::VIDataType )eVIDataType::POSITION, R_HW::eVIDataElementType::FLOAT, 3 },
		{ (R_HW::VIDataType )eVIDataType::COLOR, R_HW::eVIDataElementType::FLOAT, 3 },
	};

	for( GfxModel& debug_model : g_btDebugDrawState.debug_models  )
		debug_model = CreateGfxModel( modelVIDescs, maxVertices, maxIndices, sizeof( uint32_t ) );
}

R_HW::GpuPipelineLayout GetBtDebugPipelineLayout()
{
	return R_HW::GpuPipelineLayout();
}

R_HW::GpuPipelineStateDesc GetBtDebugPipelineState()
{
	R_HW::GpuPipelineStateDesc gpuPipelineState = {};
	GetBindingDescription( VIBindings_PosColUV, &gpuPipelineState.viState );

	gpuPipelineState.shaders = {
		{ FS::readFile( "shaders/line_draw.vert.spv" ), "main", R_HW::GFX_SHADER_STAGE_VERTEX_BIT },
		{ FS::readFile( "shaders/line_draw.frag.spv" ), "main", R_HW::GFX_SHADER_STAGE_FRAGMENT_BIT } };

	gpuPipelineState.rasterizationState.backFaceCulling = false;
	gpuPipelineState.rasterizationState.depthBiased = false;

	gpuPipelineState.depthStencilState.depthRead = false;
	gpuPipelineState.depthStencilState.depthWrite = false;
	gpuPipelineState.depthStencilState.depthCompareOp = R_HW::GfxCompareOp::LESS;

	gpuPipelineState.blendEnabled = false;
	gpuPipelineState.primitiveTopology = R_HW::GfxPrimitiveTopology::LINE_LIST;

	gpuPipelineState.lineWidth = 1.0f;
	gpuPipelineState.polygonMode = R_HW::GfxPolygonMode::FILL;

	return gpuPipelineState;
}

static void CmdBeginRenderPass( VkCommandBuffer commandBuffer, uint32_t currentFrame, const R_HW::RenderPass * renderpass, const Technique * technique )
{
	R_HW::CmdBeginLabel( commandBuffer, "Bullet debug", glm::vec4( 0.3f, 0.2f, 0.8f, 1.0f ) );

	const R_HW::FrameBuffer& frameBuffer = renderpass->outputFrameBuffer[currentFrame];
	R_HW::BeginRenderPass( commandBuffer, *renderpass, frameBuffer );

	BeginTechnique( commandBuffer, technique, currentFrame );
}

static void CmdEndRenderPass( VkCommandBuffer vkCommandBuffer )
{
	R_HW::EndRenderPass( vkCommandBuffer );
	R_HW::CmdEndLabel( vkCommandBuffer );
}

void BtDebugRecordDrawCommandsBuffer( R_HW::GfxCommandBuffer graphicsCommandBuffer, const FG::TaskInputData& inputData )
{
	//assert( g_btDebugDrawState.currentFrameIndex == inputData.currentFrame );

	const SceneFrameData* frameData = static_cast< const SceneFrameData* >(inputData.userData);
	CmdBeginRenderPass( graphicsCommandBuffer, inputData.currentFrame, inputData.renderpass, inputData.technique );

	CmdDrawIndexed( graphicsCommandBuffer, VIBindingLayout_PosCol, g_btDebugDrawState.debug_models[inputData.currentFrame]);

	CmdEndRenderPass( graphicsCommandBuffer );

	g_btDebugDrawState.currentFrameIndex = (g_btDebugDrawState.currentFrameIndex + 1) % SIMULTANEOUS_FRAMES;
	g_btDebugDrawState.vertexOffset = 0;
	g_btDebugDrawState.indexOffset = 0;
}


void phs::BulletDebugDraw::drawLine( const btVector3& from, const btVector3& to, const btVector3& color )
{
	constexpr uint32_t num_vertices = 2;
	std::array<glm::vec3, num_vertices> text_vertex_positions;
	std::array<glm::vec3, num_vertices> text_vertex_color;
	std::array<uint32_t, num_vertices>	text_indices;

	assert( g_btDebugDrawState.vertexOffset + num_vertices < maxVertices );
	assert( g_btDebugDrawState.indexOffset + num_vertices < maxIndices );

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

	text_indices[0] = g_btDebugDrawState.vertexOffset;
	text_indices[1] = g_btDebugDrawState.vertexOffset + 1;

	GfxModel* debug_model = &g_btDebugDrawState.debug_models[g_btDebugDrawState.currentFrameIndex];

	R_HW::GfxDeviceSize bufferSize = sizeof( text_vertex_positions[0] ) * num_vertices;
	UpdateGpuBuffer( &GetVertexInput( *debug_model, eVIDataType::POSITION )->buffer, text_vertex_positions.data(), bufferSize, g_btDebugDrawState.vertexOffset * sizeof(glm::vec3) );

	bufferSize = sizeof( text_vertex_color[0] ) * num_vertices;
	UpdateGpuBuffer( &GetVertexInput( *debug_model, eVIDataType::COLOR )->buffer, text_vertex_color.data(), bufferSize, g_btDebugDrawState.vertexOffset * sizeof( glm::vec3 ) );

	bufferSize = sizeof( text_indices[0] ) * num_vertices;
	UpdateGpuBuffer( &debug_model->indexBuffer, text_indices.data(), bufferSize, g_btDebugDrawState.indexOffset * sizeof( text_indices[0] ) );

	g_btDebugDrawState.vertexOffset += num_vertices;
	g_btDebugDrawState.indexOffset += num_vertices;
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