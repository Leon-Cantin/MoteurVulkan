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

	VkDescriptorPool parentDescriptorPool;
};

struct GfxMaterial
{	
	Technique techniques[1];
};

//Don't recall this if the technique is the same
void BeginTechnique( VkCommandBuffer commandBuffer, const Technique* technique, size_t currentFrame );
void CmdDrawTechnique( VkCommandBuffer commandBuffer, const Technique* technique, const SceneInstanceSet* instanceSet, const GfxModel* modelAsset, uint32_t currentFrame );
void Destroy( GfxMaterial* material );


enum eTechniqueDataEntryFlags
{
	NONE = 1 << 0,
	EXTERNAL = 1 << 1,
};

struct TechniqueDataEntry
{
	uint32_t id;
	VkDescriptorType descriptorType;
	uint32_t count;
	uint32_t flags;
	uint32_t size;
};

struct TechniqueDataEntryImage
{
	uint32_t id;
	VkDescriptorType descriptorType;
	uint32_t count;
};


struct TechniqueDataBinding
{
	uint32_t id;
	uint32_t binding;
	VkShaderStageFlags stageFlags;
};

struct TechniqueDescriptorSetDesc
{
	//TODO: Could be just "dataBindings" instead of 2 seperate arrays if they end up using the same structures. Use an enum to tell which type it is.
	TechniqueDataBinding buffersBindings [8];
	uint32_t buffersCount;

	TechniqueDataBinding imagesBindings [8];
	uint32_t imagesCount;
};

constexpr size_t MAX_DATA_ENTRIES = 16;

struct InputBuffers
{
	//TODO the ptr here is kinda dangerous. Used for descriptors that contains multiple descriptors... mostly just for images
	std::array<GpuBuffer* , MAX_DATA_ENTRIES> data;
	std::array<VkDescriptorImageInfo* , MAX_DATA_ENTRIES> dataImages;
};

inline void SetBuffers( InputBuffers* buffers, uint32_t id, GpuBuffer* input )
{
	assert( id < MAX_DATA_ENTRIES );
	buffers->data[id] = input;
}

inline void SetImages( InputBuffers* buffers, uint32_t id, VkDescriptorImageInfo* input )
{
	assert( id < MAX_DATA_ENTRIES );
	buffers->dataImages[id] = input;
}

inline GpuBuffer* GetBuffer( const InputBuffers* buffers, uint32_t id )
{
	assert( id < MAX_DATA_ENTRIES );
	return buffers->data[id];
}

inline VkDescriptorImageInfo* GetImage( const InputBuffers* buffers, uint32_t id )
{
	assert( id < MAX_DATA_ENTRIES );
	return buffers->dataImages[id];
}

void CreateDescriptorSetLayout( const TechniqueDescriptorSetDesc * desc, VkDescriptorSetLayout * o_setLayout );
void CreateDescriptorSet( const InputBuffers* buffers, const TechniqueDescriptorSetDesc* descriptorSetDesc, VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool, VkDescriptorSet* o_descriptorSet );