#version 450 core
#include "shadersCommon.h"

layout (set = RENDERPASS_SET, location = 0) out float fragmentdepth;

void main(void)
{
	//fragmentdepth = gl_FragCoord.z;
}