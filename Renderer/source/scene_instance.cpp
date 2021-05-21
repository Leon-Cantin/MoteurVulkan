#include "scene_instance.h"

#include <glm/gtc/matrix_transform.hpp>
#include "vk_globals.h"

template< class ... T >
glm::mat4 OrderedMatMultiplication( T&& ... args )
{
	return args * ...;
}

static uint32_t instances_count = 0;

glm::mat4 ComputeSceneInstanceModelMatrix(const SceneInstance& sceneInstance)
{
	glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(sceneInstance.scale));
	glm::mat4 rotation = glm::toMat4(sceneInstance.orientation);
	glm::mat4 translation = glm::translate(glm::mat4(1.0f), sceneInstance.location);
	glm::mat4 result = translation * rotation * scale;

	if( sceneInstance.parent )
		result = ComputeSceneInstanceModelMatrix( *sceneInstance.parent ) * result;

	return result;
}

glm::mat4 ComputeCameraSceneInstanceViewMatrix(const SceneInstance& sceneInstance)
{
	//Assume quaternions are of lenght 1 and thus conjugate == inverse
	glm::mat4 result = glm::translate( glm::toMat4( glm::conjugate( sceneInstance.orientation ) ), -sceneInstance.location );

	if( sceneInstance.parent )
		result = glm::translate( result, -sceneInstance.parent->location );

	return result;
}