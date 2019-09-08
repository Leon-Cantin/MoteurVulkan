#pragma once
#include "window_handler.h"

#include "vk_globals.h"

namespace WH {
	namespace VK{
		extern VkSurfaceKHR _windowSurface;

		void InitializeWindow();
		void ShutdownWindow();
	}
}