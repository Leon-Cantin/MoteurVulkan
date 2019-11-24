#pragma once

#include "vk_globals.h"
#include "glm/mat4x4.hpp"
#include "model_asset.h"
#include <glm/gtx/quaternion.hpp>
#include "vk_buffer.h"

struct SceneInstanceSet
{
	uint32_t geometryBufferOffsets;
};

struct SceneInstance
{
	glm::vec3 location;
	glm::fquat orientation;
	float scale;
};

struct GfxInstanceData {
	glm::mat4 model;
	uint32_t texturesIndexes[4];
};

struct SceneMatricesUniform {
	glm::mat4 view;
	glm::mat4 proj;
};

struct GfxAsset {
	const GfxModel* modelAsset;
	std::vector<uint32_t> textureIndices;
};

struct GfxAssetInstance
{
	const GfxAsset* asset;
	SceneInstance instanceData;
};

struct DrawListEntry
{
	const GfxAsset* asset;
	SceneInstanceSet descriptorSet;
};

glm::mat4 ComputeSceneInstanceModelMatrix(const SceneInstance& sceneInstance);
glm::mat4 ComputeCameraSceneInstanceViewMatrix(const SceneInstance& sceneInstance);