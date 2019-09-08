#pragma once

#include<Windows.h>
#undef max
#undef min

#include <cstdint>

namespace WH
{
	extern bool framebuffer_resized;
	extern HWND g_window;
	extern HINSTANCE g_instance;

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
