#include "shadow_renderpass.h"
#include "..\shaders\shadersCommon.h"

#include "file_system.h"
#include "renderer.h"

#include "glm/gtc/matrix_transform.hpp"

void computeShadowMatrix(const glm::vec3& light_location, glm::mat4* view, glm::mat4* projection)
{
	*view = glm::lookAt(light_location, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	//TODO: encapsulate projection computation
	*projection = glm::perspective(glm::radians(65.0f), 1.0f, 0.1f, 100.0f);
	(*projection)[1][1] *= -1;//Compensate for OpenGL Y coordinate being inverted

	//return light_projection_matrix * light_view_matrix;
}

void UpdateShadowUniformBuffers( R_HW::GpuBuffer* shadowSceneUniformBuffer, const SceneMatricesUniform* sceneUniforms)
{
	//TODO don't pass in the perFrameBuffer but just the right one, doesn't need to know the frame
	//per Instance data should be updated by the geometry render pass
	//We only update the scene related data
	UpdateGpuBuffer( shadowSceneUniformBuffer, sceneUniforms, sizeof( SceneMatricesUniform ), 0 );
}

R_HW::GpuPipelineLayout GetShadowPipelineLayout()
{
	return R_HW::GpuPipelineLayout();
}

R_HW::GpuPipelineStateDesc GetShadowPipelineState()
{
	R_HW::GpuPipelineStateDesc gpuPipelineState = {};
	uint32_t bindingCount = GetBindingDescription( VIBindingsMeshOnly, &gpuPipelineState.viState );

	gpuPipelineState.shaders = {
		{ FS::readFile( "shaders/shadows.vert.spv" ), "main", R_HW::GFX_SHADER_STAGE_VERTEX_BIT }, };

	gpuPipelineState.rasterizationState.backFaceCulling = true;
	gpuPipelineState.rasterizationState.depthBiased = true;

	gpuPipelineState.depthStencilState.depthRead = true;
	gpuPipelineState.depthStencilState.depthWrite = true;
	gpuPipelineState.depthStencilState.depthCompareOp = R_HW::GfxCompareOp::LESS;

	gpuPipelineState.blendEnabled = false;
	gpuPipelineState.primitiveTopology = R_HW::GfxPrimitiveTopology::TRIANGLE_LIST;

	return gpuPipelineState;
}

/*
	Draw stuff
*/

static void CmdBeginShadowPass( R_HW::GfxCommandBuffer commandBuffer, size_t currentFrame, const R_HW::RenderPass * renderpass, const Technique * technique )
{
	R_HW::CmdBeginLabel( commandBuffer, "Shadow Renderpass", glm::vec4( 0.5f, 0.2f, 0.4f, 1.0f ) );

	const R_HW::FrameBuffer& frameBuffer = renderpass->outputFrameBuffer[currentFrame];
	R_HW::BeginRenderPass( commandBuffer, *renderpass, frameBuffer );

	BeginTechnique( commandBuffer, technique, currentFrame );
}

static void CmdDrawModel( R_HW::GfxCommandBuffer commandBuffer, const DrawListEntry* drawModel, uint32_t currentFrame, const Technique * technique )
{
	const SceneInstanceSet* instanceSet = &drawModel->descriptorSet;
	const GfxModel* modelAsset = drawModel->asset->modelAsset;
	R_HW::CmdBindRootDescriptor( commandBuffer, R_HW::GfxPipelineBindPoint::GRAPHICS, technique->pipelineLayout, INSTANCE_SET, technique->descriptor_sets[INSTANCE_SET].hw_descriptorSets[currentFrame],
		instanceSet->geometryBufferOffsets );

	CmdDrawIndexed( commandBuffer, VIBindingsMeshOnly, *modelAsset );
}

static void CmdEndShadowPass( R_HW::GfxCommandBuffer commandBuffer )
{
	R_HW::EndRenderPass( commandBuffer );
	R_HW::CmdEndLabel( commandBuffer );
}

void ShadowRecordDrawCommandsBuffer( R_HW::GfxCommandBuffer graphicsCommandBuffer, const FG::TaskInputData& inputData )
{
	const SceneFrameData* frameData = static_cast< const SceneFrameData* >(inputData.userData);
	CmdBeginShadowPass( graphicsCommandBuffer, inputData.currentFrame, inputData.renderpass, inputData.technique );

	for (size_t i = 0; i < frameData->drawList.size(); ++i)
	{
		const DrawListEntry* renderable = &frameData->drawList[i];
		CmdDrawModel(graphicsCommandBuffer, renderable, inputData.currentFrame, inputData.technique);
	}
	CmdEndShadowPass(graphicsCommandBuffer);
}