#pragma once

#ifdef _WIN32
#include<Windows.h>
#elif defined __linux__
#include <xcb/xcb.h>
#endif // #ifdef _WIN32

#undef max
#undef min

#include <cstdint>

namespace WH
{
	extern bool framebuffer_resized;

#ifdef _WIN32
	extern HWND g_window;
	extern HINSTANCE g_instance;
#elif defined __linux__
	extern xcb_connection_t *xcb_connection;
	extern xcb_screen_t		*screen;
	extern xcb_window_t		window;
#endif // _WIN32

	using FrameBufferResizeCallback_T = void(*)(int width, int height);
	using CharCallback_T = void(*)(uint32_t character);

	void init_window(int windowWidth, int windowHeight, const char * windowName);
	void add_framebuffer_resize_callback(FrameBufferResizeCallback_T cbfun);
	void SetCharCallback(CharCallback_T callback);

	void terminate();
	bool shouldClose();
	void GetFramebufferSize(uint64_t *width, uint64_t *height);
	size_t GetTime();
	void ProcessMessages();

	void InitializeWindow( int windowWidth, int windowHeight, const char * windowName );
	void ShutdownWindow();
}
