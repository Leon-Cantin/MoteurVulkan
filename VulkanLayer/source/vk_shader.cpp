#include "vk_globals.h"

namespace R_HW
{
	VkShaderModule create_shader_module( const uint32_t* byte_code, size_t size )
	{
		VkShaderModuleCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.codeSize = size;
		//vector should ensure that we are aligned correctly
		create_info.pCode = byte_code;

		VkShaderModule shader_module;
		if( vkCreateShaderModule( g_gfx.device.device, &create_info, nullptr, &shader_module ) != VK_SUCCESS )
			throw std::runtime_error( "failed to create shader module!" );

		return shader_module;
	}
}