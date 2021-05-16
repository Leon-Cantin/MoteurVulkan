#include "geometry_renderpass.h"
#include "..\shaders\shadersCommon.h"

#include "file_system.h"
#include "renderer.h"

GpuPipelineLayout GetGeoPipelineLayout()
{
	GpuPipelineLayout pipelineLayout = {};
	pipelineLayout.RootConstantRanges.resize( 1 );

	pipelineLayout.RootConstantRanges[0] = {};
	pipelineLayout.RootConstantRanges[0].offset = 0;
	pipelineLayout.RootConstantRanges[0].count = sizeof( uint32_t );
	pipelineLayout.RootConstantRanges[0].stageFlags = GFX_SHADER_STAGE_FRAGMENT_BIT;
	
	return pipelineLayout;
}

GpuPipelineStateDesc GetGeoPipelineState()
{
	GpuPipelineStateDesc gpuPipelineState = {};
	GetBindingDescription( VIBindingsFullModel, &gpuPipelineState.viState );

	gpuPipelineState.shaders = {
		{ FS::readFile( "shaders/retro_opaque.vert.spv" ), "main", GFX_SHADER_STAGE_VERTEX_BIT },
		{ FS::readFile( "shaders/retro_opaque.frag.spv" ), "main", GFX_SHADER_STAGE_FRAGMENT_BIT } };

	gpuPipelineState.rasterizationState.backFaceCulling = true;
	gpuPipelineState.rasterizationState.depthBiased = false;

	gpuPipelineState.depthStencilState.depthRead = true;
	gpuPipelineState.depthStencilState.depthWrite = true;
	gpuPipelineState.depthStencilState.depthCompareOp = GfxCompareOp::LESS;

	gpuPipelineState.blendEnabled = false;
	gpuPipelineState.primitiveTopology = GfxPrimitiveTopology::TRIANGLE_LIST;

	return gpuPipelineState;
}

void CmdBeginGeometryRenderPass(VkCommandBuffer commandBuffer, uint32_t currentFrame, const RenderPass * renderpass, const Technique * technique)
{
	CmdBeginLabel( commandBuffer, "Geometry renderpass", glm::vec4(0.8f, 0.6f, 0.4f, 1.0f) );

	const FrameBuffer& frameBuffer = renderpass->outputFrameBuffer[currentFrame];
	BeginRenderPass( commandBuffer, *renderpass, frameBuffer );

	BeginTechnique( commandBuffer, technique, currentFrame );
}

void CmdEndGeometryRenderPass(VkCommandBuffer vkCommandBuffer)
{
	EndRenderPass( vkCommandBuffer );
	CmdEndLabel( vkCommandBuffer );
}

static void CmdDrawModelAsset( VkCommandBuffer commandBuffer, const DrawListEntry* drawModel, uint32_t currentFrame, const Technique* technique )
{	
	//TODO: could do like the VIB, query a texture of X from an array using an enum index
	//Have a list of all required paremeters for this pass.
	//vkCmdPushConstants( commandBuffer, technique->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof( uint32_t ), &drawModel->asset->albedoIndex );

	const SceneInstanceSet* instanceSet = &drawModel->descriptorSet;
	const GfxModel* modelAsset = drawModel->asset->modelAsset;
	CmdBindRootDescriptor(commandBuffer, GfxPipelineBindPoint::GRAPHICS, technique->pipelineLayout, INSTANCE_SET,
		technique->descriptor_sets[INSTANCE_SET].hw_descriptorSets[currentFrame], instanceSet->geometryBufferOffsets );

	CmdDrawIndexed(commandBuffer, VIBindingsFullModel, *modelAsset);
}

void GeometryRecordDrawCommandsBuffer( GfxCommandBuffer graphicsCommandBuffer, const FG::TaskInputData& inputData )
{
	const SceneFrameData* frameData = static_cast< const SceneFrameData* >( inputData.userData );
	CmdBeginGeometryRenderPass( graphicsCommandBuffer, inputData.currentFrame, inputData.renderpass, inputData.technique );
	for (size_t i = 0; i < frameData->drawList.size(); ++i)
	{
		const DrawListEntry* drawModel = &frameData->drawList[i];
		CmdDrawModelAsset(graphicsCommandBuffer, drawModel, inputData.currentFrame, inputData.technique);
	}
	CmdEndGeometryRenderPass(graphicsCommandBuffer);
}
