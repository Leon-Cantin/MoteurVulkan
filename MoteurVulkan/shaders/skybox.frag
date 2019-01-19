#version 460

layout (binding = 1) uniform samplerCube tex_cubemap;

layout (location = 0) in VS_OUT
{
    vec3    tc;
} fs_in;

layout (location = 0) out vec4 color;

void main()
{
    color = vec4(texture(tex_cubemap, fs_in.tc).xyz, 1.0f);
}
