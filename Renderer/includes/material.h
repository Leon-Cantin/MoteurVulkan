#pragma once

#include "vk_globals.h"
#include "bindings.h"

#include <array>

struct GfxDescriptorSetBinding
{
	std::array< R_HW::GfxDescriptorTable, SIMULTANEOUS_FRAMES> hw_descriptorSets;
	//TODO: something to manage and generate descriptor sets. Passes can register to use one of many descriptor set layout
	R_HW::GfxDescriptorTableLayout hw_layout;
	R_HW::GfxDescriptorTableDesc desc;
	bool isValid = false;
};

struct Technique
{
	std::array< GfxDescriptorSetBinding, 8 > descriptor_sets;

	R_HW::GfxPipelineLayout pipelineLayout;
	R_HW::GfxPipeline pipeline;

	VkDescriptorPool parentDescriptorPool;
};

//Don't recall this if the technique is the same
void BeginTechnique( R_HW::GfxCommandBuffer commandBuffer, const Technique* technique, size_t currentFrame );
void Destroy( Technique* technique );