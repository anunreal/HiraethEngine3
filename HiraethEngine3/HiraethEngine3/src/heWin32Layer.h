#ifndef HE_WIN32_LAYER_H
#define HE_WIN32_LAYER_H

#include "heTypes.h"
#include "heWindow.h"
#include <string>

/* NOTE: ALL THESE FUNCTIONS ARE ONLY DEFINED IF HE_USE_WIN32 IS DEFINED */

// returns true if the file was modified since the last time it was checked (with this function). If this is
// the first time this file is checked, it is assumed to not have changed. From then on the last access time will
// be stored
extern HE_API b8 heWin32FileModified(std::string const& file);
extern HE_API b8 heWin32FileExists(std::string const& file);

// creates the windows class with name Hiraeth2D if it wasnt created before
extern HE_API b8 heWin32SetupClassInstance();
// creates a dummy context needed for retrieving function pointers (context creation with attributes...)
extern HE_API void heWin32CreateDummyContext();

extern HE_API b8 heWin32IsMainThread();
extern HE_API b8 heWin32WindowCreate(HeWindow* window);
extern HE_API void heWin32WindowUpdate(HeWindow* window);
extern HE_API void heWin32WindowDestroy(HeWindow* window);
extern HE_API void heWin32WindowEnableVsync(int8_t const timestamp);
extern HE_API void heWin32WindowSwapBuffers(HeWindow const* window);
extern HE_API void heWin32WindowToggleCursor(b8 const hidden);
extern HE_API void heWin32WindowSetCursorPosition(HeWindow* window, hm::vec2f const& position);
extern HE_API hm::vec2i heWin32WindowCalculateBorderSize(HeWindow const* window);

#endif