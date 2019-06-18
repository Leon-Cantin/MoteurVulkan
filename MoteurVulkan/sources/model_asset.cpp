#include "model_asset.h"

#include "vk_buffer.h"

#include <unordered_map>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

//#define TINYOBJLOADER_IMPLEMENTATION
//#include <tiny_obj_loader.h>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

/*
const std::vector<Vertex> static_cube_vertices = {
//Top
{ { -0.5f, 0.5f, -0.5f },{ 0.5f, 0.0f, 0.0f },{ 0.0f, 0.0f } },
{ { -0.5f, 0.5f, 0.5f },{ 0.5f, 0.5f, 0.5f },{ 0.0f, 1.0f } },
{ { 0.5f, 0.5f, 0.5f },{ 0.0f, 0.0f, 0.5f},{ 1.0f, 1.0f } },
{ { 0.5f, 0.5f, -0.5f },{ 0.0f, 0.5f, 0.0f },{ 1.0f, 0.0f } },

//Bottom
{ { -0.5f, -0.5f, -0.5f },{ 0.5f, 0.0f, 0.0f },{ 0.0f, 1.0f } },
{ { -0.5f, -0.5f, 0.5f },{ 0.5f, 0.5f, 0.5f},{ 0.0f, 0.0f } },
{ { 0.5f, -0.5f, 0.5f },{ 0.0f, 0.0f, 0.5f},{ 1.0f, 0.0f } },
{ { 0.5f, -0.5f, -0.5f },{ 0.0f, 0.5f, 0.0f },{ 1.0f, 1.0f } }
};*/

const std::vector<uint32_t> static_cube_indices = {
	0, 1, 2, 2, 3, 0,   1, 5, 2, 5, 6, 2,   3, 7, 0, 7, 4, 0,
	6, 5, 4, 4, 7, 6,   2, 6, 3, 6, 7, 3,   0, 4, 1, 4, 5, 1
};

void CreateGfxModel( const std::vector<glm::vec3>& vertPos,
	const std::vector<glm::vec3>& vertNormal,
	const std::vector<glm::vec3>& vertTangent,
	const std::vector<glm::vec3>& vertColor,
	const std::vector<glm::vec2>& vertTexCoord,
	const std::vector<uint32_t>& indices,
	GfxModel& o_modelAsset )
{
	//VkDeviceSize vertexMemorySize = ( sizeof( glm::vec3 ) * 4 + sizeof( glm::vec2 ) ) * vertPos.size();
	//VkDeviceSize indicesMemorySize = sizeof( uint32_t ) * indices.size();

	o_modelAsset.vertexCount = static_cast< uint32_t >(vertPos.size());
	o_modelAsset.indexCount = static_cast< uint32_t >(indices.size());

	VkDeviceSize bufferSize = sizeof( glm::vec3 ) * vertPos.size();
	createBufferToDeviceLocalMemory( vertPos.data(), bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &o_modelAsset.vertPosBuffer, &o_modelAsset.vertPosMem);
	
	bufferSize = sizeof( glm::vec3 ) * vertNormal.size();
	createBufferToDeviceLocalMemory( vertNormal.data(), bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &o_modelAsset.vertNormalBuffer, &o_modelAsset.vertNormalMem );

	bufferSize = sizeof( glm::vec3 ) * vertTangent.size();
	createBufferToDeviceLocalMemory( vertTangent.data(), bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &o_modelAsset.vertTangentBuffer, &o_modelAsset.vertTangentMem );

	bufferSize = sizeof( glm::vec3 ) * vertColor.size();
	createBufferToDeviceLocalMemory( vertColor.data(), bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &o_modelAsset.vertColorBuffer, &o_modelAsset.vertColorMem );

	bufferSize = sizeof( glm::vec2 ) * vertTexCoord.size();
	createBufferToDeviceLocalMemory( vertTexCoord.data(), bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &o_modelAsset.vertTexCoordBuffer, &o_modelAsset.vertTexCoordMem );

	bufferSize = sizeof( uint32_t ) * indices.size();
	createBufferToDeviceLocalMemory(indices.data(), bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &o_modelAsset.indexBuffer, &o_modelAsset.indicesMemory );

}

void LoadGenericModel(const char * filename, GfxModel& o_modelAsset, size_t hackModelIndex)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filename,
		aiProcess_FlipUVs |
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_MakeLeftHanded |
		aiProcess_SortByPType);

	//TODO merge meshes or something
	aiMesh* mesh = scene->mMeshes[scene->mRootNode->mChildren[0]->mMeshes[hackModelIndex]];

	std::vector<glm::vec3> vertPos, vertNormals, vertTangents, vertColor;
	std::vector<glm::vec2> vertTexCoord;
	vertPos.resize( mesh->mNumVertices );
	vertNormals.resize( mesh->mNumVertices );
	vertTangents.resize( mesh->mNumVertices );
	vertColor.resize( mesh->mNumVertices );
	vertTexCoord.resize( mesh->mNumVertices );

	for (size_t i = 0; i < mesh->mNumVertices; ++i)
	{
		vertPos[i] = { mesh->mVertices[i].x, mesh->mVertices[i].y , mesh->mVertices[i].z };
		vertTexCoord[i] = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
		vertNormals[i] = { mesh->mNormals[i].x, mesh->mNormals[i].y , mesh->mNormals[i].z };
		vertTangents[i] = { mesh->mTangents[i].x, mesh->mTangents[i].y , mesh->mTangents[i].z };
	}

	std::vector<uint32_t> indices;
	indices.resize(mesh->mNumFaces * 3);
	for (size_t i = 0; i < mesh->mNumFaces; ++i)
	{
		indices[i * 3] = mesh->mFaces[i].mIndices[0];
		indices[i * 3 + 1] = mesh->mFaces[i].mIndices[1];
		indices[i * 3 + 2] = mesh->mFaces[i].mIndices[2];
	}
	
	CreateGfxModel(vertPos, vertNormals, vertTangents, vertColor, vertTexCoord, indices, o_modelAsset);
}

void DestroyGfxModel(GfxModel& o_modelAsset)
{
	vkDestroyBuffer(g_vk.device, o_modelAsset.vertPosBuffer, nullptr);
	vkFreeMemory(g_vk.device, o_modelAsset.vertPosMem, nullptr);

	vkDestroyBuffer( g_vk.device, o_modelAsset.vertNormalBuffer, nullptr );
	vkFreeMemory( g_vk.device, o_modelAsset.vertNormalMem, nullptr );

	vkDestroyBuffer( g_vk.device, o_modelAsset.vertTangentBuffer, nullptr );
	vkFreeMemory( g_vk.device, o_modelAsset.vertTangentMem, nullptr );

	vkDestroyBuffer( g_vk.device, o_modelAsset.vertColorBuffer, nullptr );
	vkFreeMemory( g_vk.device, o_modelAsset.vertColorMem, nullptr );

	vkDestroyBuffer( g_vk.device, o_modelAsset.vertTexCoordBuffer, nullptr );
	vkFreeMemory( g_vk.device, o_modelAsset.vertTexCoordMem, nullptr );

	vkDestroyBuffer(g_vk.device, o_modelAsset.indexBuffer, nullptr);
	vkFreeMemory(g_vk.device, o_modelAsset.indicesMemory, nullptr);
}