#version 450
#extension GL_ARB_separate_shader_objects : enable
#include "shadersCommon/shadersCommon.h"

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
layout(set = RENDERPASS_SET, binding = 2) uniform sampler2D albedoSampler[5];
layout(set = RENDERPASS_SET, binding = 3) uniform sampler2D normalSampler;
layout(set = RENDERPASS_SET, binding = 4) uniform sampler2DShadow shadowSampler;

layout(set = INSTANCE_SET, binding = 0) uniform InstanceMatrices {
    mat4 model;
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

float G1(vec3 n, float alpha, vec3 v)
{
	float k = alpha*sqrt(2/M_PI);
	float dot_nv = dot(n,v);
	return dot_nv/(dot_nv * (1.0 - k) + k);
}

void main() {
float specular_power = 15;
float roughness = 0.65;
	/*float shadow = textureProj(shadowSampler, fs_in.shadowCoord);	

	vec3 light_vs = (sceneMat.view * vec4(light.location,1.0)).xyz;
	vec3 surface_to_light = normalize( light_vs - fs_in.viewVector );
	
	mat3 view_tangent_matrix = mat3(normalize(fs_in.tangent_vs),
							normalize(fs_in.bitangent_vs),
							normalize(fs_in.normal_vs));
	vec3 normal_ts = texture(normalSampler, fs_in.fragTexCoord).rgb;
	vec3 normal_vs = view_tangent_matrix * normal_ts;//vec3 normal_vs = normalize(fs_in.normal_vs);		
	float diffuse_factor = saturate( dot(normal_vs, surface_to_light)) * light.intensity;
	vec3 albedo = texture(albedoSampler , fs_in.fragTexCoord).rgb;
	vec3 diffuse = albedo * (diffuse_factor) * shadow;
	
	vec3 viewVector = normalize(fs_in.viewVector);
	float alpha = pow(roughness,2);
	float NDF = pow(alpha,2)/( 3.1416 * pow(pow(dot(normal_vs,H),2) * (pow(alpha,2) - 1) + 1, 2));
	float GF = saturate(G1(normal_vs, roughness, surface_to_light)) * saturate(G1(normal_vs, roughness, -viewVector));
	float specular = (NDF * GF * fresnel)/(4 * dot(normal_vs, surface_to_light) * dot(normal_vs, -viewVector));
	float specular_factor = pow( saturate( dot(-viewVector, reflect(-surface_to_light, normal_vs))), specular_power);
	vec3 specular_color = vec3(1,1,1);
	
	vec3 specular = specular_color * specular_factor * fresnel * light.intensity * shadow;
	
	outColor = vec4(diffuse*(1.0-gloss)+specular*gloss, 1.0);*/
	
	float shadow = textureProj(shadowSampler, fs_in.shadowCoord);	

	vec3 light_vs = (sceneMat.view * vec4(light.location,1.0)).xyz;
	vec3 surface_to_light = normalize( light_vs - fs_in.viewVector );
	
	mat3 view_tangent_matrix = mat3(normalize(fs_in.tangent_vs),
							normalize(fs_in.bitangent_vs),
							normalize(fs_in.normal_vs));
	vec3 normal_ts = texture(normalSampler, fs_in.fragTexCoord).rgb;
	vec3 normal_vs = view_tangent_matrix * normal_ts;

	float diffuse_factor = saturate( dot(normal_vs, surface_to_light));
	vec3 albedo = texture(albedoSampler[pushConsts.index] , fs_in.fragTexCoord).rgb;
	vec3 diffuse = albedo * (diffuse_factor);
	
	vec3 viewVector = normalize(fs_in.viewVector);
	vec3 H = normalize(-viewVector+surface_to_light);
	float base = 1 - dot(-viewVector,H);
	float exponential = pow( base, 5.0);
	float F0 = 0.4;
	float fresnel = exponential + F0 * (1.0 - exponential);
	
	float alpha = roughness*roughness;
	float NDF = pow(alpha,2)/( M_PI * pow(pow(dot(normal_vs,H),2) * (pow(alpha,2) - 1) + 1, 2));
	
	float k = alpha*sqrt(2/M_PI);
	float NoV = saturate(dot(normal_vs, -viewVector)); //Saturate prevents rays that are not in front
	float NoL = saturate(dot(normal_vs, surface_to_light));
	float G_V = NoV * ( 1 - k ) + k;
	float G_L = NoL * ( 1 - k ) + k;
	float GF = 1 /( G_V * G_L * 4);
	float specular = NDF * fresnel * GF;
	
	vec3 lightColor = vec3(0.96,0.90,0.90) * light.intensity;
	vec3 color = (diffuse * (1.0-fresnel) * lightColor + specular * lightColor) * shadow;	

	outColor = vec4(color, 1.0);

			
/*	float GF = saturate(G1(normal_vs, alpha, surface_to_light)) * saturate(G1(normal_vs, alpha, -viewVector));
	float specular = (NDF * GF * fresnel)/(4 * dot(normal_vs, surface_to_light) * dot(normal_vs, -viewVector));
	
	vec3 lightColor = vec3(0.96,0.90,0.90) * light.intensity;
	vec3 color = (diffuse * (1.0-fresnel) * lightColor + specular * lightColor) * shadow;
	
	//outColor = vec4(color, 1.0);
	
	if(dot(normal_vs,surface_to_light) >= 0 && dot(normal_vs,-viewVector) >= 0 )
		outColor = vec4(color, 1.0);
	else		
		outColor = vec4(0,0,0,1);*/
}