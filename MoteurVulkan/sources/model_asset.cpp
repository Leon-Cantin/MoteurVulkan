#include "model_asset.h"

#include "vk_buffer.h"

#include <unordered_map>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

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

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

void loadObjModel(const char * filename, std::vector<Vertex>& o_vertices, std::vector<uint32_t>& o_indices)
{	
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename))
		throw std::runtime_error(warn + err);

	std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

	uniqueVertices.reserve(attrib.vertices.size());
	o_vertices.reserve(attrib.vertices.size());
	o_indices.reserve(attrib.vertices.size());

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex = {};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			//Vulkan 0,0 is top-left while OBJ is bottom-left
			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };

			auto ret = uniqueVertices.insert({ vertex, static_cast<uint32_t>(o_vertices.size())});
			if (ret.second)
				o_vertices.push_back(vertex);

			o_indices.push_back(ret.first->second);
		}
	}

	o_vertices.shrink_to_fit();
}

void CreateModelAsset(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, GfxModel& o_modelAsset)
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	createBufferToDeviceLocalMemory(vertices.data(), bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &o_modelAsset.vertexBuffer, &o_modelAsset.vertexBufferMemory);
	o_modelAsset.vertexCount = static_cast<uint32_t>(vertices.size());

	bufferSize = sizeof(indices[0]) * indices.size();
	createBufferToDeviceLocalMemory(indices.data(), bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &o_modelAsset.indexBuffer, &o_modelAsset.indexBufferMemory);
	o_modelAsset.indexCount = static_cast<uint32_t>(indices.size());
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

	std::vector<Vertex> vertices;
	vertices.resize(mesh->mNumVertices);
	for (size_t i = 0; i < mesh->mNumVertices; ++i)
	{
		vertices[i].pos = { mesh->mVertices[i].x, mesh->mVertices[i].y , mesh->mVertices[i].z };
		vertices[i].texCoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
		vertices[i].normal = { mesh->mNormals[i].x, mesh->mNormals[i].y , mesh->mNormals[i].z };
		vertices[i].tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y , mesh->mTangents[i].z };
	}

	std::vector<uint32_t> indices;
	indices.resize(mesh->mNumFaces * 3);
	for (size_t i = 0; i < mesh->mNumFaces; ++i)
	{
		indices[i * 3] = mesh->mFaces[i].mIndices[0];
		indices[i * 3 + 1] = mesh->mFaces[i].mIndices[1];
		indices[i * 3 + 2] = mesh->mFaces[i].mIndices[2];
	}
	
	CreateModelAsset(vertices, indices, o_modelAsset);
}

void DestroyModelAsset(GfxModel& o_modelAsset)
{
	vkDestroyBuffer(g_vk.device, o_modelAsset.vertexBuffer, nullptr);
	vkFreeMemory(g_vk.device, o_modelAsset.vertexBufferMemory, nullptr);
	vkDestroyBuffer(g_vk.device, o_modelAsset.indexBuffer, nullptr);
	vkFreeMemory(g_vk.device, o_modelAsset.indexBufferMemory, nullptr);
}