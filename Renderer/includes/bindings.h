#pragma once
#include "gfx_image.h"
#include <array>

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
	//TOOD: add a dirty cache, so when a buffer changes, we update the descriptor sets
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

inline void* GetData( const GpuInputData* buffers, uint32_t id )
{
	assert( id < MAX_DATA_ENTRIES );
	return static_cast<void*>(buffers->data[id].buffer);
}

inline uint32_t GetDataCount( const GpuInputData* buffers, uint32_t id )
{
	assert( id < MAX_DATA_ENTRIES );
	return buffers->dataCount[id];
}

inline const GfxImage* GetDummyImage()
{
	extern GfxImage dummyImage;//defined by the renderer
	assert( IsValid( dummyImage ) );
	return &dummyImage;
}
