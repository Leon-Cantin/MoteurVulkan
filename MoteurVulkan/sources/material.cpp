#include "material.h"

#include "renderpass.h"
#include "vk_debug.h"
#include "vk_buffer.h"
#include "vk_commands.h"
#include "..\shaders\shadersCommon.h"

#include <array>

//Don't recall this if the technique is the same
void BeginTechnique( VkCommandBuffer commandBuffer, const Technique* technique, size_t currentFrame )
{
	vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, technique->pipeline );
	vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, technique->pipelineLayout, RENDERPASS_SET, 1, &technique->renderPass_descriptor[currentFrame], 0, nullptr );
}

void CmdDrawTechnique( VkCommandBuffer commandBuffer, const Technique* technique, const SceneInstanceSet* instanceSet, const GfxModel* modelAsset, uint32_t currentFrame )
{
	/*vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, technique->pipelineLayout, INSTANCE_SET, 1,
		&technique->instance_descriptor[currentFrame], 1, &instanceSet->geometryBufferOffsets[currentFrame] );
	CmdDrawIndexed( commandBuffer, *modelAsset );*/
}

void Destroy( GfxMaterial* material )
{
	for( Technique& technique : material->techniques )
	{
		vkDestroyPipeline( g_vk.device, technique.pipeline, nullptr );
		vkDestroyPipelineLayout( g_vk.device, technique.pipelineLayout, nullptr );
		vkDestroyDescriptorSetLayout( g_vk.device, technique.renderpass_descriptor_layout, nullptr );
		vkDestroyDescriptorSetLayout( g_vk.device, technique.instance_descriptor_layout, nullptr );
	}
}