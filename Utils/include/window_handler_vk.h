#pragma once
#include "window_handler.h"

#include "vk_globals.h"

namespace WH {
	namespace VK{
		R_HW::DisplaySurface create_surface( VkInstance vkInstance, const WindowState& windowState );
		void DestroySurface( R_HW::DisplaySurface* surface, VkInstance vkInstance );
	}
}