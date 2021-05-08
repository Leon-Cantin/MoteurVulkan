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

void UpdateShadowUniformBuffers(GpuBuffer* shadowSceneUniformBuffer, const SceneMatricesUniform* sceneUniforms)
{
	//TODO don't pass in the perFrameBuffer but just the right one, doesn't need to know the frame
	//per Instance data should be updated by the geometry render pass
	//We only update the scene related data
	UpdateGpuBuffer( shadowSceneUniformBuffer, sceneUniforms, sizeof( SceneMatricesUniform ), 0 );
}

GpuPipelineLayout GetShadowPipelineLayout()
{
	return GpuPipelineLayout();
}

GpuPipelineStateDesc GetShadowPipelineState()
{
	GpuPipelineStateDesc gpuPipelineState = {};
	uint32_t bindingCount = GetBindingDescription( VIBindingsMeshOnly, &gpuPipelineState.viState );

	gpuPipelineState.shaders = {
		{ FS::readFile( "shaders/shadows.vert.spv" ), "main", GFX_SHADER_STAGE_VERTEX_BIT }, };

	gpuPipelineState.rasterizationState.backFaceCulling = true;
	gpuPipelineState.rasterizationState.depthBiased = true;

	gpuPipelineState.depthStencilState.depthRead = true;
	gpuPipelineState.depthStencilState.depthWrite = true;
	gpuPipelineState.depthStencilState.depthCompareOp = GfxCompareOp::LESS;

	gpuPipelineState.blendEnabled = false;
	gpuPipelineState.primitiveTopology = GfxPrimitiveTopology::TRIANGLE_LIST;

	return gpuPipelineState;
}

/*
	Draw stuff
*/

static void CmdBeginShadowPass( GfxCommandBuffer commandBuffer, size_t currentFrame, const RenderPass * renderpass, const Technique * technique )
{
	CmdBeginLabel( commandBuffer, "Shadow Renderpass", glm::vec4( 0.5f, 0.2f, 0.4f, 1.0f ) );

	const FrameBuffer& frameBuffer = renderpass->outputFrameBuffer[currentFrame];
	BeginRenderPass( commandBuffer, *renderpass, frameBuffer );

	BeginTechnique( commandBuffer, technique, currentFrame );
}

static void CmdDrawModel( GfxCommandBuffer commandBuffer, const DrawListEntry* drawModel, uint32_t currentFrame, const Technique * technique )
{
	const SceneInstanceSet* instanceSet = &drawModel->descriptorSet;
	const GfxModel* modelAsset = drawModel->asset->modelAsset;
	CmdBindRootDescriptor( commandBuffer, GfxPipelineBindPoint::GRAPHICS, technique->pipelineLayout, INSTANCE_SET, technique->descriptor_sets[INSTANCE_SET].hw_descriptorSets[currentFrame],
		instanceSet->geometryBufferOffsets );

	CmdDrawIndexed( commandBuffer, VIBindingsMeshOnly, *modelAsset );
}

static void CmdEndShadowPass( GfxCommandBuffer commandBuffer )
{
	EndRenderPass( commandBuffer );
	CmdEndLabel( commandBuffer );
}

void ShadowRecordDrawCommandsBuffer( GfxCommandBuffer graphicsCommandBuffer, const FG::TaskInputData& inputData )
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