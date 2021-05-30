#include "skybox.h"

#include "file_system.h"
#include "renderer.h"

#include <glm/gtc/matrix_transform.hpp>

R_HW::GpuPipelineLayout GetSkyboxPipelineLayout()
{
	return R_HW::GpuPipelineLayout();
}

R_HW::GpuPipelineStateDesc GetSkyboxPipelineState()
{
	R_HW::GpuPipelineStateDesc gpuPipelineState = {};
	gpuPipelineState.shaders = {
		{ FS::readFile( "shaders/skybox.vert.spv" ), "main", R_HW::GFX_SHADER_STAGE_VERTEX_BIT },
		{ FS::readFile( "shaders/skybox.frag.spv" ), "main", R_HW::GFX_SHADER_STAGE_FRAGMENT_BIT } };

	gpuPipelineState.rasterizationState.backFaceCulling = false;
	gpuPipelineState.rasterizationState.depthBiased = false;

	gpuPipelineState.depthStencilState.depthRead = true;
	gpuPipelineState.depthStencilState.depthWrite = false;
	gpuPipelineState.depthStencilState.depthCompareOp = R_HW::GfxCompareOp::LESS_OR_EQUAL;

	gpuPipelineState.blendEnabled = false;
	gpuPipelineState.primitiveTopology = R_HW::GfxPrimitiveTopology::TRIANGLE_STRIP;

	return gpuPipelineState;
}

void UpdateSkyboxUniformBuffers( R_HW::GpuBuffer* skyboxUniformBuffer, const glm::mat4& world_view_matrix)
{
	SkyboxUniformBufferObject subo = {};
	subo.inv_view_matrix = glm::mat3(glm::scale(transpose(world_view_matrix), glm::vec3(1.0f, -1.0f, 1.0f)));

	UpdateGpuBuffer( skyboxUniformBuffer, &subo, sizeof( subo ), 0 );
}

void SkyboxRecordDrawCommandsBuffer( R_HW::GfxCommandBuffer commandBuffer, const FG::TaskInputData& inputData )
{
	R_HW::CmdBeginLabel( commandBuffer, "Skybox Renderpass", glm::vec4( 0.2f, 0.2f, 0.9f, 1.0f ) );
	const R_HW::FrameBuffer& frameBuffer = inputData.renderpass->outputFrameBuffer[inputData.currentFrame];
	BeginRenderPass( commandBuffer, *inputData.renderpass, frameBuffer );

	BeginTechnique( commandBuffer, inputData.technique, inputData.currentFrame );

	vkCmdDraw( commandBuffer, 4, 1, 0, 0 );

	R_HW::EndRenderPass( commandBuffer );
	R_HW::CmdEndLabel( commandBuffer );
}