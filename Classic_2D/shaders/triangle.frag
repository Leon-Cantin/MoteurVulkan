#version 450
#extension GL_ARB_separate_shader_objects : enable
#include "shadersCommon/shadersCommon.h"

layout(set = RENDERPASS_SET, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} sceneMat;
layout(set = RENDERPASS_SET, binding = 2) uniform sampler2D bindlessTextures[5];

layout(set = INSTANCE_SET, binding = 0) uniform InstanceMatrices {
    mat4 model;
	uint textureIndices[4];
} instanceMat;

layout(location = 0) in VS_OUT
{
	vec3 fragColor;
	vec2 fragTexCoord;
	vec3 normal_vs;
	vec3 tangent_vs;
	vec3 bitangent_vs;
	vec3 viewVector;
}fs_in;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConsts {
	uint index;
} pushConsts;

void main() {
	vec3 albedo = texture(bindlessTextures[instanceMat.textureIndices[0]] , fs_in.fragTexCoord).rgb;
	
	outColor = vec4(albedo, 1.0);
}