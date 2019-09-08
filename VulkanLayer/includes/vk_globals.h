#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
//TODO: find a way of getting rid of those undef
#undef max
#undef min

struct Vk_Globals {
	VkInstance vk_instance = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkQueue graphics_queue = VK_NULL_HANDLE;
	VkQueue present_queue = VK_NULL_HANDLE;
	VkQueue compute_queue = VK_NULL_HANDLE;
	VkQueue transfer_queue = VK_NULL_HANDLE;
	VkCommandPool graphicsSingleUseCommandPool = VK_NULL_HANDLE;
	VkCommandPool graphicsCommandPool = VK_NULL_HANDLE;
	VkCommandPool computeCommandPool = VK_NULL_HANDLE;
	VkCommandPool transferCommandPool = VK_NULL_HANDLE;
	VkSurfaceKHR windowSurface = VK_NULL_HANDLE;
};

extern Vk_Globals g_vk;

const int SIMULTANEOUS_FRAMES = 2;