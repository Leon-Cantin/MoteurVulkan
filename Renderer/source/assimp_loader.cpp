#include "assimp_loader.h"

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include <vector>

void LoadModel_AssImp( const char * filename, GfxModel& o_modelAsset, size_t hackModelIndex, R_HW::I_BufferAllocator* allocator )
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
	uint32_t indexCount = mesh->mNumFaces * 3;
	indices.resize( indexCount );
	for( size_t i = 0; i < mesh->mNumFaces; ++i )
	{
		indices[i * 3] = mesh->mFaces[i].mIndices[0];
		indices[i * 3 + 1] = mesh->mFaces[i].mIndices[1];
		indices[i * 3 + 2] = mesh->mFaces[i].mIndices[2];
	}

	std::vector<R_HW::VIDesc> modelVIDescs = {
		{ (R_HW::VIDataType )eVIDataType::POSITION, R_HW::eVIDataElementType::FLOAT, 3 },
		{ (R_HW::VIDataType )eVIDataType::NORMAL, R_HW::eVIDataElementType::FLOAT, 3 },
		{ (R_HW::VIDataType )eVIDataType::TANGENT, R_HW::eVIDataElementType::FLOAT, 3 },
		{ (R_HW::VIDataType )eVIDataType::COLOR, R_HW::eVIDataElementType::FLOAT, 3 },
		{ (R_HW::VIDataType )eVIDataType::TEX_COORD, R_HW::eVIDataElementType::FLOAT, 2 },
	};
	std::vector<void*> modelData = {
		vertPos.data(),
		vertNormals.data(),
		vertTangents.data(),
		vertColor.data(),
		vertTexCoord.data(),
	};

	o_modelAsset = CreateGfxModel( modelVIDescs, modelData, mesh->mNumVertices, indices.data(), indexCount, sizeof(uint32_t), allocator );
}