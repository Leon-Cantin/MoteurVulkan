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

enum eDescriptorType
{
	BUFFER = 0,
	BUFFER_DYNAMIC,
	IMAGE,
	SAMPLER,
	IMAGE_SAMPLER,
};

inline bool IsBufferType( eDescriptorType type )
{
	return type == eDescriptorType::BUFFER || type == eDescriptorType::BUFFER_DYNAMIC;
}

enum eDescriptorAccess
{
	READ = 0,
	WRITE,
};

struct TechniqueDataBinding
{
	uint32_t id;
	uint32_t binding;
	eDescriptorAccess descriptorAccess;
	VkShaderStageFlags stageFlags;
};

struct TechniqueDescriptorSetDesc
{
	std::vector<TechniqueDataBinding> dataBindings;
};

constexpr size_t MAX_DATA_ENTRIES = 16;

union GpuInputDataEntry
{
	GpuBuffer* buffer;
	VkDescriptorImageInfo* image;
};

struct GpuInputData
{
	//TODO the ptr here is kinda dangerous. Used for descriptors that contains multiple descriptors... mostly just for images
	std::array<GpuInputDataEntry, MAX_DATA_ENTRIES> data;
};

inline void SetBuffers( GpuInputData* buffers, uint32_t id, GpuBuffer* input )
{
	assert( id < MAX_DATA_ENTRIES );
	buffers->data[id].buffer = input;
}

inline void SetImages( GpuInputData* buffers, uint32_t id, VkDescriptorImageInfo* input )
{
	assert( id < MAX_DATA_ENTRIES );
	buffers->data[id].image = input;
}

inline GpuBuffer* GetBuffer( const GpuInputData* buffers, uint32_t id )
{
	assert( id < MAX_DATA_ENTRIES );
	return buffers->data[id].buffer;
}

inline VkDescriptorImageInfo* GetImage( const GpuInputData* buffers, uint32_t id )
{
	assert( id < MAX_DATA_ENTRIES );
	return buffers->data[id].image;
}