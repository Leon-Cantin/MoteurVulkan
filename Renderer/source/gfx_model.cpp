#include "gfx_model.h"

#include "vk_buffer.h"

#include <unordered_map>
#include <stdexcept>

void DestroyGfxModel(GfxModel& gfxModel)
{
	for( uint8_t i = 0; i < ( uint8_t )eVIDataType::VI_DATA_TYPE_COUNT; ++i )
	{
		if( gfxModel.vertAttribBuffers[i].buffer.gpuMemory.memory != VK_NULL_HANDLE )
			DestroyCommitedGpuBuffer( &gfxModel.vertAttribBuffers[i].buffer );
	}
	DestroyCommitedGpuBuffer( &gfxModel.indexBuffer );
}

static VkIndexType GetIndexType( uint8_t sizeofIndexType )
{
	switch( sizeofIndexType )
	{
	case 4:
		return VK_INDEX_TYPE_UINT32;
	case 2:
		return VK_INDEX_TYPE_UINT16;
	default:
		std::runtime_error( "Unknown index type" );
	}
	return VK_INDEX_TYPE_MAX_ENUM;
}

GfxModelVertexInput* GetVertexInput( GfxModel& gfxModel, eVIDataType dataType )
{
	return &gfxModel.vertAttribBuffers[( uint8_t )dataType];
}

const GfxModelVertexInput* GetVertexInput( const GfxModel& gfxModel, eVIDataType dataType )
{
	return GetVertexInput( const_cast<GfxModel&>(gfxModel), dataType );
}


GfxModel CreateGfxModel( const std::vector<VIDesc>& viDescs, size_t vertexCount, size_t indiceCount, uint8_t indexTypeSize )
{
	GfxModel gfxModel = {};
	gfxModel.vertexCount = vertexCount;
	gfxModel.indexCount = indiceCount;

	for( const VIDesc& viDesc : viDescs )
	{
		GfxModelVertexInput* currentVI = GetVertexInput( gfxModel, viDesc.dataType );
		currentVI->desc = viDesc;
		VkDeviceSize bufferSize = GetBindingSize( &viDesc ) * vertexCount;
		CreateCommitedGpuBuffer( bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &currentVI->buffer );
	}

	VkDeviceSize bufferSize = indexTypeSize * indiceCount;
	CreateCommitedGpuBuffer( bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &gfxModel.indexBuffer );
	gfxModel.indexType = GetIndexType( indexTypeSize );

	return gfxModel;
}

GfxModel CreateGfxModel( const std::vector<VIDesc>& viDescs, const std::vector<void*>& data, size_t vertexCount, const void* indicesData, size_t indiceCount, uint8_t indexTypeSize, I_BufferAllocator* allocator )
{
	GfxModel gfxModel = {};
	gfxModel.vertexCount = vertexCount;
	gfxModel.indexCount = indiceCount;

	for( uint8_t i = 0; i < viDescs.size(); ++i )
	{
		const VIDesc& viDesc = viDescs[i];
		GfxModelVertexInput* currentVI = GetVertexInput( gfxModel, viDesc.dataType );
		currentVI->desc = viDesc;
		VkDeviceSize bufferSize = GetBindingSize( &viDesc ) * vertexCount;
		currentVI->buffer.buffer = create_buffer( bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT );
		allocator->Allocate( currentVI->buffer.buffer, &currentVI->buffer.gpuMemory );
		allocator->UploadData( currentVI->buffer, data[i] );
	}

	VkDeviceSize bufferSize = indexTypeSize * indiceCount;
	gfxModel.indexBuffer.buffer = create_buffer( bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT );
	allocator->Allocate( gfxModel.indexBuffer.buffer, &gfxModel.indexBuffer.gpuMemory );
	allocator->UploadData( gfxModel.indexBuffer, indicesData );
	gfxModel.indexType = GetIndexType( indexTypeSize );

	return gfxModel;
}