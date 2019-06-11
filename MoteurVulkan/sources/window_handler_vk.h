#pragma once

#include "vk_globals.h"

namespace WH {
	namespace VK{
		void create_surface(VkInstance vkInstance, HWND window, HINSTANCE instance, VkSurfaceKHR * windowSurface);
}
}