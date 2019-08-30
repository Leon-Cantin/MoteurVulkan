#include "geometry_renderpass.h"
#include "..\shaders\shadersCommon.h"

#include "file_system.h"
#include "descriptors.h"
#include "vk_commands.h"
#include "vk_debug.h"

#include <vector>

GpuPipelineLayout GetGeoPipelineLayout()
{
	GpuPipelineLayout pipelineLayout = {};
	pipelineLayout.pushConstantRanges.resize( 1 );

	VkPushConstantRange pushConstantRange = {};
	pipelineLayout.pushConstantRanges[0].offset = 0;
	pipelineLayout.pushConstantRanges[0].size = sizeof( uint32_t );
	pipelineLayout.pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	
	return pipelineLayout;
}

GpuPipelineState GetGeoPipelineState()
{
	GpuPipelineState gpuPipelineState = {};
	GetBindingDescription( &gpuPipelineState.viState );

	gpuPipelineState.shaders = {
		{ FS::readFile( "shaders/triangle.vert.spv" ), "main", VK_SHADER_STAGE_VERTEX_BIT },
		{ FS::readFile( "shaders/triangle.frag.spv" ), "main", VK_SHADER_STAGE_FRAGMENT_BIT } };

	gpuPipelineState.rasterizationState.backFaceCulling = true;
	gpuPipelineState.rasterizationState.depthBiased = false;

	gpuPipelineState.depthStencilState.depthRead = true;
	gpuPipelineState.depthStencilState.depthWrite = true;
	gpuPipelineState.depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;

	gpuPipelineState.blendEnabled = false;
	gpuPipelineState.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	gpuPipelineState.framebufferExtent = { 0,0 }; //swapchain sized

	return gpuPipelineState;
}

void CmdBeginGeometryRenderPass(VkCommandBuffer commandBuffer, VkExtent2D extent, uint32_t currentFrame, const RenderPass * renderpass, const Technique * technique)
{
	CmdBeginVkLabel(commandBuffer, "Geometry renderpass", glm::vec4(0.8f, 0.6f, 0.4f, 1.0f));
	BeginRenderPass(commandBuffer, *renderpass, renderpass->outputFrameBuffer[currentFrame].frameBuffer, extent);

	BeginTechnique( commandBuffer, technique, currentFrame );
}

void CmdEndGeometryRenderPass(VkCommandBuffer vkCommandBuffer)
{
	EndRenderPass(vkCommandBuffer);
	CmdEndVkLabel(vkCommandBuffer);
}

static void CmdDrawModelAsset( VkCommandBuffer commandBuffer, const SceneRenderableAsset* renderableAsset, uint32_t currentFrame, const Technique* technique )
{	
	//TODO: could do like the VIB, query a texture of X from an array using an enum index
	//Have a list of all required paremeters for this pass.
	vkCmdPushConstants( commandBuffer, technique->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof( uint32_t ), &renderableAsset->albedoIndex );

	const SceneInstanceSet* instanceSet = renderableAsset->descriptorSet;
	const GfxModel* modelAsset = renderableAsset->modelAsset;
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, technique->pipelineLayout, INSTANCE_SET, 1,
		&technique->instance_descriptor[currentFrame], 1, &instanceSet->geometryBufferOffsets[currentFrame]);
	CmdDrawIndexed(commandBuffer, *modelAsset);	
}

void GeometryRecordDrawCommandsBuffer(uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent, const RenderPass * renderpass, const Technique * technique )
{
	CmdBeginGeometryRenderPass(graphicsCommandBuffer, extent, currentFrame, renderpass, technique);
	for (size_t i = 0; i < frameData->renderableAssets.size(); ++i)
	{
		const SceneRenderableAsset* renderable = frameData->renderableAssets[i];
		CmdDrawModelAsset(graphicsCommandBuffer, renderable, currentFrame, technique);
	}
	CmdEndGeometryRenderPass(graphicsCommandBuffer);
}
