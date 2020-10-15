#pragma once

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined __linux__
#define VK_USE_PLATFORM_XCB_KHR
#endif

#include <vulkan/vulkan.h>
//TODO: find a way of getting rid of those undef
#undef max
#undef min

struct GpuInstance
{
	VkInstance instance;
	bool validationLayerEnabled;
};

typedef VkSurfaceKHR DisplaySurface;

typedef VkPhysicalDevice PhysicalDevice;

struct Queue
{
	VkQueue queue = VK_NULL_HANDLE;
	uint32_t queueFamilyIndex;
};

struct Device
{
	VkDevice device = VK_NULL_HANDLE;
	Queue graphics_queue;
	Queue present_queue;
	Queue compute_queue;
	Queue transfer_queue;
};

struct Vk_Globals {
	GpuInstance instance = {};
	PhysicalDevice physicalDevice = VK_NULL_HANDLE;
	Device device = {};
	VkCommandPool graphicsSingleUseCommandPool = VK_NULL_HANDLE;
	VkCommandPool graphicsCommandPool = VK_NULL_HANDLE;
	VkCommandPool computeCommandPool = VK_NULL_HANDLE;
	VkCommandPool transferCommandPool = VK_NULL_HANDLE;
};

extern Vk_Globals g_vk;

const int SIMULTANEOUS_FRAMES = 2;