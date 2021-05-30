#include "material.h"

#include "../shaders/shadersCommon.h"

#include <array>

//Don't recall this if the technique is the same
void BeginTechnique( R_HW::GfxCommandBuffer commandBuffer, const Technique* technique, size_t currentFrame )
{
	R_HW::CmdBindPipeline( commandBuffer, R_HW::GfxPipelineBindPoint::GRAPHICS, technique->pipeline );
	R_HW::CmdBindDescriptorTable( commandBuffer, R_HW::GfxPipelineBindPoint::GRAPHICS, technique->pipelineLayout, RENDERPASS_SET, technique->descriptor_sets[RENDERPASS_SET].hw_descriptorSets[currentFrame] );
}

void Destroy( Technique* technique )
{
	R_HW::Destroy( &technique->pipeline );
	R_HW::Destroy( &technique->pipelineLayout );
	for( GfxDescriptorSetBinding& setBinding : technique->descriptor_sets )
	{
		if( setBinding.isValid )
		{
			R_HW::Destroy( &setBinding.hw_layout );
			R_HW::Destroy( setBinding.hw_descriptorSets.data(), setBinding.hw_descriptorSets.size(), technique->parentDescriptorPool );
		}
	}
}