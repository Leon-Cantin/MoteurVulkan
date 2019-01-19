#pragma once

#include "vk_globals.h"
#include "glm/mat4x4.hpp"
#include "model_asset.h"
#include <glm/gtx/quaternion.hpp>
#include "vk_buffer.h"

struct SceneInstanceSet
{
	std::array<VkDescriptorSet, SIMULTANEOUS_FRAMES> shadowDescriptorSets;
	std::array<uint32_t, SIMULTANEOUS_FRAMES> geometryBufferOffsets;
	PerFrameBuffer instanceUniformBuffer;
};

struct SceneInstance
{
	glm::vec3 location;
	glm::fquat orientation;
	float scale;
};

struct InstanceMatrices {
	glm::mat4 model;
};

struct SceneMatricesUniform {
	glm::mat4 view;
	glm::mat4 proj;
};

glm::mat4 ComputeSceneInstanceModelMatrix(const SceneInstance& sceneInstance);
glm::mat4 ComputeCameraSceneInstanceViewMatrix(const SceneInstance& sceneInstance);
void CreateSceneInstanceDescriptorSet(SceneInstanceSet * o_set, uint32_t hackIndex);