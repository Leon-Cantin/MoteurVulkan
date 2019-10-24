#pragma once

#include "descriptors.h"

#include <array>

struct Technique
{
	std::array< VkDescriptorSet, SIMULTANEOUS_FRAMES> renderPass_descriptor;
	std::array< VkDescriptorSet, SIMULTANEOUS_FRAMES> instance_descriptor;

	//TODO: something to manage and generate descriptor sets. Passes can register to use one of many descriptor set layout
	VkDescriptorSetLayout renderpass_descriptor_layout;
	VkDescriptorSetLayout instance_descriptor_layout;

	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

	VkDescriptorPool parentDescriptorPool;
};

struct GfxMaterial
{	
	Technique techniques[1];
};

//Don't recall this if the technique is the same
void BeginTechnique( VkCommandBuffer commandBuffer, const Technique* technique, size_t currentFrame );
void Destroy( GfxMaterial* material );