#include "window_handler.h"

#include <stdexcept>
#include <chrono>

namespace WH
{
	HWND g_window = nullptr;
	HINSTANCE g_instance = nullptr;

	FrameBufferResizeCallback_T framebuffer_resize_callback;
	bool shouldQuit = false;

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
		switch (message) {
		case WM_SIZE:
			if(framebuffer_resize_callback)
				framebuffer_resize_callback(LOWORD(lParam), HIWORD(lParam));
			break;
		case WM_KEYDOWN:
		case WM_CLOSE:
			shouldQuit = true;
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}

	void init_window(int windowWidth, int windowHeight, const char * windowName)
	{
		g_instance = GetModuleHandle(nullptr);
		// Register window class
		WNDCLASSEX wcex;

		wcex.cbSize = sizeof(WNDCLASSEX);

		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = g_instance;
		wcex.hIcon = NULL;
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = L"Truc";
		wcex.hIconSm = NULL;

		if (!RegisterClassEx(&wcex)) {
			throw std::runtime_error("Failed to create window");
		}

		g_window = CreateWindow(L"Truc", L"Truc2", WS_OVERLAPPEDWINDOW, 20, 20, windowWidth, windowHeight, nullptr, nullptr, g_instance, nullptr);
		if (!g_window)
			throw std::runtime_error("Failed to create window");

		ShowWindow( g_window, SW_SHOWNORMAL);
		UpdateWindow( g_window );
	}

	void ProcessMessages()
	{
		MSG message;
		while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
	}
	
	void add_framebuffer_resize_callback(FrameBufferResizeCallback_T cbfun )
	{
		framebuffer_resize_callback = cbfun;
	}


	void terminate()
	{
		DestroyWindow(g_window);
	}

	bool shouldClose()
	{
		return shouldQuit;
	}

	void GetFramebufferSize( uint64_t *width, uint64_t *height )
	{
		RECT rect;
		GetClientRect(g_window, &rect);
		*width = rect.left - rect.right;
		*height = rect.top - rect.bottom;
	}

	size_t GetTime()
	{
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		static std::chrono::system_clock::time_point start = now;
		return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
	}
}