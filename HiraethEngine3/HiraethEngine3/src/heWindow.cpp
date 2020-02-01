#include "heWindow.h"
#include <map>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <windowsx.h>
#include "glew/glew.h"
#include "glew/wglew.h"

HINSTANCE classInstance = NULL;
static std::map<HWND, HeWindow*> windowMap;

PFNWGLCREATECONTEXTATTRIBSARBPROC _wglCreateContextAttribsARB = nullptr;
PFNWGLCHOOSEPIXELFORMATARBPROC    _wglChoosePixelFormatARB = nullptr;
PFNWGLSWAPINTERVALEXTPROC         _wglSwapIntervalEXT = nullptr;

LRESULT CALLBACK heWindowCallback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

	HeWindow* window = windowMap[hwnd];
	if (window != nullptr) {

		switch (msg) {

		case WM_CLOSE: {
			window->shouldClose = true;
			break;
		}

		case WM_DESTROY: {
			PostQuitMessage(0);
			break;
		}

		case WM_SIZE: {
			window->windowInfo.size.x = LOWORD(lparam);
			window->windowInfo.size.y = HIWORD(lparam);
			glViewport(0, 0, window->windowInfo.size.x, window->windowInfo.size.y);
			break;
		}

		case WM_MOUSEMOVE: {
			hm::vec2i pos(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
			window->mouseInfo.mousePosition = pos;
			break;
		}

		case WM_LBUTTONDOWN: {
			window->mouseInfo.leftButtonDown = true;
			break;
		}

		case WM_RBUTTONDOWN: {
			window->mouseInfo.rightButtonDown = true;
			break;
		}

		case WM_KEYDOWN: {
			window->keyboardInfo.keyStatus[(HeKeyCode)wparam] = true;
			window->keyboardInfo.keysPressed.emplace_back((HeKeyCode)wparam);
			break;
		};

		case WM_KEYUP: {
			window->keyboardInfo.keyStatus[(HeKeyCode)wparam] = false;
			break;
		};

		default:
			return DefWindowProc(hwnd, msg, wparam, lparam);
		}

	} else
		return DefWindowProc(hwnd, msg, wparam, lparam);

	return 0;

};

bool heCreateWindowHandle(HeWindow* window) {

	DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME | WS_MAXIMIZEBOX;

	window->handle = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		L"Hiraeth2D", // class name
		(LPCWSTR)window->windowInfo.title.c_str(), // window name
		CS_OWNDC | dwStyle,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		window->windowInfo.size.x,
		window->windowInfo.size.y,
		NULL,
		NULL,
		classInstance,
		NULL);

	if (window->handle == NULL) {
		MessageBox(NULL, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
		std::cout << "Error: Could not create window instance: " << GetLastError() << std::endl;
		return false;
	}

	ShowWindow(window->handle, SW_SHOW);
	UpdateWindow(window->handle);
	hm::vec2i size = heCalculateWindowBorderSize(window);
	window->windowInfo.size.x -= size.x;
	window->windowInfo.size.y -= size.y;
	window->shouldClose = false;

	return true;

};

bool heCreateWindowContext(HeWindow* window) {

	if (_wglCreateContextAttribsARB == nullptr)
		heCreateDummyContext();

	/*
	const int pixelAttribs[] = {
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
		WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
		WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB,
		WGL_COLOR_BITS_ARB,     32,
		WGL_ALPHA_BITS_ARB,     8,
		WGL_DEPTH_BITS_ARB,     24,
		WGL_STENCIL_BITS_ARB,   8,
		WGL_SAMPLE_BUFFERS_ARB, 1,
		WGL_SAMPLES_ARB,        0,
		0
	};


	int pixelFormatID;
	UINT numFormats;
	bool status = _wglChoosePixelFormatARB(window->dc, pixelAttribs, NULL, 1, &pixelFormatID, &numFormats);

	if (status == false || numFormats == 0) {
		std::cout << "wglChoosePixelFormatARB() failed." << std::endl;
		return 1;
	}

	PIXELFORMATDESCRIPTOR PFD;
	DescribePixelFormat(window->dc, pixelFormatID, sizeof(PFD), &PFD);
	SetPixelFormat(window->dc, pixelFormatID, &PFD);
	*/

	PIXELFORMATDESCRIPTOR pfd = {
	sizeof(PIXELFORMATDESCRIPTOR),
	1,
	PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
	PFD_TYPE_RGBA,												   // The kind of framebuffer. RGBA or palette.
	32,															   // Colordepth of the framebuffer.
	0, 0, 0, 0, 0, 0,
	0,
	0,
	0,
	0, 0, 0, 0,
	24,															   // Number of bits for the depthbuffer
	8,															   // Number of bits for the stencilbuffer
	0,														       // Number of Aux buffers in the framebuffer.
	PFD_MAIN_PLANE,
	0,
	0, 0, 0
	};

	window->dc = GetDC(window->handle);

	int letWindowsChooseThisPixelFormat;
	letWindowsChooseThisPixelFormat = ChoosePixelFormat(window->dc, &pfd);
	SetPixelFormat(window->dc, letWindowsChooseThisPixelFormat, &pfd);


	int gl33_attribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 3,
		WGL_CONTEXT_PROFILE_MASK_ARB, 0x00000001,
		0
	};

	window->context = _wglCreateContextAttribsARB(window->dc, 0, gl33_attribs);

	wglMakeCurrent(window->dc, window->context);

	int glew = glewInit();
	if (glew != GLEW_OK) {
		std::cout << "Error: Could not setup GLEW (" << glew << ")" << std::endl;
		return false;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	return true;

};

bool heSetupClassInstance() {

	if (classInstance == NULL) {
		WNDCLASSEX wex = { 0 };
		wex.cbSize = sizeof(WNDCLASSEX);
		wex.style = 0;
		wex.cbClsExtra = 0;
		wex.cbWndExtra = 0;
		wex.lpszMenuName = 0;
		wex.lpfnWndProc = heWindowCallback;
		wex.hInstance = classInstance;
		wex.hIcon = LoadIcon(NULL, IDI_WINLOGO);
		wex.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
		wex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wex.lpszClassName = L"Hiraeth2D";
		if (!RegisterClassEx(&wex)) {
			std::cout << "Error: Could not setup windows class: " << GetLastError() << std::endl;
			return false;
		}

		return true;
	};

	return true;

};

void heCreateDummyContext() {

	HWND dummy_window = CreateWindowExA(
		0,
		"Hiraeth2D",
		"Dummy OpenGL Window",
		0,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		0,
		0,
		classInstance,
		0);

	if (!dummy_window) {
		std::cout << "Error: Failed to create dummy OpenGL window." << std::endl;
		return;
	}

	HDC dummy_dc = GetDC(dummy_window);

	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
		PFD_TYPE_RGBA, // The kind of framebuffer. RGBA or palette.
		32,		    // Colordepth of the framebuffer.
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		0,			// Number of bits for the depthbuffer
		0,			 // Number of bits for the stencilbuffer
		0,			 // Number of Aux buffers in the framebuffer.
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	int pixel_format = ChoosePixelFormat(dummy_dc, &pfd);
	if (!pixel_format) {
		//HeLogger::log(HE_LOG_ERROR, "Failed to find a suitable pixel format.");
		std::cout << "Error: Failed to activate dummy OpenGl rendering context" << std::endl;
		return;
	}

	if (!SetPixelFormat(dummy_dc, pixel_format, &pfd)) {
		//HeLogger::log(HE_LOG_ERROR, "Failed to set the pixel format.");
		std::cout << "Error: Failed to activate dummy OpenGl rendering context" << std::endl;
		return;
	}

	HGLRC dummy_context = wglCreateContext(dummy_dc);
	if (!dummy_context) {
		//HeLogger::log(HE_LOG_ERROR, "Failed to create a dummy OpenGL rendering context.");
		std::cout << "Error: Failed to activate dummy OpenGl rendering context" << std::endl;
		return;
	}

	if (!wglMakeCurrent(dummy_dc, dummy_context)) {
		//HeLogger::log(HE_LOG_ERROR, "Failed to activate dummy OpenGL rendering context.");
		std::cout << "Error: Failed to activate dummy OpenGl rendering context" << std::endl;
		return;
	}

	_wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
	_wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");

	wglMakeCurrent(dummy_dc, 0);
	wglDeleteContext(dummy_context);
	ReleaseDC(dummy_window, dummy_dc);
	DestroyWindow(dummy_window);

};

bool heCreateWindow(HeWindow* window) {

	if (!heSetupClassInstance())
		return false;

	if (!heCreateWindowHandle(window))
		return false;

	if (!heCreateWindowContext(window))
		return false;

	if (window->windowInfo.fpsCap == 0)
		heEnableVsync(1);

	windowMap[window->handle] = window;

	return true;

};

void heUpdateWindow(HeWindow* window) {

	window->mouseInfo.leftButtonDown = false;
	window->mouseInfo.rightButtonDown = false;
	window->keyboardInfo.keysPressed.clear();

	MSG msg;
	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	glClearColor(window->windowInfo.backgroundColour.getR(), window->windowInfo.backgroundColour.getG(), window->windowInfo.backgroundColour.getB(), 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

};

void heDestroyWindow(HeWindow* window) {

	HDC context = GetDC(window->handle);
	wglMakeCurrent(context, NULL);
	ReleaseDC(window->handle, context);

	wglDeleteContext(window->context);
	DestroyWindow(window->handle);
	window->handle = NULL;
	windowMap.erase(window->handle);

	if (windowMap.size() == 0)
		UnregisterClass(L"HiraethUI", classInstance);

};

void heSyncToFps(HeWindow* window) {

	auto now = std::chrono::system_clock::now();
	double nowTime = (double)std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
	if (window->lastFrame == 0.)
		window->lastFrame = nowTime;

	// make sure that vsync is not enabled
	if (window->windowInfo.fpsCap > 0) {
		double deltaTime = nowTime - window->lastFrame;

		double requestedTime = 1000. / window->windowInfo.fpsCap;
		double sleepTime = (requestedTime - deltaTime);

		if (sleepTime > 0.) {
			std::chrono::milliseconds duration((int)sleepTime);
			std::this_thread::sleep_for(duration);
		};
	}

	now = std::chrono::system_clock::now();
	nowTime = (double)std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
	window->frameTime = (nowTime - window->lastFrame) / 1000.;
	window->lastFrame = nowTime;

};

void heEnableVsync(unsigned int timestamp) {

	if (!_wglSwapIntervalEXT)
		_wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

	if (_wglSwapIntervalEXT)
		_wglSwapIntervalEXT(1);
	else
		std::cout << "Error: Could not enable vsync" << std::endl;
}

void heSwapWindow(const HeWindow* window) {

	SwapBuffers(window->dc);

};

bool heKeyWasPressed(const HeWindow* window, const HeKeyCode key) {

	return std::find(window->keyboardInfo.keysPressed.begin(), window->keyboardInfo.keysPressed.end(), key)
		!= window->keyboardInfo.keysPressed.end();

};

hm::vec2i heCalculateWindowBorderSize(const HeWindow* window) {

	RECT w, c;
	GetWindowRect(window->handle, &w);
	GetClientRect(window->handle, &c);
	return hm::vec2i((w.right - w.left) - (c.right - c.left), (w.bottom - w.top) - (c.bottom - c.top));

};
