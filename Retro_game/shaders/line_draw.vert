#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "shadersCommon/shadersCommon.h"

layout(set = RENDERPASS_SET, binding = 0) uniform SceneMatrices {
    mat4 view;
    mat4 proj;
} sceneMat;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out VS_OUT
{
	vec3 fragColor;
}vs_out;

void main() 
{
    gl_Position = sceneMat.proj * sceneMat.view * vec4(inPosition, 1.0);
	vs_out.fragColor = inColor;
}