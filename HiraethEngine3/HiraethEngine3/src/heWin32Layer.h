#ifndef HE_WIN32_LAYER_H
#define HE_WIN32_LAYER_H

#include "heTypes.h"
#include "heWindow.h"
#include <string>
#include <windows.h>

/* NOTE: ALL THESE FUNCTIONS ARE ONLY DEFINED IF HE_USE_WIN32 IS DEFINED */

struct HeWin32Window {
    // opengl stuff
    HGLRC        context = nullptr;
    HWND         handle = nullptr;
    HDC          dc = nullptr;
};

// returns true if the file was modified since the last time it was checked (with this function). If this is
// the first time this file is checked, it is assumed to not have changed. From then on the last access time will
// be stored
extern HE_API b8 heFileModified(const std::string& file);

// creates the windows class with name Hiraeth2D if it wasnt created before
extern HE_API b8 heWin32SetupClassInstance();
// creates a dummy context needed for retrieving function pointers (context creation with attributes...)
extern HE_API void heWin32CreateDummyContext();

extern HE_API b8 heWin32IsMainThread();
extern HE_API b8 heWin32WindowCreate(HeWindow* window);
extern HE_API void heWin32WindowUpdate(HeWindow* window);
extern HE_API void heWin32WindowDestroy(HeWindow* window);
extern HE_API void heWin32WindowEnableVsync(const int8_t timestamp);
extern HE_API void heWin32WindowSwapBuffers(const HeWindow* window);
extern HE_API void heWin32WindowToggleCursor(const b8 hidden);
extern HE_API void heWin32WindowSetCursorPosition(HeWindow* window, const hm::vec2f& position);
extern HE_API hm::vec2i heWin32WindowCalculateBorderSize(const HeWindow* window);

#endif