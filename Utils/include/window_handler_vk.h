#pragma once
#include "window_handler.h"

#include "vk_globals.h"

namespace WH {
	namespace VK{
		DisplaySurface create_surface( VkInstance vkInstance, const WindowState& windowState );
		void DestroySurface( DisplaySurface* surface, VkInstance vkInstance );
	}
}