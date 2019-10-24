#pragma once
#include "vk_buffer.h"
#include "gfx_image.h"

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

struct GfxDataBinding
{
	uint32_t id;
	uint32_t binding;
	eDescriptorAccess descriptorAccess;
	VkShaderStageFlags stageFlags;
};

struct GfxDescriptorSetDesc
{
	std::vector<GfxDataBinding> dataBindings;
};

constexpr size_t MAX_DATA_ENTRIES = 16;

union GpuInputDataEntry
{
	GpuBuffer* buffer;
	GfxImageSamplerCombined* image;
};

struct GpuInputData
{
	//TODO the ptr here is kinda dangerous. Used to point to an array for arrays of descriptors that contains multiple descriptors... mostly just for images
	std::array<GpuInputDataEntry, MAX_DATA_ENTRIES> data;
	std::array<uint32_t, MAX_DATA_ENTRIES> dataCount;
};

inline void SetBuffers( GpuInputData* buffers, uint32_t id, GpuBuffer* input, uint32_t count )
{
	assert( id < MAX_DATA_ENTRIES );
	buffers->data[id].buffer = input;
	buffers->dataCount[id] = count;
}

inline void SetImages( GpuInputData* buffers, uint32_t id, GfxImageSamplerCombined* input, uint32_t count )
{
	assert( id < MAX_DATA_ENTRIES );
	buffers->data[id].image = input;
	buffers->dataCount[id] = count;
}

inline GpuBuffer* GetBuffer( const GpuInputData* buffers, uint32_t id )
{
	assert( id < MAX_DATA_ENTRIES );
	return buffers->data[id].buffer;
}

inline GfxImageSamplerCombined* GetImage( const GpuInputData* buffers, uint32_t id )
{
	assert( id < MAX_DATA_ENTRIES );
	return buffers->data[id].image;
}

inline uint32_t GetDataCount( const GpuInputData* buffers, uint32_t id )
{
	assert( id < MAX_DATA_ENTRIES );
	return buffers->dataCount[id];
}
