#pragma once
#include "window_handler.h"

#include "vk_globals.h"

namespace WH {
	namespace VK{
		extern bool framebuffer_resized;

		void create_surface(VkInstance vkInstance, HWND window, HINSTANCE instance, VkSurfaceKHR * windowSurface);
		void InitializeWindow( int windowWidth, int windowHeight, const char * windowName );
		void ShutdownWindow();
	}
}