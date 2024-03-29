#pragma once

#if !defined(VULKAN_FUNCTIONS_HEADER)
#define VULKAN_FUNCTIONS_HEADER

#include <vulkan/vulkan.h>

#define VK_EXPORTED_FUNCTION( fun ) extern PFN_##fun fun;
#define VK_GLOBAL_LEVEL_FUNCTION( fun) extern PFN_##fun fun;
#define VK_INSTANCE_LEVEL_FUNCTION( fun ) extern PFN_##fun fun;
#define VK_DEVICE_LEVEL_FUNCTION( fun ) extern PFN_##fun fun;

#include "vk_functions_def.inl"

#endif