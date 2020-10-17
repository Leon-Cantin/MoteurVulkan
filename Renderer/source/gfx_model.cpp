#include "gfx_model.h"

#include "vk_buffer.h"

#include <unordered_map>
#include <stdexcept>

void DestroyGfxModel(GfxModel& gfxModel)
{
	for( uint8_t i = 0; i < ( VIDataType )eVIDataType::VI_DATA_TYPE_COUNT; ++i )
	{
		if( isValid( gfxModel.vertAttribBuffers[i].buffer ) )
			Destroy( &gfxModel.vertAttribBuffers[i].buffer );
	}
	Destroy( &gfxModel.indexBuffer );
}

static GfxIndexType GetIndexType( uint8_t sizeofIndexType )
{
	switch( sizeofIndexType )
	{
	case 4:
		return GfxIndexType::UINT32;
	case 2:
		return GfxIndexType::UINT16;
	default:
		std::runtime_error( "Unknown index type" );
	}
	return GfxIndexType::UNKNOWN;
}

GfxModelVertexInput* GetVertexInput( GfxModel& gfxModel, eVIDataType dataType )
{
	return &gfxModel.vertAttribBuffers[( VIDataType )dataType];
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
		GfxModelVertexInput* currentVI = GetVertexInput( gfxModel, static_cast<eVIDataType>( viDesc.dataType ) );
		currentVI->desc = viDesc;
		GfxDeviceSize bufferSize = GetBindingSize( &viDesc ) * vertexCount;
		CreateCommitedGpuBuffer( bufferSize, GfxBufferUsageFlagBits::VERTEX_BUFFER | GfxBufferUsageFlagBits::TRANSFER_DST_BUFFER, GfxMemoryPropertyBit::HOST_VISIBLE | GfxMemoryPropertyBit::HOST_COHERENT, &currentVI->buffer );
	}

	GfxDeviceSize bufferSize = indexTypeSize * indiceCount;
	CreateCommitedGpuBuffer( bufferSize, GfxBufferUsageFlagBits::INDEX_BUFFER | GfxBufferUsageFlagBits::TRANSFER_DST_BUFFER, GfxMemoryPropertyBit::HOST_VISIBLE | GfxMemoryPropertyBit::HOST_COHERENT, &gfxModel.indexBuffer );
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
		GfxModelVertexInput* currentVI = GetVertexInput( gfxModel, static_cast< eVIDataType >( viDesc.dataType ) );
		currentVI->desc = viDesc;
		GfxDeviceSize bufferSize = GetBindingSize( &viDesc ) * vertexCount;
		currentVI->buffer.buffer = create_buffer( bufferSize, GfxBufferUsageFlagBits::VERTEX_BUFFER | GfxBufferUsageFlagBits::TRANSFER_DST_BUFFER );
		allocator->Allocate( currentVI->buffer.buffer, &currentVI->buffer.gpuMemory );
		allocator->UploadData( currentVI->buffer, data[i] );
	}

	GfxDeviceSize bufferSize = indexTypeSize * indiceCount;
	gfxModel.indexBuffer.buffer = create_buffer( bufferSize, GfxBufferUsageFlagBits::INDEX_BUFFER | GfxBufferUsageFlagBits::TRANSFER_DST_BUFFER );
	allocator->Allocate( gfxModel.indexBuffer.buffer, &gfxModel.indexBuffer.gpuMemory );
	allocator->UploadData( gfxModel.indexBuffer, indicesData );
	gfxModel.indexType = GetIndexType( indexTypeSize );

	return gfxModel;
}