#include "vk_shader.h"

VkShaderModule create_shader_module(char* byte_code, size_t size)
{
	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = size;
	//vector should ensure that we are aligned correctly
	create_info.pCode = reinterpret_cast<const uint32_t*>(byte_code);

	VkShaderModule shader_module;
	if (vkCreateShaderModule(g_vk.device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
		throw std::runtime_error("failed to create shader module!");

	return shader_module;
}
