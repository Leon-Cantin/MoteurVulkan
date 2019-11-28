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
	vec4 fragColor;
	vec2 fragTexCoord;
}fs_in;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConsts {
	uint index;
} pushConsts;

void main() {
	vec4 albedo = texture(bindlessTextures[instanceMat.textureIndices[0]] , fs_in.fragTexCoord);
	//vec3 albedo = fs_in.fragColor;
	if( albedo.a != 1 )
		discard;

	outColor = albedo;
}