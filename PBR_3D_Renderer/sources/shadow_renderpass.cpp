#include "shadow_renderpass.h"
#include "..\shaders\shadersCommon.h"

#include "file_system.h"
#include "renderer.h"
#include "vk_commands.h"
#include "vk_debug.h"

#include "glm/gtc/matrix_transform.hpp"

//TODO: this is redundant, also found in frame graph
constexpr VkExtent2D RT_EXTENT_SHADOW = { 1024, 1024 };

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

GpuPipelineState GetShadowPipelineState()
{
	GpuPipelineState gpuPipelineState = {};
	uint32_t bindingCount = GetBindingDescription( VIBindingsMeshOnly, &gpuPipelineState.viState );

	gpuPipelineState.shaders = {
		{ FS::readFile( "shaders/shadows.vert.spv" ), "main", VK_SHADER_STAGE_VERTEX_BIT } };

	gpuPipelineState.rasterizationState.backFaceCulling = true;
	gpuPipelineState.rasterizationState.depthBiased = true;

	gpuPipelineState.depthStencilState.depthRead = true;
	gpuPipelineState.depthStencilState.depthWrite = true;
	gpuPipelineState.depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;

	gpuPipelineState.blendEnabled = false;
	gpuPipelineState.framebufferExtent = RT_EXTENT_SHADOW;
	gpuPipelineState.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	return gpuPipelineState;
}

/*
	Draw stuff
*/

static void CmdBeginShadowPass( VkCommandBuffer commandBuffer, size_t currentFrame, const RenderPass * renderpass, const Technique * technique )
{
	CmdBeginVkLabel( commandBuffer, "Shadow Renderpass", glm::vec4( 0.5f, 0.2f, 0.4f, 1.0f ) );
	BeginRenderPass( commandBuffer, *renderpass, renderpass->frameBuffer.frameBuffer, renderpass->frameBuffer.extent );

	BeginTechnique( commandBuffer, technique, currentFrame );
}

static void CmdDrawModel( VkCommandBuffer commandBuffer, const SceneInstanceSet* instanceSet, const GfxModel* modelAsset, uint32_t currentFrame, const Technique * technique )
{
	vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, technique->pipelineLayout, INSTANCE_SET, 1,
		&technique->instance_descriptor[currentFrame], 1, &instanceSet->geometryBufferOffsets );
	CmdDrawIndexed( commandBuffer, VIBindingsMeshOnly, *modelAsset );
}

static void CmdEndShadowPass( VkCommandBuffer commandBuffer )
{
	EndRenderPass( commandBuffer );
	CmdEndVkLabel( commandBuffer );
}

void ShadowRecordDrawCommandsBuffer(uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent, const RenderPass * renderpass, const Technique * technique )
{
	CmdBeginShadowPass(graphicsCommandBuffer, currentFrame, renderpass, technique);
	for (size_t i = 0; i < frameData->drawList.size(); ++i)
	{
		const DrawModel* renderable = &frameData->drawList[i];
		CmdDrawModel(graphicsCommandBuffer, &renderable->descriptorSet, renderable->asset->modelAsset, currentFrame, technique);
	}
	CmdEndShadowPass(graphicsCommandBuffer);
}