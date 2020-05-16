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
	vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, technique->pipelineLayout, RENDERPASS_SET, 1, &technique->descriptor_sets[RENDERPASS_SET].hw_descriptorSets[currentFrame], 0, nullptr );
}

void Destroy( Technique* technique )
{
	vkDestroyPipeline( g_vk.device, technique->pipeline, nullptr );
	vkDestroyPipelineLayout( g_vk.device, technique->pipelineLayout, nullptr );
	for( GfxDescriptorSetBinding& setBinding : technique->descriptor_sets )
	{
		if( setBinding.isValid )
		{
			vkDestroyDescriptorSetLayout( g_vk.device, setBinding.hw_layout, nullptr );
			vkFreeDescriptorSets( g_vk.device, technique->parentDescriptorPool, setBinding.hw_descriptorSets.size(), setBinding.hw_descriptorSets.data() );
		}
	}
}