#version 450
#extension GL_ARB_separate_shader_objects : enable
#include "shadersCommon/shadersCommon.h"

layout(location = 0) in VS_OUT
{
	vec3 fragColor;
}fs_in;

layout(location = 0) out vec4 outColor;

void main() 
{	
	outColor = vec4( fs_in.fragColor, 1.0 );
}