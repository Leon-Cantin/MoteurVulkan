#include "window_handler_vk.h"

#include <stdexcept>

namespace WH 
{
	namespace VK 
	{
#ifdef _WIN32
		DisplaySurface create_surface( VkInstance vkInstance, const WindowState& windowState )
		{
			DisplaySurface windowSurface;
			VkWin32SurfaceCreateInfoKHR surface_create_info = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, nullptr, 0, windowState.instance, windowState.window };

			if (vkCreateWin32SurfaceKHR(vkInstance, &surface_create_info, nullptr, &windowSurface) != VK_SUCCESS)
				throw std::runtime_error("failed to create window surface!");

			return windowSurface;
		}

#elif defined __linux__
		DisplaySurface create_surface( VkInstance vkInstance, const WindowState& windowState )
		{
			VkXcbSurfaceCreateInfoKHR surface_create_info = {VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR, NULL, 0, windowState.connection, windowState.window };
			
			if ( vkCreateXcbSurfaceKHR( windowState.vkInstance, &surface_create_info, nullptr, windowState.windowSurface )  != VK_SUCCESS)
				throw std::runtime_error("failed to create window surface!");
		}
#endif

		void DestroySurface( DisplaySurface* surface, VkInstance vkInstance )
		{
			vkDestroySurfaceKHR( vkInstance, *surface, nullptr );
			*surface = VK_NULL_HANDLE;
		}
	}
}