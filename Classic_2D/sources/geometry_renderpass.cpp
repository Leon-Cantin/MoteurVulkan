#include "geometry_renderpass.h"
#include "../shaders/shadersCommon.h"

#include "file_system.h"
#include "renderer.h"
#include "vk_commands.h"
#include "vk_debug.h"

GpuPipelineLayout GetGeoPipelineLayout()
{
	GpuPipelineLayout pipelineLayout = {};
	pipelineLayout.RootConstantRanges.resize( 1 );

	pipelineLayout.RootConstantRanges[0] = {};
	pipelineLayout.RootConstantRanges[0].offset = 0;
	pipelineLayout.RootConstantRanges[0].count = 1;
	pipelineLayout.RootConstantRanges[0].stageFlags = GFX_SHADER_STAGE_FRAGMENT_BIT;
	
	return pipelineLayout;
}

GpuPipelineStateDesc GetGeoPipelineState()
{
	GpuPipelineStateDesc gpuPipelineState = {};
	GetBindingDescription( VIBindings_PosColUV, &gpuPipelineState.viState );

	gpuPipelineState.shaders = {
		{ FS::readFile( "shaders/triangle.vert.spv" ), "main", VK_SHADER_STAGE_VERTEX_BIT },
		{ FS::readFile( "shaders/triangle.frag.spv" ), "main", VK_SHADER_STAGE_FRAGMENT_BIT } };

	gpuPipelineState.rasterizationState.backFaceCulling = true;
	gpuPipelineState.rasterizationState.depthBiased = false;

	gpuPipelineState.depthStencilState.depthRead = true;
	gpuPipelineState.depthStencilState.depthWrite = true;
	gpuPipelineState.depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;

	gpuPipelineState.blendEnabled = true;
	gpuPipelineState.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	return gpuPipelineState;
}

void CmdBeginGeometryRenderPass(VkCommandBuffer commandBuffer, VkExtent2D extent, uint32_t currentFrame, const RenderPass * renderpass, const Technique * technique)
{
	CmdBeginVkLabel(commandBuffer, "Geometry renderpass", glm::vec4(0.8f, 0.6f, 0.4f, 1.0f));
	const FrameBuffer& frameBuffer = renderpass->outputFrameBuffer[currentFrame];
	BeginRenderPass(commandBuffer, *renderpass, frameBuffer );

	BeginTechnique( commandBuffer, technique, currentFrame );
}

void CmdEndGeometryRenderPass(VkCommandBuffer vkCommandBuffer)
{
	EndRenderPass(vkCommandBuffer);
	CmdEndVkLabel(vkCommandBuffer);
}

static void CmdDrawModelAsset( VkCommandBuffer commandBuffer, const DrawListEntry* drawModel, uint32_t currentFrame, const Technique* technique )
{	
	//TODO: could do like the VIB, query a texture of X from an array using an enum index
	//Have a list of all required paremeters for this pass.
	//vkCmdPushConstants( commandBuffer, technique->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof( uint32_t ), &drawModel->asset->albedoIndex );

	const SceneInstanceSet* instanceSet = &drawModel->descriptorSet;
	const GfxModel* modelAsset = drawModel->asset->modelAsset;
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, technique->pipelineLayout, INSTANCE_SET, 1,
		&technique->descriptor_sets[INSTANCE_SET].hw_descriptorSets[currentFrame], 1, &instanceSet->geometryBufferOffsets);
	CmdDrawIndexed(commandBuffer, VIBindings_PosColUV, *modelAsset);
}

void GeometryRecordDrawCommandsBuffer(uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent, const RenderPass * renderpass, const Technique * technique )
{
	CmdBeginGeometryRenderPass(graphicsCommandBuffer, extent, currentFrame, renderpass, technique);
	for (size_t i = 0; i < frameData->drawList.size(); ++i)
	{
		const DrawListEntry* drawModel = &frameData->drawList[i];
		CmdDrawModelAsset(graphicsCommandBuffer, drawModel, currentFrame, technique);
	}
	CmdEndGeometryRenderPass(graphicsCommandBuffer);
}
