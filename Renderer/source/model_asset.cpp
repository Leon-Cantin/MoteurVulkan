#include "model_asset.h"

#include "vk_buffer.h"

#include <unordered_map>
#include <glm/gtx/hash.hpp>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

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

void LoadGenericModel( const char * filename, GfxModel& o_modelAsset, size_t hackModelIndex )
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile( filename,
		aiProcess_FlipUVs |
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_MakeLeftHanded |
		aiProcess_SortByPType );

	//TODO merge meshes or something
	aiMesh* mesh = scene->mMeshes[scene->mRootNode->mChildren[0]->mMeshes[hackModelIndex]];

	std::vector<glm::vec3> vertPos, vertNormals, vertTangents, vertColor;
	std::vector<glm::vec2> vertTexCoord;
	vertPos.resize( mesh->mNumVertices );
	vertNormals.resize( mesh->mNumVertices );
	vertTangents.resize( mesh->mNumVertices );
	vertColor.resize( mesh->mNumVertices );
	vertTexCoord.resize( mesh->mNumVertices );

	for( size_t i = 0; i < mesh->mNumVertices; ++i )
	{
		vertPos[i] = { mesh->mVertices[i].x, mesh->mVertices[i].y , mesh->mVertices[i].z };
		vertTexCoord[i] = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
		vertNormals[i] = { mesh->mNormals[i].x, mesh->mNormals[i].y , mesh->mNormals[i].z };
		vertTangents[i] = { mesh->mTangents[i].x, mesh->mTangents[i].y , mesh->mTangents[i].z };
	}

	std::vector<uint32_t> indices;
	indices.resize( mesh->mNumFaces * 3 );
	for( size_t i = 0; i < mesh->mNumFaces; ++i )
	{
		indices[i * 3] = mesh->mFaces[i].mIndices[0];
		indices[i * 3 + 1] = mesh->mFaces[i].mIndices[1];
		indices[i * 3 + 2] = mesh->mFaces[i].mIndices[2];
	}

	std::vector<GfxModelCreationData> modelCreationData;
	modelCreationData.resize(5);
	modelCreationData[0].data = reinterpret_cast<uint8_t*>(vertPos.data());
	modelCreationData[0].vertexCount = vertPos.size();
	modelCreationData[0].desc = { eVIDataType::POSITION, eVIDataElementType::FLOAT, 3 };

	modelCreationData[1].data = reinterpret_cast< uint8_t* >(vertNormals.data());
	modelCreationData[1].vertexCount = vertNormals.size();
	modelCreationData[1].desc = { eVIDataType::NORMAL, eVIDataElementType::FLOAT, 3 };

	modelCreationData[2].data = reinterpret_cast< uint8_t* >(vertTangents.data());
	modelCreationData[2].vertexCount = vertTangents.size();
	modelCreationData[2].desc = { eVIDataType::TANGENT, eVIDataElementType::FLOAT, 3 };

	modelCreationData[3].data = reinterpret_cast< uint8_t* >(vertColor.data());
	modelCreationData[3].vertexCount = vertColor.size();
	modelCreationData[3].desc = { eVIDataType::COLOR, eVIDataElementType::FLOAT, 3 };

	modelCreationData[4].data = reinterpret_cast< uint8_t* >(vertTexCoord.data());
	modelCreationData[4].vertexCount = vertTexCoord.size();
	modelCreationData[4].desc = { eVIDataType::TEX_COORD, eVIDataElementType::FLOAT, 2 };

	CreateGfxModel( modelCreationData, indices, o_modelAsset );
}