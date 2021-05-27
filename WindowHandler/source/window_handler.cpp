#include "window_handler.h"

#include <stdexcept>
#include <chrono>

namespace WH
{
	WindowState g_windowState;

	FrameBufferResizeCallback_T _framebuffer_resize_callback;
	CharCallback_T charCallback;
	bool shouldQuit = false;

	static std::chrono::system_clock::time_point startTime;

#ifdef _WIN32
	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
		switch (message) {
		case WM_SIZE:
			if(_framebuffer_resize_callback)
				_framebuffer_resize_callback(LOWORD(lParam), HIWORD(lParam));
			break;
		case WM_CHAR:
			if (charCallback)
				charCallback(static_cast<uint32_t>(wParam));
			break;
		case WM_CLOSE:
			shouldQuit = true;
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}

	void init_window(int clientAreaWidth, int clientAreaHeight, const char * windowName)
	{
		wchar_t windowNameW[256];
		mbstowcs( windowNameW, windowName, 256 );

		g_windowState.instance = GetModuleHandle(nullptr);
		// Register window class
		WNDCLASSEX wcex;

		wcex.cbSize = sizeof(WNDCLASSEX);

		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = g_windowState.instance;
		wcex.hIcon = NULL;
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = windowNameW;
		wcex.hIconSm = NULL;

		if (!RegisterClassEx(&wcex)) {
			throw std::runtime_error("Failed to create window");
		}

		RECT windowRect = {0, 0, clientAreaWidth, clientAreaHeight};
		AdjustWindowRect( &windowRect, WS_OVERLAPPEDWINDOW, false );

		g_windowState.window = CreateWindow( windowNameW, windowNameW, WS_OVERLAPPEDWINDOW, 20, 20, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, nullptr, nullptr, g_windowState.instance, nullptr);
		if (!g_windowState.window)
			throw std::runtime_error("Failed to create window");

		ShowWindow( g_windowState.window, SW_SHOWNORMAL);
		UpdateWindow( g_windowState.window );

		startTime = std::chrono::system_clock::now();
	}

	void ProcessMessages()
	{
		MSG message;
		while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
	}

	void terminate()
	{
		DestroyWindow( g_windowState.window);
	}

	void GetFramebufferSize( uint64_t *width, uint64_t *height )
	{
		RECT rect;
		GetClientRect( g_windowState.window, &rect);
		*width = rect.left - rect.right;
		*height = rect.top - rect.bottom;
	}
#elif defined __linux__
	//TODO:
	/*LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
		switch (message) {
		case WM_SIZE:
			if(_framebuffer_resize_callback)
				_framebuffer_resize_callback(LOWORD(lParam), HIWORD(lParam));
			break;
		case WM_CHAR:
			if (charCallback)
				charCallback(static_cast<uint32_t>(wParam));
			break;
		case WM_CLOSE:
			shouldQuit = true;
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}*/

	void init_window(int clientAreaWidth, int clientAreaHeight, const char * windowName)
	{
		wchar_t windowNameW[256];
		mbstowcs( windowNameW, windowName, 256 );

		g_windowState.xcb_connection = xcb_connect (NULL, NULL);

		//Get the first screen
		g_windowState.screen = xcb_setup_roots_iterator (xcb_get_setup (xcb_connection)).data;

		g_windowState.window = xcb_generate_id(xcb_connection);

		xcb_create_window( xcb_connection, 
							XCB_COPY_FROM_PARENT, // depth
							g_windowState.window,
							g_windowState.screen->root,
							0, 0,
							clientAreaWidth, clientAreaHeight,
							10,
							XCB_WINDOW_CLASS_INPUT_OUTPUT,
							g_windowState.screen->root_visual,
							0, nullptr);

		xcb_map_window( g_windowState.xcb_connection, g_windowState.window );

		xcb_flush( g_windowState.xcb_connection );

		if( !g_windowState.window )
			throw std::runtime_error("Failed to create window");

		startTime = std::chrono::system_clock::now();
	}

	void ProcessMessages()
	{
		/*MSG message;
		while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&message);
			DispatchMessage(&message);
		}*/
	}

	void terminate()
	{
		xcb_disconnect( g_windowState.xcb_connection );
	}

	void GetFramebufferSize( uint64_t *width, uint64_t *height )
	{
		//TODO: Linux
		*width = 800;
		*height = 600;
	}
#endif // _WIN32
	
	void add_framebuffer_resize_callback(FrameBufferResizeCallback_T cbfun )
	{
		_framebuffer_resize_callback = cbfun;
	}

	void SetCharCallback(CharCallback_T callback)
	{
		charCallback = callback;
	}

	bool shouldClose()
	{
		return shouldQuit;
	}

	size_t GetTime()
	{
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		return std::chrono::duration_cast< std::chrono::milliseconds >(now - startTime).count();
	}

	bool framebuffer_resized = false;
	static void framebuffer_resize_callback( int width, int height )
	{
		framebuffer_resized = true;
	}

	void InitializeWindow( int windowWidth, int windowHeight, const char * windowName )
	{
		init_window( windowWidth, windowHeight, windowName );
		add_framebuffer_resize_callback( framebuffer_resize_callback );
	}

	void ShutdownWindow()
	{
		terminate();
	}
}