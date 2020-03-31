#include "heWin32Layer.h"
#include "heCore.h"
#include "heGlLayer.h"
#include "glew/glew.h"
#include "glew/wglew.h"
#include <map>
#include <iostream>
#include <windowsx.h>


// stores the last modification time of a file path
typedef std::map<std::string, FILETIME> HeFileTimeMap;
// stores a file handle to any possible file in this map for faster lookups
typedef std::map<std::string, HANDLE>   HeFileHandleMap;

HeFileTimeMap   timeMap;
HeFileHandleMap handleMap;



#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC         ((USHORT) 0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE        ((USHORT) 0x02)
#endif

PFNWGLCREATECONTEXTATTRIBSARBPROC _wglCreateContextAttribsARB = nullptr;
PFNWGLCHOOSEPIXELFORMATARBPROC    _wglChoosePixelFormatARB = nullptr;
PFNWGLSWAPINTERVALEXTPROC         _wglSwapIntervalEXT = nullptr;
HINSTANCE                          classInstance;								
std::map<HWND, HeWindow*>          windowHandleMap;
std::map<HeWindow*, HeWin32Window> win32map;
#define                            HE_RAW_INPUT 0


// --- File System

b8 heFileModified(const std::string& file) {
    
    auto it = handleMap.find(file);
    if(it == handleMap.end()) {
        // file was never requested before, load now
        HANDLE handle = CreateFile(std::wstring(file.begin(), file.end()).c_str(), 
                                   GENERIC_READ, 
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);
        
        if(handle == INVALID_HANDLE_VALUE)
            HE_ERROR("Could not open file handle for time checking (" + file + ")");
        else {
            handleMap[file] = handle;
            FILETIME time;
            GetFileTime(handle, nullptr, nullptr, &time);
            timeMap[file] = time;
        }
        
        return false;
    } else {
        
        FILETIME time;
        GetFileTime(handleMap[file], nullptr, nullptr, &time);
        const FILETIME& last = timeMap[file];
        b8 wasModified = (last.dwLowDateTime != time.dwLowDateTime || last.dwHighDateTime != time.dwHighDateTime);
        timeMap[file] = time;
        return wasModified;
        
    }
    
};


// --- window stuff

HeWin32Window* heWin32GetWindow(const HeWindow* window) {
    
    HeWin32Window* win32 = nullptr;
    
    for(auto& all : win32map) {
        if(all.first == window) {
            win32 = &all.second;
            break;
        }
    }
    
    return win32;
    
};

LRESULT CALLBACK heWin32WindowCallback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    
    HeWindow* window = windowHandleMap[hwnd];
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
                heViewport(hm::vec2i(0), window->windowInfo.size);
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
                    hm::vec2i pos(raw->data.mouse.lLastX, raw->data.mouse.lLastY);
                    window->mouseInfo.deltaMousePosition = pos - window->mouseInfo.mousePosition;
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


void heWin32registerMouseInput(HeWin32Window* window) {
    const RAWINPUTDEVICE rid = { 0x01, 0x02, 0, window->handle };
    
    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
        HE_ERROR("Win32: Failed to register raw input device");
};


b8 heWin32SetupClassInstance() {
    
    if (classInstance == NULL) {
        WNDCLASSEX wex = { 0 };
        wex.cbSize = sizeof(WNDCLASSEX);
        wex.style = 0;
        wex.cbClsExtra = 0;
        wex.cbWndExtra = 0;
        wex.lpszMenuName = 0;
        wex.lpfnWndProc = heWin32WindowCallback;
        wex.hInstance = classInstance;
        wex.hIcon = LoadIcon(NULL, IDI_WINLOGO);
        wex.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
        wex.hCursor = LoadCursor(NULL, IDC_ARROW);
        wex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wex.lpszClassName = L"Hiraeth2D";
        if (!RegisterClassEx(&wex)) {
            HE_ERROR("Win32: Could not setup windows class (" + std::to_string(GetLastError()) + ")");
            return false;
        }
        
        return true;
    };
    
    return true;
    
};

void heWin32CreateDummyContext() {
    
    HWND dummy_window = CreateWindowExA(0, 
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
        HE_ERROR("Win32: Failed to create dummy OpenGL window");
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
        HE_ERROR("Win32: Failed to activate dummy OpenGl rendering context");
        return;
    }
    
    if (!SetPixelFormat(dummy_dc, pixel_format, &pfd)) {
        HE_ERROR("Win32: Failed to activate dummy OpenGl rendering context");
        return;
    }
    
    HGLRC dummy_context = wglCreateContext(dummy_dc);
    if (!dummy_context) {
        //HeLogger::log(HE_LOG_ERROR, "Failed to create a dummy OpenGL rendering context.");
        HE_ERROR("Win32: Failed to activate dummy OpenGl rendering context");
        return;
    }
    
    if (!wglMakeCurrent(dummy_dc, dummy_context)) {
        //HeLogger::log(HE_LOG_ERROR, "Failed to activate dummy OpenGL rendering context.");
        HE_ERROR("Win32: Failed to activate dummy OpenGl rendering context");
        return;
    }
    
    _wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    _wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
    
    wglMakeCurrent(dummy_dc, 0);
    wglDeleteContext(dummy_context);
    ReleaseDC(dummy_window, dummy_dc);
    DestroyWindow(dummy_window);
    
};


b8 heWin32WindowCreateHandle(HeWindow* window, HeWin32Window* win32) {
    
    DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME | WS_MAXIMIZEBOX;
    
    win32->handle = CreateWindowEx(
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
    
    if (win32->handle == NULL) {
        MessageBox(NULL, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        HE_ERROR("Win32: Could not create window instance: " + std::to_string(GetLastError()));
        return false;
    }
    
    ShowWindow(win32->handle, SW_SHOW);
    UpdateWindow(win32->handle);
    hm::vec2i size = heWin32WindowCalculateBorderSize(window);
    window->windowInfo.size.x -= size.x;
    window->windowInfo.size.y -= size.y;
    window->shouldClose = false;
    
    return true;
    
};

b8 heWin32WindowCreateContext(HeWindow* window, HeWin32Window* win32) {
    
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
    
    win32->dc = GetDC(win32->handle);
    
    int letWindowsChooseThisPixelFormat;
    letWindowsChooseThisPixelFormat = ChoosePixelFormat(win32->dc, &pfd);
    SetPixelFormat(win32->dc, letWindowsChooseThisPixelFormat, &pfd);
    
    
    int gl33_attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_PROFILE_MASK_ARB, 0x00000001,
        0
    };
    
    win32->context = _wglCreateContextAttribsARB(win32->dc, 0, gl33_attribs);
    
    wglMakeCurrent(win32->dc, win32->context);
    
    int glew = glewInit();
    if (glew != GLEW_OK) {
        HE_ERROR("Win32: Could not setup GLEW (" + std::to_string(glew) + ")");
        return false;
    }
    
    // TODO(Victor): use gl layer 
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    return true;
    
};


b8 heWin32WindowCreate(HeWindow* window) {
    
    HeWin32Window* win32 = &win32map[window];
    if (!heWin32SetupClassInstance())
        return false;
    
    if (!heWin32WindowCreateHandle(window, win32))
        return false;
    
    if (!heWin32WindowCreateContext(window, win32))
        return false;
    
    if (window->windowInfo.fpsCap == 0)
        heWindowEnableVsync(1);
    
#ifdef HE_RAW_INPUT
    heWin32registerMouseInput(win32);
#endif
    
    windowHandleMap[win32->handle] = window;
    window->active = true;
    
    return true;
    
};

void heWin32WindowUpdate(HeWindow* window) {
    
    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
};

void heWin32WindowDestroy(HeWindow* window) {
    
    HeWin32Window* win32 = &win32map[window];
    HDC context = GetDC(win32->handle);
    wglMakeCurrent(context, NULL);
    ReleaseDC(win32->handle, context);
    
    wglDeleteContext(win32->context);
    DestroyWindow(win32->handle);
    win32->handle = NULL;
    windowHandleMap.erase(win32->handle);
    win32map.erase(window);
    
    if (windowHandleMap.size() == 0)
        UnregisterClass(L"HiraethUI", classInstance);
    
};

void heWin32WindowEnableVsync(const int8_t timestamp) {
    
    if (!_wglSwapIntervalEXT)
        _wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    
    if (_wglSwapIntervalEXT)
        _wglSwapIntervalEXT(timestamp);
    else
        HE_ERROR("Could not enable vsync");
    
};

void heWin32WindowSwapBuffers(const HeWindow* window) {
    
    HeWin32Window* win32 = heWin32GetWindow(window);
    if(win32 != nullptr)
        SwapBuffers(win32->dc);
    
};

void heWin32WindowToggleCursor(const b8 hidden) {
    
    if (hidden)
        while (ShowCursor(false) >= 0);
    else
        ShowCursor(true);
    
};

void heWin32WindowSetCursorPosition(HeWindow* window, const hm::vec2f& position) {
    
    HeWin32Window* win32 = &win32map[window];
    hm::vec2i abs;
    abs.x = (position.x >= 0) ? (int)position.x : (int)(-position.x * window->windowInfo.size.x);
    abs.y = (position.y >= 0) ? (int)position.y : (int)(-position.y * window->windowInfo.size.y);
    
    POINT p;
    p.x = abs.x;
    p.y = abs.y;
    ClientToScreen(win32->handle, &p);
    SetCursorPos(p.x, p.y);
    GetCursorPos(&p);
    ScreenToClient(win32->handle, &p);
    window->mouseInfo.mousePosition.x = p.x;
    window->mouseInfo.mousePosition.y = p.y;
    
};

hm::vec2i heWin32WindowCalculateBorderSize(const HeWindow* window) {
    
    HeWin32Window* win32 = heWin32GetWindow(window);
    if(win32 != nullptr) {
        RECT w, c;
        GetWindowRect(win32->handle, &w);
        GetClientRect(win32->handle, &c);
        return hm::vec2i((w.right - w.left) - (c.right - c.left), (w.bottom - w.top) - (c.bottom - c.top));
    } else
        return hm::vec2i(0);
    
};