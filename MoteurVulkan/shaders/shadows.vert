#version 450
#extension GL_ARB_separate_shader_objects : enable
#include "shadersCommon/shadersCommon.h"

layout(set = RENDERPASS_SET, binding = 0) uniform SceneMatrices {
    mat4 view;
    mat4 proj;
} sceneMatrices;
layout(set = INSTANCE_SET, binding = 0) uniform InstanceMatrices {
    mat4 model;
	uint textureIndices[4];
} instanceMat;

layout (location = 0) in vec3 position;

void main(void)
{
    gl_Position = sceneMatrices.proj * sceneMatrices.view * instanceMat.model * vec4(position, 1.0f);
}