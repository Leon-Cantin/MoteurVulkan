#version 460
#extension GL_ARB_separate_shader_objects : enable
#include "shadersCommon/shadersCommon.h"

layout (set = RENDERPASS_SET, binding = 0) uniform sampler2D srcTexture;

layout (location = 0) in VS_OUT
{
    vec2    tc;
} fs_in;

layout (location = 0) out vec4 color;

void main()
{
    color = vec4(texture(srcTexture, fs_in.tc).xyz, 1.0f);
}
