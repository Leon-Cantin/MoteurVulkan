#pragma once

#include "descriptors.h"
#include "scene_instance.h"

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
};

struct GfxMaterial
{	
	Technique techniques[1];
};

//Don't recall this if the technique is the same
void BeginTechnique( VkCommandBuffer commandBuffer, const Technique* technique, size_t currentFrame );
void CmdDrawTechnique( VkCommandBuffer commandBuffer, const Technique* technique, const SceneInstanceSet* instanceSet, const GfxModel* modelAsset, uint32_t currentFrame );
void Destroy( GfxMaterial* material );


enum class eTechniqueDataEntryName
{
	INSTANCE_DATA = 0,
	SHADOW_DATA,
	SCENE_DATA,
	LIGHT_DATA,
	SKYBOX_DATA,
	COUNT
};

enum class eTechniqueDataEntryImageName
{
	ALBEDOS = 0,
	NORMALS,
	SHADOWS,
	TEXT,
	SKYBOX,
	COUNT
};

struct TechniqueDataEntry
{
	eTechniqueDataEntryName name;
	VkDescriptorType descriptorType;
	uint32_t count;
};

struct TechniqueDataEntryImage
{
	eTechniqueDataEntryImageName name;
	VkDescriptorType descriptorType;
	uint32_t count;
};

static const TechniqueDataEntry techniqueDataEntries[static_cast< size_t >(eTechniqueDataEntryName::COUNT)] =
{
	{ eTechniqueDataEntryName::INSTANCE_DATA, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1 },
	{ eTechniqueDataEntryName::SHADOW_DATA,	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
	{ eTechniqueDataEntryName::SCENE_DATA,	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
	{ eTechniqueDataEntryName::LIGHT_DATA,	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
	{ eTechniqueDataEntryName::SKYBOX_DATA,	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
};

static const TechniqueDataEntryImage techniqueDataEntryImages[static_cast< size_t >(eTechniqueDataEntryImageName::COUNT)] =
{
	{ eTechniqueDataEntryImageName::ALBEDOS, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5 },
	{ eTechniqueDataEntryImageName::NORMALS, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
	{ eTechniqueDataEntryImageName::SHADOWS, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
	{ eTechniqueDataEntryImageName::TEXT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
	{ eTechniqueDataEntryImageName::SKYBOX, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
};

struct TechniqueDataBinding
{
	eTechniqueDataEntryName name;
	uint32_t binding;
	VkShaderStageFlags stageFlags;
};

struct TechniqueDataImageBinding
{
	eTechniqueDataEntryImageName name;
	uint32_t binding;
	VkShaderStageFlags stageFlags;
};

struct TechniqueDescriptorSetDesc
{
	TechniqueDataBinding dataBindings [8];
	uint32_t buffersCount;

	TechniqueDataImageBinding dataImageBindings [8];
	uint32_t imagesCount;
};

struct InputBuffers
{
	std::array<VkBuffer* , static_cast< size_t >(eTechniqueDataEntryName::COUNT)> data;
	std::array<VkDescriptorImageInfo*, static_cast< size_t >(eTechniqueDataEntryImageName::COUNT)> dataImages;
};

inline void SetBuffers( InputBuffers* buffers, eTechniqueDataEntryName name, VkBuffer* input )
{
	buffers->data[static_cast< size_t >(name)] = input;
}

inline void SetImages( InputBuffers* buffers, eTechniqueDataEntryImageName name, VkDescriptorImageInfo* input )
{
	buffers->dataImages[static_cast< size_t >(name)] = input;
}

inline VkBuffer* GetBuffer( const InputBuffers* buffers, eTechniqueDataEntryName name )
{
	return buffers->data[static_cast< size_t >(name)];
}

inline VkDescriptorImageInfo* GetImage( const InputBuffers* buffers, eTechniqueDataEntryImageName name )
{
	return buffers->dataImages[static_cast< size_t >(name)];
}

void CreateDescriptorSetLayout( const TechniqueDescriptorSetDesc * desc, VkDescriptorSetLayout * o_setLayout );
void CreateDescriptorSet( const InputBuffers* buffers, const TechniqueDescriptorSetDesc* descriptorSetDesc, VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool, VkDescriptorSet* o_descriptorSet );