#include "heWindow.h"
#include "heWin32Layer.h"
#include "heGlLayer.h"
#include "heCore.h"
#include <map>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <windowsx.h>
#include "glew/glew.h"
#include "glew/wglew.h"

#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC         ((USHORT) 0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE        ((USHORT) 0x02)
#endif

#define HE_RAW_INPUT 0

HINSTANCE classInstance = NULL;
static std::map<HWND, HeWindow*> windowMap;

PFNWGLCREATECONTEXTATTRIBSARBPROC _wglCreateContextAttribsARB = nullptr;
PFNWGLCHOOSEPIXELFORMATARBPROC    _wglChoosePixelFormatARB = nullptr;
PFNWGLSWAPINTERVALEXTPROC         _wglSwapIntervalEXT = nullptr;


void registerMouseInput(HeWindow* window) {
    const RAWINPUTDEVICE rid = { 0x01, 0x02, 0, window->handle };
    
    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
        HE_ERROR("Win32: Failed to register raw input device");
};

LRESULT CALLBACK heWindowCallback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    
    HeWindow* window = windowMap[hwnd];
    if (window != nullptr) {
        
        switch (msg) {
            
            case WM_SETFOCUS: {
                window->active = true;
                break;
            };
            
            case WM_KILLFOCUS: {
                window->active = false;
                break;
            }
            
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
            
#if HE_RAW_INPUT
            case WM_INPUT: {
                UINT dwSize = 40;
                static BYTE lpb[40];
                
                GetRawInputData((HRAWINPUT)lparam, RID_INPUT,
                                lpb, &dwSize, sizeof(RAWINPUTHEADER));
                
                RAWINPUT* raw = (RAWINPUT*)lpb;
                
                if (raw->header.dwType == RIM_TYPEMOUSE) {
                    //int xPosRelative = raw->data.mouse.lLastX;
                    //int yPosRelative = raw->data.mouse.lLastY;
                    hm::vec2i pos(raw->data.mouse.lLastX, raw->data.mouse.lLastY);
                    window->mouseInfo.deltaMousePosition = pos - window->mouseInfo.mousePosition;
                    //std::cout << "Delta: " << hm::to_string(window->mouseInfo.deltaMousePosition) << std::endl;
                    window->mouseInfo.mousePosition = pos;
                } 
                
                break;
            };
#else
            case WM_MOUSEMOVE: {
                hm::vec2i pos(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
                window->mouseInfo.deltaMousePosition = pos - window->mouseInfo.mousePosition;
                window->mouseInfo.mousePosition = pos;
                break;
            };
            
#endif
            
            
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

bool heWindowCreateHandle(HeWindow* window) {
    
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
        HE_ERROR("Could not create window instance: " + std::to_string(GetLastError()));
        return false;
    }
    
    ShowWindow(window->handle, SW_SHOW);
    UpdateWindow(window->handle);
    hm::vec2i size = heWindowCalculateBorderSize(window);
    window->windowInfo.size.x -= size.x;
    window->windowInfo.size.y -= size.y;
    window->shouldClose = false;
    
    return true;
    
};

bool heWindowCreateContext(HeWindow* window) {
    
    if (_wglCreateContextAttribsARB == nullptr)
        heWin32CreateDummyContext();
    
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
        HE_ERROR("Could not setup GLEW (" + std::to_string(glew) + ")");
        return false;
    }
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    return true;
    
};

bool heWindowCreate(HeWindow* window) {
    
    if (!heWin32SetupClassInstance())
        return false;
    
    if (!heWindowCreateHandle(window))
        return false;
    
    if (!heWindowCreateContext(window))
        return false;
    
    if (window->windowInfo.fpsCap == 0)
        heWindowEnableVsync(1);
    
    registerMouseInput(window);
    windowMap[window->handle] = window;
    window->active = true;
    
    return true;
    
};

void heWindowUpdate(HeWindow* window) {
    
    window->mouseInfo.leftButtonDown = false;
    window->mouseInfo.rightButtonDown = false;
    window->mouseInfo.deltaMousePosition = hm::vec2i(0);
    window->keyboardInfo.keysPressed.clear();
    
    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    heFrameClear(window->windowInfo.backgroundColour, 2);
    
};

void heWindowDestroy(HeWindow* window) {
    
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

void heWindowSyncToFps(HeWindow* window) {
    
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

void heWindowEnableVsync(const int8_t timestamp) {
    
    if (!_wglSwapIntervalEXT)
        _wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    
    if (_wglSwapIntervalEXT)
        _wglSwapIntervalEXT(timestamp);
    else
        HE_ERROR("Could not enable vsync");
}

void heWindowSwapBuffers(const HeWindow* window) {
    
    SwapBuffers(window->dc);
    
};

hm::vec2i heWindowCalculateBorderSize(const HeWindow* window) {
    
    RECT w, c;
    GetWindowRect(window->handle, &w);
    GetClientRect(window->handle, &c);
    return hm::vec2i((w.right - w.left) - (c.right - c.left), (w.bottom - w.top) - (c.bottom - c.top));
    
};

void heWindowToggleCursor(const bool hidden) {
    if (hidden)
        while (ShowCursor(false) >= 0);
    else
        ShowCursor(true);
};


bool heWindowKeyWasPressed(const HeWindow* window, const HeKeyCode key) {
    
    return std::find(window->keyboardInfo.keysPressed.begin(), window->keyboardInfo.keysPressed.end(), key)
        != window->keyboardInfo.keysPressed.end();
    
};

void heWindowSetMousePosition(HeWindow* window, const hm::vec2f& position) {
    
    hm::vec2i abs;
    abs.x = (position.x >= 0) ? (int)position.x : (int)(-position.x * window->windowInfo.size.x);
    abs.y = (position.y >= 0) ? (int)position.y : (int)(-position.y * window->windowInfo.size.y);
    
    POINT p;
    p.x = abs.x;
    p.y = abs.y;
    ClientToScreen(window->handle, &p);
    SetCursorPos(p.x, p.y);
    GetCursorPos(&p);
    ScreenToClient(window->handle, &p);
    window->mouseInfo.mousePosition.x = p.x;
    window->mouseInfo.mousePosition.y = p.y;
    
};

bool heIsMainThread() {
    
    return wglGetCurrentContext() != NULL;
    
};




bool heWin32SetupClassInstance() {
    
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
            HE_ERROR("Could not setup windows class: " + std::to_string(GetLastError()));
            return false;
        }
        
        return true;
    };
    
    return true;
    
};


void heWin32CreateDummyContext() {
    
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
        HE_ERROR("Failed to create dummy OpenGL window");
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
        HE_ERROR("Failed to activate dummy OpenGl rendering context");
        return;
    }
    
    if (!SetPixelFormat(dummy_dc, pixel_format, &pfd)) {
        //HeLogger::log(HE_LOG_ERROR, "Failed to set the pixel format.");
        HE_ERROR("Failed to activate dummy OpenGl rendering context");
        return;
    }
    
    HGLRC dummy_context = wglCreateContext(dummy_dc);
    if (!dummy_context) {
        //HeLogger::log(HE_LOG_ERROR, "Failed to create a dummy OpenGL rendering context.");
        HE_ERROR("Failed to activate dummy OpenGl rendering context");
        return;
    }
    
    if (!wglMakeCurrent(dummy_dc, dummy_context)) {
        //HeLogger::log(HE_LOG_ERROR, "Failed to activate dummy OpenGL rendering context.");
        HE_ERROR("Failed to activate dummy OpenGl rendering context");
        return;
    }
    
    _wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    _wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
    
    wglMakeCurrent(dummy_dc, 0);
    wglDeleteContext(dummy_context);
    ReleaseDC(dummy_window, dummy_dc);
    DestroyWindow(dummy_window);
    
};
