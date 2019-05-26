#pragma once

//TODO: find a way to remove dependence to this (because of the special include of GLFW for vulkan)
#include "vk_globals.h"

namespace WH
{
	void init_window(int windowWidth, int windowHeight, const char * windowName);
	void add_framebuffer_resize_callback(GLFWframebuffersizefun cbfun);
	void create_surface(VkSurfaceKHR * windowSurface);
	void terminate();
}
