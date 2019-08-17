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

enum eTechniqueDataEntryFlags
{
	NONE = 1 << 0,
	EXTERNAL = 1 << 1,
};

struct TechniqueDataEntry
{
	eTechniqueDataEntryName name;
	VkDescriptorType descriptorType;
	uint32_t count;
	uint32_t flags;
	uint32_t size;
};

struct TechniqueDataEntryImage
{
	eTechniqueDataEntryImageName name;
	VkDescriptorType descriptorType;
	uint32_t count;
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
	//TODO the ptr here is kinda dangerous. Used for descriptors that contains multiple descriptors... mostly just for images
	std::array<GpuBuffer* , static_cast< size_t >(eTechniqueDataEntryName::COUNT)> data;
	std::array<VkDescriptorImageInfo* , static_cast< size_t >(eTechniqueDataEntryImageName::COUNT)> dataImages;
};

inline void SetBuffers( InputBuffers* buffers, eTechniqueDataEntryName name, GpuBuffer* input )
{
	buffers->data[static_cast< size_t >(name)] = input;
}

inline void SetImages( InputBuffers* buffers, eTechniqueDataEntryImageName name, VkDescriptorImageInfo* input )
{
	buffers->dataImages[static_cast< size_t >(name)] = input;
}

inline GpuBuffer* GetBuffer( const InputBuffers* buffers, eTechniqueDataEntryName name )
{
	return buffers->data[static_cast< size_t >(name)];
}

inline VkDescriptorImageInfo* GetImage( const InputBuffers* buffers, eTechniqueDataEntryImageName name )
{
	return buffers->dataImages[static_cast< size_t >(name)];
}

void CreateDescriptorSetLayout( const TechniqueDescriptorSetDesc * desc, VkDescriptorSetLayout * o_setLayout );
void CreateDescriptorSet( const InputBuffers* buffers, const TechniqueDescriptorSetDesc* descriptorSetDesc, VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool, VkDescriptorSet* o_descriptorSet );