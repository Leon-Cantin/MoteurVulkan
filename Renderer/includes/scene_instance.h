#pragma once

#include "glm/mat4x4.hpp"
#include <glm/gtx/quaternion.hpp>


struct SceneInstance
{
	glm::vec3 location;
	glm::fquat orientation;
	float scale;
};

struct SceneMatricesUniform {
	glm::mat4 view;
	glm::mat4 proj;
};

glm::mat4 ComputeSceneInstanceModelMatrix(const SceneInstance& sceneInstance);
glm::mat4 ComputeCameraSceneInstanceViewMatrix(const SceneInstance& sceneInstance);