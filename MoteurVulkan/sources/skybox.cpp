#include "skybox.h"

#include "file_system.h"
#include "renderer.h"
#include "vk_commands.h"
#include "vk_debug.h"

#include <glm/gtc/matrix_transform.hpp>

GpuPipelineLayout GetSkyboxPipelineLayout()
{
	return GpuPipelineLayout();
}

GpuPipelineState GetSkyboxPipelineState()
{
	GpuPipelineState gpuPipelineState = {};
	gpuPipelineState.shaders = {
		{ FS::readFile( "shaders/skybox.vert.spv" ), "main", VK_SHADER_STAGE_VERTEX_BIT },
		{ FS::readFile( "shaders/skybox.frag.spv" ), "main", VK_SHADER_STAGE_FRAGMENT_BIT } };

	gpuPipelineState.rasterizationState.backFaceCulling = false;
	gpuPipelineState.rasterizationState.depthBiased = false;

	gpuPipelineState.depthStencilState.depthRead = true;
	gpuPipelineState.depthStencilState.depthWrite = false;
	gpuPipelineState.depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	gpuPipelineState.framebufferExtent = { 0,0 }; // swapchain sized
	gpuPipelineState.blendEnabled = false;
	gpuPipelineState.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

	return gpuPipelineState;
}

void UpdateSkyboxUniformBuffers( GpuBuffer* skyboxUniformBuffer, const glm::mat4& world_view_matrix)
{
	SkyboxUniformBufferObject subo = {};
	subo.inv_view_matrix = glm::mat3(glm::scale(transpose(world_view_matrix), glm::vec3(1.0f, -1.0f, 1.0f)));

	UpdateGpuBuffer( skyboxUniformBuffer, &subo, sizeof( subo ), 0 );
}

static void CmdDrawSkybox(VkCommandBuffer commandBuffer, VkExtent2D extent, size_t currentFrame, const RenderPass * renderpass, const Technique * technique )
{
	CmdBeginVkLabel( commandBuffer, "Skybox Renderpass", glm::vec4( 0.2f, 0.2f, 0.9f, 1.0f ) );
	BeginRenderPass( commandBuffer, *renderpass, renderpass->outputFrameBuffer[currentFrame].frameBuffer, extent );

	vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, technique->pipeline );
	vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, technique->pipelineLayout, 0, 1, &technique->renderPass_descriptor[currentFrame], 0, nullptr );

	vkCmdDraw(commandBuffer, 4, 1, 0, 0);
	EndRenderPass(commandBuffer);
	CmdEndVkLabel(commandBuffer);
}

void SkyboxRecordDrawCommandsBuffer(uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent, const RenderPass * renderpass, const Technique * technique )
{
	CmdDrawSkybox( graphicsCommandBuffer, extent, currentFrame, renderpass, technique );
}