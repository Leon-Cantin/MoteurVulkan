#include "window_handler_vk.h"

#include <stdexcept>

namespace WH 
{
	namespace VK 
	{
		VkSurfaceKHR _windowSurface;

		static void create_surface(VkInstance vkInstance, HWND window, HINSTANCE instance, VkSurfaceKHR * windowSurface)
		{
			VkWin32SurfaceCreateInfoKHR surface_create_info = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, nullptr, 0, instance, window };

			if (vkCreateWin32SurfaceKHR(vkInstance, &surface_create_info, nullptr, windowSurface) != VK_SUCCESS)
				throw std::runtime_error("failed to create window surface!");
		}

		void InitializeWindow()
		{
			create_surface( g_vk.vk_instance, WH::g_window, WH::g_instance, &_windowSurface );
		}

		void ShutdownWindow()
		{
			vkDestroySurfaceKHR( g_vk.vk_instance, _windowSurface, nullptr );
		}
	}
}