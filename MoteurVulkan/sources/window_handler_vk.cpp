#include "window_handler_vk.h"

#include <stdexcept>

namespace WH 
{
	namespace VK 
	{
		void create_surface(VkInstance vkInstance, HWND window, HINSTANCE instance, VkSurfaceKHR * windowSurface)
		{
			VkWin32SurfaceCreateInfoKHR surface_create_info = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, nullptr, 0, instance, window };

			if (vkCreateWin32SurfaceKHR(vkInstance, &surface_create_info, nullptr, windowSurface) != VK_SUCCESS)
				throw std::runtime_error("failed to create window surface!");
		}

		bool framebuffer_resized = false;
		static void framebuffer_resize_callback( int width, int height )
		{
			framebuffer_resized = true;
		}

		void InitializeWindow( int windowWidth, int windowHeight, const char * windowName )
		{
			WH::init_window( windowWidth, windowHeight, windowName );
			WH::add_framebuffer_resize_callback( framebuffer_resize_callback );
			WH::VK::create_surface( g_vk.vk_instance, WH::g_window, WH::g_instance, &g_vk.windowSurface );
		}

		void ShutdownWindow()
		{
			WH::terminate();
		}
	}
}