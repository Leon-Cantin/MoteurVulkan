#pragma once

#include "vk_globals.h"
#include <stdexcept>

VkShaderModule create_shader_module(char* byte_code, size_t size);
