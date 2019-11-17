#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "shadersCommon/shadersCommon.h"

layout(set = RENDERPASS_SET, binding = 0) uniform SceneMatrices {
    mat4 view;
    mat4 proj;
} sceneMat;
layout(set = INSTANCE_SET, binding = 0) uniform InstanceMatrices {
    mat4 model;
	vec4 textureIndices;
} instanceMat;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;

layout(location = 0) out VS_OUT
{
	vec3 fragColor;
	vec2 fragTexCoord;
	vec3 normal_vs;
	vec3 tangent_vs;
	vec3 bitangent_vs;
	vec3 viewVector;
}vs_out;

void main() {
    gl_Position = sceneMat.proj * instanceMat.model * vec4(inPosition, 1.0);

	vs_out.fragColor = inColor;
	vs_out.fragTexCoord = inTexCoord;
}