#include "model_asset.h"

#include "vk_buffer.h"

#include <unordered_map>

void DestroyGfxModel(GfxModel& o_modelAsset)
{
	for( uint8_t i = 0; i < ( uint8_t )eVIDataType::VI_DATA_TYPE_COUNT; ++i )
	{
		if( o_modelAsset.vertAttribBuffers[i].vertexAttribBuffer.gpuMemory.memory != VK_NULL_HANDLE )
			DestroyCommitedGpuBuffer( &o_modelAsset.vertAttribBuffers[i].vertexAttribBuffer );
	}
	DestroyCommitedGpuBuffer( &o_modelAsset.indexBuffer );
}

void CreateGfxModel( const std::vector<GfxModelCreationData>& creationData, const std::vector<uint32_t>& indices, GfxModel& o_modelAsset )
{
	o_modelAsset.vertexCount = static_cast< uint32_t >(creationData[0].vertexCount);
	o_modelAsset.indexCount = static_cast< uint32_t >(indices.size());

	for( const GfxModelCreationData& creationDatax : creationData )
	{
		GfxModelVertexInput* currentInput = &o_modelAsset.vertAttribBuffers[( uint8_t )creationDatax.desc.dataType];
		currentInput->desc = creationDatax.desc;
		VkDeviceSize bufferSize = GetBindingSize(&creationDatax.desc) * creationDatax.vertexCount;
		//TODO assert that bufferSize is the same size as the one reported from loading the buffer;
		CreateCommitedGpuBuffer( bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &currentInput->vertexAttribBuffer );
		copyDataToDeviceLocalMemoryImmediate( currentInput->vertexAttribBuffer.buffer, creationDatax.data, bufferSize );
	}

	VkDeviceSize bufferSize = sizeof( uint32_t ) * indices.size();
	CreateCommitedGpuBuffer( bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &o_modelAsset.indexBuffer );
	copyDataToDeviceLocalMemoryImmediate( o_modelAsset.indexBuffer.buffer, indices.data(), bufferSize );
}

void CreateGfxModelNoData( const std::vector<GfxModelCreationData>& creationData, uint32_t indiceCount, GfxModel& o_modelAsset )
{
	o_modelAsset.vertexCount = static_cast< uint32_t >(creationData[0].vertexCount);
	o_modelAsset.indexCount = indiceCount;

	for( const GfxModelCreationData& creationDatax : creationData )
	{
		GfxModelVertexInput* currentInput = &o_modelAsset.vertAttribBuffers[( uint8_t )creationDatax.desc.dataType];
		currentInput->desc = creationDatax.desc;
		VkDeviceSize bufferSize = GetBindingSize( &creationDatax.desc ) * creationDatax.vertexCount;
		//TODO assert that bufferSize is the same size as the one reported from loading the buffer;
		CreateCommitedGpuBuffer( bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &currentInput->vertexAttribBuffer );
	}

	VkDeviceSize bufferSize = sizeof( uint32_t ) * indiceCount;
	CreateCommitedGpuBuffer( bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &o_modelAsset.indexBuffer );
}