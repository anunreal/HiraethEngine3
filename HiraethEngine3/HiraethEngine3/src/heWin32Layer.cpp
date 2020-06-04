#include "heWin32Layer.h"
#include "heCore.h"
#include "heGlLayer.h"
#include "heUtils.h"
#include "glew/glew.h"
#include "glew/wglew.h"
#include "heAssets.h"
#include <map>
#include <iostream>
#include <filesystem>

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>

struct HeTimer {
    static const uint8_t maxScopes   = 10;
    uint8_t currentScope             = 0;
    LARGE_INTEGER frequency          = { 0 };
    LARGE_INTEGER entries[maxScopes] = { 0 };
} heTimer;

struct HeWin32Window {
    // opengl stuff
    HGLRC context = nullptr;
    HWND handle   = nullptr;
    HDC dc        = nullptr;
};


// stores the last modification time of a file path
typedef std::map<std::string, FILETIME> HeFileTimeMap;
// stores a file handle to any possible file in this map for faster lookups
typedef std::map<std::string, HANDLE>   HeFileHandleMap;

HeFileTimeMap   timeMap;
HeFileHandleMap handleMap;


#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC ((USHORT) 0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE ((USHORT) 0x02)
#endif
#define HE_RAW_INPUT 0

PFNWGLCREATECONTEXTATTRIBSARBPROC  _wglCreateContextAttribsARB = nullptr;
PFNWGLCHOOSEPIXELFORMATARBPROC     _wglChoosePixelFormatARB = nullptr;
PFNWGLSWAPINTERVALEXTPROC          _wglSwapIntervalEXT = nullptr;
HINSTANCE                          classInstance;

#ifdef HE_ENABLE_MULTIPLE_WINDOWS
std::map<HWND, HeWindow*>          windowHandleMap;
std::map<HeWindow*, HeWin32Window> win32map;
#else
HeWindow*     heWindow = nullptr;
HeWin32Window heWin32Window;
#endif
// --- File System

b8 heWin32FileModified(const std::string& file) {
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

b8 heWin32FileExists(std::string const& file) {
    DWORD dwAttrib = GetFileAttributes((LPCWSTR) file.c_str());
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
};

void heWin32FolderGetFiles(std::string const& folder, std::vector<HeFileDescriptor>& files, b8 const recursive) {
    namespace fs = std::filesystem;
    for(const auto& all : fs::directory_iterator(folder)) {
        if(all.is_directory()) {
            if(recursive)
                heWin32FolderGetFiles(all.path().string(), files, true);
        } else {
            std::string path = all.path().string();
            path = heStringReplaceAll(path, '\\', '/');

            HeFileDescriptor* file = &files.emplace_back();
            size_t dot      = path.find_last_of('.');
            size_t lastDash = path.find_last_of('/');
            file->fullPath  = path;
            file->type      = path.substr(dot + 1);
            file->name      = path.substr(lastDash + 1, dot);
        }
    }
};

void heWin32FolderCreate(std::string const& folder) {
    size_t index = 0;
    while(index != std::string::npos) {
        size_t next = folder.find('/', index + 1);
        CreateDirectoryA(folder.substr(0, next).c_str(), NULL);
        index = next;
    }
};

// --- window stuff

HeWin32Window* heWin32GetWindow(HeWindow const* window) {
#ifdef HE_ENABLE_MULTIPLE_WINDOWS
    HeWin32Window* win32 = nullptr;
    
    for(auto& all : win32map) {
        if(all.first == window) {
            win32 = &all.second;
            break;
        }
    }
    
    return win32;
#else
    return &heWin32Window;
#endif
};

HeKeyCode heWin32GetKeyCode(WPARAM wparam, LPARAM lparam) {
    // check if this is a special key with left and right (shift, control...)
    WPARAM newVk = wparam;
    const UINT scancode = (lparam & 0x00ff0000) >> 16;
    const int extended = (lparam & 0x01000000) != 0;
    
    switch (wparam) {
    case VK_SHIFT:
        newVk = MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
        break;
    case VK_CONTROL:
        newVk = extended ? VK_RCONTROL : VK_LCONTROL;
        break;
    case VK_MENU:
        newVk = extended ? VK_RMENU : VK_LMENU;
        break;
    default:
        // not a key we map from generic to left/right specialized, just return it.
        newVk = wparam;
        break;
    }
    
    return (HeKeyCode) newVk;
};

LRESULT CALLBACK heWin32WindowCallback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    HeWindow* window;
#ifdef HE_ENABLE_MULTIPLE_WINDOWS
    window = windowHandleMap[hwnd];
#else
    window = heWindow;
#endif
    
    if (window != nullptr) {        
        switch (msg) {          
        case WM_SETFOCUS: {
            window->active = true;
            break;
        };
            
        case WM_KILLFOCUS: {
            window->active = false;
            break;
        };
            
        case WM_CLOSE: {
            window->shouldClose = true;
            break;
        };
            
        case WM_DESTROY: {
            PostQuitMessage(0);
            break;
        };
            
        case WM_SIZE: {
            window->windowInfo.size.x = LOWORD(lparam);
            window->windowInfo.size.y = HIWORD(lparam);
            heViewport(hm::vec2i(0), window->windowInfo.size);
            window->resized = true;
            break;
        };
            
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

        case WM_MOUSEWHEEL: {
            int8_t delta = GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA; // windows delta is in multiples of WHEEL_DELTA
            
            for(auto all : window->mouseInfo.scrollCallbacks)
                if(all(window, delta, window->mouseInfo.mousePosition))
                    break;
            
            break;
        };
            
        case WM_LBUTTONDOWN: {
            window->mouseInfo.leftButtonDown = true;
            window->mouseInfo.leftButtonPressed = true;
            break;
        };
            
        case WM_RBUTTONDOWN: {
            window->mouseInfo.rightButtonDown = true;
            window->mouseInfo.rightButtonPressed = true;
            break;
        };

        case WM_LBUTTONUP: {
            window->mouseInfo.leftButtonDown = false;
            break;
        };

        case WM_RBUTTONUP: {
            window->mouseInfo.rightButtonDown = false;
            break;
        };
            
        case WM_KEYDOWN: {
            HeKeyCode key = heWin32GetKeyCode(wparam, lparam);
            window->keyboardInfo.keyStatus[key] = true;
            window->keyboardInfo.keysPressed.emplace_back(key);

            for(auto all : window->keyboardInfo.keyPressCallbacks)
                if(all(window, key))
                    break;

            break;
        };
            
        case WM_KEYUP: {
            window->keyboardInfo.keyStatus[heWin32GetKeyCode(wparam, lparam)] = false;
            break;
        };

        case WM_CHAR: {
            uint32_t unicode = (uint32_t) wparam;
                
            for(auto all : window->keyboardInfo.textInputCallbacks)
                if(all(window, unicode))
                    break;
                
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
                                 32,            // Colordepth of the framebuffer.
                                 0, 0, 0, 0, 0, 0,
                                 0,
                                 0,
                                 0,
                                 0, 0, 0, 0,
                                 0,         // Number of bits for the depthbuffer
                                 0,          // Number of bits for the stencilbuffer
                                 0,          // Number of Aux buffers in the framebuffer.
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
                                 PFD_TYPE_RGBA,                                                // The kind of framebuffer. RGBA or palette.
                                 32,                                                           // Colordepth of the framebuffer.
                                 0, 0, 0, 0, 0, 0,
                                 0,
                                 0,
                                 0,
                                 0, 0, 0, 0,
                                 24,                                                           // Number of bits for the depthbuffer
                                 8,                                                        // Number of bits for the stencilbuffer
                                 0,                                                            // Number of Aux buffers in the framebuffer.
                                 PFD_MAIN_PLANE,
                                 0,
                                 0, 0, 0
    };
    
    win32->dc = GetDC(win32->handle);
    
    int letWindowsChooseThisPixelFormat;
    letWindowsChooseThisPixelFormat = ChoosePixelFormat(win32->dc, &pfd);
    SetPixelFormat(win32->dc, letWindowsChooseThisPixelFormat, &pfd);
    
    
    int gl33_attribs[] = {
                          WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
                          WGL_CONTEXT_MINOR_VERSION_ARB, 2,
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
    
    heBlendMode(0);
    return true;
};


b8 heWin32IsMainThread() {
    return wglGetCurrentContext() != NULL;
};

b8 heWin32WindowCreate(HeWindow* window) {
    HeWin32Window* win32;
    
#ifdef HE_ENABLE_MULTIPLE_WINDOWS
    win32 = &win32map[window];
    windowHandleMap[win32->handle] = window;
#else
    win32 = &heWin32Window;
    heWindow = window;
#endif
    
    if (!heWin32SetupClassInstance())
        return false;
    
    if (!heWin32WindowCreateHandle(window, win32))
        return false;
    
    if (!heWin32WindowCreateContext(window, win32))
        return false;
    
    if (window->windowInfo.fpsCap == 0)
        heWindowEnableVsync(1);
    else
        heWindowEnableVsync(0);
    
#ifdef HE_RAW_INPUT
    heWin32registerMouseInput(win32);
#endif

    // estimate context size
    uint32_t colourBuffers = hm::ceilPowerOfTwo(window->windowInfo.size.x * window->windowInfo.size.y * 4) * 2;
    uint32_t depthBuffers  = hm::ceilPowerOfTwo(window->windowInfo.size.x * window->windowInfo.size.y * 3) * 2;
    uint32_t stencilBuffer = hm::ceilPowerOfTwo(window->windowInfo.size.x * window->windowInfo.size.y) * 2;
    uint32_t size = colourBuffers + depthBuffers + stencilBuffer;
    heMemoryTracker[HE_MEMORY_TYPE_CONTEXT] = size;
    window->active = true;
    heViewport(hm::vec2i(0), window->windowInfo.size);
    return true;
};

void heWin32WindowUpdate(HeWindow* window) {
    window->resized = false;
    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if(window->active && window->mouseInfo.cursorLock != hm::vec2f(0.f)) {
		POINT currentMousePosP = {};
		::GetCursorPos(&currentMousePosP);
		hm::vec2i current(currentMousePosP.x, currentMousePosP.y);

        hm::vec2i lock;
        if(window->mouseInfo.cursorLock.x > 0)
            lock.x = (int32_t) window->mouseInfo.cursorLock.x;
        else
            lock.x = (int32_t) (-window->mouseInfo.cursorLock.x * window->windowInfo.size.x);

        if(window->mouseInfo.cursorLock.y > 0)
            lock.y = (int32_t) window->mouseInfo.cursorLock.y;
        else
            lock.y = (int32_t) (-window->mouseInfo.cursorLock.y * window->windowInfo.size.y);
                
		heWindowSetCursorPosition(window, lock);

        ::GetCursorPos(&currentMousePosP);
		hm::vec2i newPos(currentMousePosP.x, currentMousePosP.y);

        window->mouseInfo.deltaMousePosition = current - newPos;
        window->mouseInfo.mousePosition = newPos;
    }
};

void heWin32WindowDestroy(HeWindow* window) {
    HeWin32Window* win32 = heWin32GetWindow(window);
    HDC context = GetDC(win32->handle);
    wglMakeCurrent(context, NULL);
    ReleaseDC(win32->handle, context);
    
    wglDeleteContext(win32->context);
    DestroyWindow(win32->handle);
    win32->handle = NULL;
    
#ifdef HE_ENABLE_MULTIPLE_WINDOWS
    windowHandleMap.erase(win32->handle);
    win32map.erase(window);
    if (windowHandleMap.size() == 0)
        UnregisterClass(L"HiraethUI", classInstance);
#else
    heWindow = nullptr;
    heWin32Window = HeWin32Window();
    UnregisterClass(L"HiraethUI", classInstance);
#endif
};

void heWin32WindowEnableVsync(int8_t const timestamp) {
    if (!_wglSwapIntervalEXT)
        _wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    
    if (_wglSwapIntervalEXT)
        _wglSwapIntervalEXT(timestamp);
    else
        HE_ERROR("Could not enable vsync");
};

void heWin32WindowSwapBuffers(HeWindow const* window) {
    HeWin32Window* win32 = heWin32GetWindow(window);
    if(win32 != nullptr)
        SwapBuffers(win32->dc);
};

void heWin32WindowToggleCursor(b8 const hidden) {
    if (hidden)
        while (ShowCursor(false) >= 0);
    else
        ShowCursor(true);
};

void heWin32WindowSetCursorPosition(HeWindow* window, hm::vec2f const& position) {
    HeWin32Window* win32 = heWin32GetWindow(window);
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

hm::vec2i heWin32WindowCalculateBorderSize(HeWindow const* window) {
    HeWin32Window* win32 = heWin32GetWindow(window);
    if(win32 != nullptr) {
        RECT w, c;
        GetWindowRect(win32->handle, &w);
        GetClientRect(win32->handle, &c);
        return hm::vec2i((w.right - w.left) - (c.right - c.left), (w.bottom - w.top) - (c.bottom - c.top));
    } else
        return hm::vec2i(0);
};


void heWin32TimerStart() {
    if(heTimer.frequency.QuadPart == 0)
        QueryPerformanceFrequency(&heTimer.frequency);
    
    QueryPerformanceCounter(&heTimer.entries[heTimer.currentScope]);
    heTimer.currentScope++;
};

double heWin32TimerGet() {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    heTimer.currentScope--;
    return (now.QuadPart - heTimer.entries[heTimer.currentScope].QuadPart) * 1000.0 / heTimer.frequency.QuadPart;
};

void heWin32TimerPrint(std::string const& id) {
    heLogCout(id + ": " + std::to_string(heWin32TimerGet()) + "ms", "[TIMER]:");
};

__int64 heWin32TimeGet() {
    LARGE_INTEGER _int;
    QueryPerformanceCounter(&_int);
    return _int.QuadPart;
};

double heWin32TimeCalculateMs(__int64 duration) {
    return duration * 1000.0 / heTimer.frequency.QuadPart;
};


std::string heWin32ClipboardGet() {
    if(!OpenClipboard(NULL))
        return ""; // could not open clipboard for some reason
    
    if(!IsClipboardFormatAvailable(CF_TEXT))
        return ""; // no text in the clipboard

    HANDLE glb   = GetClipboardData(CF_TEXT);
    CloseClipboard();
    char* s = (char*) glb;
    return std::string(s);
};
