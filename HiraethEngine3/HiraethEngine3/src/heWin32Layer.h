#ifndef HE_WIN32_LAYER_H
#define HE_WIN32_LAYER_H

#include "heTypes.h"
#include "heWindow.h"
#include <string>
#include <vector>

/* NOTE: ALL THESE FUNCTIONS ARE ONLY DEFINED IF HE_USE_WIN32 IS DEFINED */

// -- filesystem

// returns true if the file was modified since the last time it was checked (with this function). If this is
// the first time this file is checked, it is assumed to not have changed. From then on the last access time will
// be stored
extern HE_API b8 heWin32FileModified(std::string const& file);
// returns true if the given file exists (relative or absolute path)
extern HE_API b8 heWin32FileExists(std::string const& file);
// adds the (relative) paths of all the files in given folder to the vector. If the given path does not exist or is not a folder,
// the vector will not be modified. If recursive is true, all subfolders will be searched too
extern HE_API void heWin32FolderGetFiles(std::string const& folder, std::vector<std::string>& files, b8 const recursive);
extern HE_API void heWin32FolderCreate(std::string const& path);

// -- window

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

// -- basic profiler

// starts a new time entry
extern HE_API void heTimerStart();
// gets the latest time entry (last in first out) in milliseconds
extern HE_API double heTimerGet();
// prints the latest time entry
extern HE_API inline void heTimerPrint(std::string const& id);

#endif