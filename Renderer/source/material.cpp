#include "material.h"

#include "renderpass.h"
#include "vk_debug.h"
#include "vk_buffer.h"
#include "vk_commands.h"
#include "../shaders/shadersCommon.h"

#include <array>

//Don't recall this if the technique is the same
void BeginTechnique( VkCommandBuffer commandBuffer, const Technique* technique, size_t currentFrame )
{
	CmdBindPipeline( commandBuffer, GfxPipelineBindPoint::GRAPHICS, technique->pipeline );
	CmdBindDescriptorTable( commandBuffer, GfxPipelineBindPoint::GRAPHICS, technique->pipelineLayout, RENDERPASS_SET, technique->descriptor_sets[RENDERPASS_SET].hw_descriptorSets[currentFrame] );
}

void Destroy( Technique* technique )
{
	Destroy( &technique->pipeline );
	Destroy( &technique->pipelineLayout );
	for( GfxDescriptorSetBinding& setBinding : technique->descriptor_sets )
	{
		if( setBinding.isValid )
		{
			Destroy( &setBinding.hw_layout );
			Destroy( setBinding.hw_descriptorSets.data(), setBinding.hw_descriptorSets.size(), technique->parentDescriptorPool );
		}
	}
}