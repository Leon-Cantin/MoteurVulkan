#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "shadersCommon/shadersCommon.h"

layout(set = RENDERPASS_SET, binding = 0) uniform SceneMatrices {
    mat4 view;
    mat4 proj;
} sceneMat;
layout(set = RENDERPASS_SET, binding = 1) uniform Light
{
	mat4 shadowMatrix;
	vec3 location;
	float intensity;
}light;
layout(set = INSTANCE_SET, binding = 0) uniform InstanceMatrices {
    mat4 model;
	uvec4 textureIndices;
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
	vec4 shadowCoord;
}vs_out;

void main() {
	mat4 model_view_matrix = sceneMat.view * instanceMat.model;
	vs_out.viewVector = (model_view_matrix *  vec4(inPosition, 1.0)).xyz;
    gl_Position = sceneMat.proj * vec4(vs_out.viewVector, 1.0);
	vs_out.fragColor = inColor;
	vs_out.fragTexCoord = inTexCoord;
	vs_out.normal_vs = ( model_view_matrix * vec4(inNormal,0.0)).xyz;
	vs_out.tangent_vs = ( model_view_matrix * vec4(inTangent, 0.0)).xyz;
	vs_out.tangent_vs = cross(vs_out.normal_vs, vs_out.tangent_vs);
	vs_out.shadowCoord = (light.shadowMatrix * instanceMat.model) * vec4(inPosition, 1.0);
}