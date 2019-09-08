#include "scene_instance.h"

#include <glm/gtc/matrix_transform.hpp>
#include "descriptors.h"
#include "vk_buffer.h"

static uint32_t instances_count = 0;

glm::mat4 ComputeSceneInstanceModelMatrix(const SceneInstance& sceneInstance)
{
	glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(sceneInstance.scale));
	glm::mat4 rotation = glm::toMat4(sceneInstance.orientation);
	glm::mat4 translation = glm::translate(glm::mat4(1.0f), sceneInstance.location);
	return translation * rotation * scale;
}

glm::mat4 ComputeCameraSceneInstanceViewMatrix(const SceneInstance& sceneInstance)
{
	//Assume quaternions are of lenght 1 and thus conjugate == inverse
	return glm::translate(glm::toMat4(glm::conjugate(sceneInstance.orientation)), -sceneInstance.location);
}

void CreateSceneInstanceDescriptorSet( SceneInstanceSet * o_set )
{
	const uint32_t index = instances_count++;
	for(size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		o_set->geometryBufferOffsets[i] = sizeof(InstanceMatrices) * index;
}