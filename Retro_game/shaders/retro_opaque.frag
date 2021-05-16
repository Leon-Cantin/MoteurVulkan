#version 450
#extension GL_ARB_separate_shader_objects : enable
#include "shadersCommon/shadersCommon.h"

#define ALBEDO_TEXTURE_ID 0

layout(set = RENDERPASS_SET, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} sceneMat;
layout(set = RENDERPASS_SET, binding = 1) uniform Light
{	
	mat4 shadowMatrix;
	vec3 location;
	float intensity;
}light;
layout(set = RENDERPASS_SET, binding = 2) uniform texture2D bindlessTextures[BINDLESS_TEXTURES_MAX];
layout(set = RENDERPASS_SET, binding = 3) uniform sampler samplers[SAMPLERS_MAX];
layout(set = RENDERPASS_SET, binding = 4) uniform sampler2DShadow shadowSampler;

layout(set = INSTANCE_SET, binding = 0) uniform InstanceMatrices {
    mat4 model;
	uvec4 textureIndices;
} instanceMat;

layout(location = 0) in VS_OUT
{
	vec3 fragColor;
	vec2 fragTexCoord;
	vec3 normal_vs;
	vec3 tangent_vs;
	vec3 bitangent_vs;
	vec3 viewVector;
	vec4 shadowCoord;
}fs_in;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConsts {
	uint index;
} pushConsts;

vec4 Sample2D( uint textureId, vec2 textureCoordinate, uint samplerId )
{
	return texture( sampler2D( bindlessTextures[instanceMat.textureIndices[textureId]], samplers[samplerId] ), textureCoordinate );
}

void main() 
{	
	vec3 albedo = Sample2D( ALBEDO_TEXTURE_ID, fs_in.fragTexCoord, 0 ).rgb;

	outColor = vec4(albedo, 1.0);
}