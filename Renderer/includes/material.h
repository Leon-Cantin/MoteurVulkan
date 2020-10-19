#pragma once

#include "descriptors.h"
#include "bindings.h"

#include <array>

struct GfxDescriptorSetBinding
{
	std::array< GfxDescriptorTable, SIMULTANEOUS_FRAMES> hw_descriptorSets;
	//TODO: something to manage and generate descriptor sets. Passes can register to use one of many descriptor set layout
	GfxDescriptorTableLayout hw_layout;
	GfxDescriptorTableDesc desc;
	bool isValid = false;
};

struct Technique
{
	std::array< GfxDescriptorSetBinding, 8 > descriptor_sets;

	GfxPipelineLayout pipelineLayout;
	GfxPipeline pipeline;

	VkDescriptorPool parentDescriptorPool;
};

//Don't recall this if the technique is the same
void BeginTechnique( VkCommandBuffer commandBuffer, const Technique* technique, size_t currentFrame );
void Destroy( Technique* technique );