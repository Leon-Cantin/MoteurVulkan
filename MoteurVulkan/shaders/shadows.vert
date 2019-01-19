#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform SceneMatrices {
    mat4 view;
    mat4 proj;
} sceneMatrices;
layout(binding = 1) uniform InstanceMatrices {
    mat4 model;
} instanceMat;

layout (location = 0) in vec3 position;

void main(void)
{
    gl_Position = sceneMatrices.proj * sceneMatrices.view * instanceMat.model * vec4(position, 1.0f);
}