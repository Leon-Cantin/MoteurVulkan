#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out VS_OUT
{
    vec2   tc;
} vs_out;

vec2 tcs[4] = vec2 [](vec2(0, 0),
						   vec2(1, 0),
						   vec2(0, 1),
						   vec2( 1, 1));
						   
vec3 vertices[4] = vec3 [](vec3(-1.0, -1.0, 1.0),
						   vec3( 1.0, -1.0, 1.0),							   
						   vec3(-1.0,  1.0, 1.0),                               
						   vec3( 1.0,  1.0, 1.0));

void main()
{
	vs_out.tc = tcs[gl_VertexIndex];
	gl_Position = vec4(vertices[gl_VertexIndex], 1.0);
}