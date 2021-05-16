#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out VS_OUT
{
    vec3    tc;
} vs_out;

layout(binding = 0) uniform Uniforms{ 
	mat4 inv_world_view_matrix;
}ubo;

/*vec3 tcs[4] = vec3 [](vec3(0, 0, 1.0),
						   vec3(0, 0, 1.0),
						   vec3(0, 0, 1.0),
						   vec3( 0, 0, 1.0));*/
						   
vec3 vertices[4] = vec3 [](vec3(-1.0, -1.0, 1.0),
						   vec3( 1.0, -1.0, 1.0),							   
						   vec3(-1.0,  1.0, 1.0),                               
						   vec3( 1.0,  1.0, 1.0));
						   
vec3 cubeMapVectors[4] = vec3 [](vec3(-0.5, -0.5, 1.0),
						   vec3( 0.5, -0.5, 1.0),							   
						   vec3(-0.5,  0.5, 1.0),                               
						   vec3( 0.5,  0.5, 1.0));

void main()
{
	vs_out.tc = (ubo.inv_world_view_matrix * vec4(cubeMapVectors[gl_VertexIndex],1.0)).xyz;
	gl_Position = vec4(vertices[gl_VertexIndex], 1.0);
}