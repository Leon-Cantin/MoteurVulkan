#pragma once

#include "vk_globals.h"
#include <stdexcept>

VkShaderModule create_shader_module(const uint32_t* byte_code, size_t size);
