#include "gfx_model.h"

#include <unordered_map>
#include <stdexcept>

void DestroyGfxModel(GfxModel& gfxModel)
{
	for( uint8_t i = 0; i < (R_HW::VIDataType )eVIDataType::VI_DATA_TYPE_COUNT; ++i )
	{
		if( IsValid( gfxModel.vertAttribBuffers[i].buffer ) )
			Destroy( &gfxModel.vertAttribBuffers[i].buffer );
	}
	Destroy( &gfxModel.indexBuffer );
}

static R_HW::GfxIndexType GetIndexType( uint8_t sizeofIndexType )
{
	switch( sizeofIndexType )
	{
	case 4:
		return R_HW::GfxIndexType::UINT32;
	case 2:
		return R_HW::GfxIndexType::UINT16;
	default:
		std::runtime_error( "Unknown index type" );
	}
	return R_HW::GfxIndexType::UNKNOWN;
}

GfxModelVertexInput* GetVertexInput( GfxModel& gfxModel, eVIDataType dataType )
{
	return &gfxModel.vertAttribBuffers[(R_HW::VIDataType )dataType];
}

const GfxModelVertexInput* GetVertexInput( const GfxModel& gfxModel, eVIDataType dataType )
{
	return GetVertexInput( const_cast<GfxModel&>(gfxModel), dataType );
}


GfxModel CreateGfxModel( const std::vector<R_HW::VIDesc>& viDescs, size_t vertexCount, size_t indiceCount, uint8_t indexTypeSize )
{
	GfxModel gfxModel = {};
	gfxModel.vertexCount = vertexCount;
	gfxModel.indexCount = indiceCount;

	for( const R_HW::VIDesc& viDesc : viDescs )
	{
		GfxModelVertexInput* currentVI = GetVertexInput( gfxModel, static_cast<eVIDataType>( viDesc.dataType ) );
		currentVI->desc = viDesc;
		R_HW::GfxDeviceSize bufferSize = R_HW::GetBindingSize( &viDesc ) * vertexCount;
		CreateCommitedGpuBuffer( bufferSize, R_HW::GFX_BUFFER_USAGE_VERTEX_BUFFER_BIT | R_HW::GFX_BUFFER_USAGE_TRANSFER_DST_BIT, R_HW::GFX_MEMORY_PROPERTY_HOST_VISIBLE_BIT | R_HW::GFX_MEMORY_PROPERTY_HOST_COHERENT_BIT, &currentVI->buffer );
	}

	R_HW::GfxDeviceSize bufferSize = indexTypeSize * indiceCount;
	CreateCommitedGpuBuffer( bufferSize, R_HW::GFX_BUFFER_USAGE_INDEX_BUFFER_BIT | R_HW::GFX_BUFFER_USAGE_TRANSFER_DST_BIT, R_HW::GFX_MEMORY_PROPERTY_HOST_VISIBLE_BIT | R_HW::GFX_MEMORY_PROPERTY_HOST_COHERENT_BIT, &gfxModel.indexBuffer );
	gfxModel.indexType = GetIndexType( indexTypeSize );

	return gfxModel;
}

GfxModel CreateGfxModel( const std::vector<R_HW::VIDesc>& viDescs, const std::vector<void*>& data, size_t vertexCount, const void* indicesData, size_t indiceCount, uint8_t indexTypeSize, R_HW::I_BufferAllocator* allocator )
{
	GfxModel gfxModel = {};
	gfxModel.vertexCount = vertexCount;
	gfxModel.indexCount = indiceCount;

	for( uint8_t i = 0; i < viDescs.size(); ++i )
	{
		const R_HW::VIDesc& viDesc = viDescs[i];
		GfxModelVertexInput* currentVI = GetVertexInput( gfxModel, static_cast< eVIDataType >( viDesc.dataType ) );
		currentVI->desc = viDesc;
		R_HW::GfxDeviceSize bufferSize = GetBindingSize( &viDesc ) * vertexCount;
		currentVI->buffer.buffer = R_HW::create_buffer( bufferSize, R_HW::GFX_BUFFER_USAGE_VERTEX_BUFFER_BIT | R_HW::GFX_BUFFER_USAGE_TRANSFER_DST_BIT );
		allocator->Allocate( currentVI->buffer.buffer, &currentVI->buffer.gpuMemory );
		allocator->UploadData( currentVI->buffer, data[i] );
	}

	R_HW::GfxDeviceSize bufferSize = indexTypeSize * indiceCount;
	gfxModel.indexBuffer.buffer = R_HW::create_buffer( bufferSize, R_HW::GFX_BUFFER_USAGE_INDEX_BUFFER_BIT | R_HW::GFX_BUFFER_USAGE_TRANSFER_DST_BIT );
	allocator->Allocate( gfxModel.indexBuffer.buffer, &gfxModel.indexBuffer.gpuMemory );
	allocator->UploadData( gfxModel.indexBuffer, indicesData );
	gfxModel.indexType = GetIndexType( indexTypeSize );

	return gfxModel;
}