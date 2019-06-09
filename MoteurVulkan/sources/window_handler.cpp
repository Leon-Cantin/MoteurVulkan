#include "window_handler.h"

#include <stdexcept>

GLFWwindow* g_window;

namespace WH
{
	void init_window(int windowWidth, int windowHeight, const char * windowName)
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		g_window = glfwCreateWindow(windowWidth, windowHeight, windowName, nullptr, nullptr);

		//glfwSetWindowUserPointer(window, this);		
	}
	
	void add_framebuffer_resize_callback( GLFWframebuffersizefun cbfun )
	{
		glfwSetFramebufferSizeCallback(g_window, cbfun);
	}
	
	void create_surface( VkSurfaceKHR * windowSurface ) 
	{
		if (glfwCreateWindowSurface(g_vk.vk_instance, g_window, nullptr, windowSurface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	void terminate()
	{
		glfwDestroyWindow(g_window);
		glfwTerminate();
	}

	bool shouldClose()
	{
		return glfwWindowShouldClose(g_window);
	}
}