#include "vk_loader.h"

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#include <Windows.h>
#elif defined __linux__
#define VK_USE_PLATFORM_XLIB_KHR
#include <dlfcn.h>
#endif 

#undef max
#undef min
#include <vulkan/vulkan.h>
//#include "vk_functions.h"

#include <stdexcept>
#include <iostream>

namespace VKL
{
	/*bool truc_exported()
	{
#define VK_EXPORTED_FUNCTION( fun )                                                   \
    if( !(fun = (PFN_##fun)GetProcAddress( VulkanLibrary, #fun )) ) {                \
      std::cout << "Could not load exported function: " << #fun << "!" << std::endl;  \
      return false;                                                                   \
    }

#include "vk_functions_def.inl"

		return true;
	}

	bool truc_global()
	{
#define VK_GLOBAL_LEVEL_FUNCTION( fun )                                                   \
    if( !(fun = (PFN_##fun)vkGetInstanceProcAddr( nullptr, #fun )) ) {                    \
      std::cout << "Could not load global level function: " << #fun << "!" << std::endl;  \
      return false;                                                                       \
    }

#include "vk_functions_def.inl"

		return true;
	}*/

	void truc()
	{
#ifdef _WIN32
		HMODULE vulkanLibrary = LoadLibrary(L"vulkan-1.dll");
#elif defined __linux__
		void* vulkanLibrary = dlopen("libvulkan.so.1.dll", RTLD_LAZY);
#endif
		if (!vulkanLibrary)
			throw std::runtime_error("Failed to load Vulkan library");

		//truc_exported();
		//truc_global();

		VkApplicationInfo app_info = {};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = "Hello triangle";
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName = "No Engine";
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.apiVersion = VK_API_VERSION_1_1;

		VkInstanceCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo = &app_info;

		VkInstance vk_instance;
		if (vkCreateInstance(&create_info, nullptr, &vk_instance) != VK_SUCCESS)
			throw std::runtime_error("failed to create instance");

	}
}