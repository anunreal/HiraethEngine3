#ifndef HE_WIN32_LAYER_H
#define HE_WIN32_LAYER_H

#include "heTypes.h"
#include "heWindow.h"

/* NOTE: ALL THESE FUNCTIONS ARE ONLY DEFINED IF HE_USE_WIN32 IS DEFINED */


// -- filesystem

struct HeFileDescriptor {
    std::string name;
    std::string fullPath;
    std::string type;
};


// registers a file monitor and returns the id of that monitor. This only has to be used when the same file has
// multiple references (for example shader headers). When checking for file modification, use the index returned
// as parameter so that every file reference receives the update at some point
extern HE_API uint32_t heWin32RegisterFileMonitor(std::string const& file);
// unregisters a file monitor by setting the entry to be free. Note that this will in fact not free any memory
// (unless this is the last monitor in the list) but it will just mark the file monitor to be reusable for when
// we next register one
extern HE_API void heWin32FreeFileMonitor(std::string const& file, uint32_t const index);
// returns true if the file was modified since the last time it was checked (with this function). If this is
// the first time this file is checked, it is assumed to not have changed. From then on the last access time will
// be stored
extern HE_API b8 heWin32FileModified(std::string const& file, uint32_t const index = 0);
// returns true if the given file exists (relative or absolute path)
extern HE_API b8 heWin32FileExists(std::string const& file);
// adds the (relative) paths of all the files in given folder to the vector. If the given path does not exist or
// is not a folder, the vector will not be modified. If recursive is true, all subfolders will be searched too
extern HE_API void heWin32FolderGetFiles(std::string const& folder, std::vector<HeFileDescriptor>& files, b8 const recursive);
// creates a folder with given relative or absolute path if it doesnt exist
extern HE_API void heWin32FolderCreate(std::string const& path);


// -- window

// creates the windows class with name HiraethEngine3 if it wasnt created before
extern HE_API b8 heWin32SetupClassInstance();
// creates a dummy context needed for retrieving function pointers (context creation with attributes...)
extern HE_API void heWin32CreateDummyContext();

// returns true if the thread that calls this method is the thread that the window was created in
extern HE_API b8 heWin32IsMainThread();
extern HE_API b8 heWin32WindowCreate(HeWindow* window);
extern HE_API void heWin32WindowUpdate(HeWindow* window);
extern HE_API void heWin32WindowDestroy(HeWindow* window);
extern HE_API void heWin32WindowEnableVsync(int8_t const timestamp);
extern HE_API void heWin32WindowSwapBuffers(HeWindow const* window);
extern HE_API void heWin32WindowToggleCursor(b8 const visible);
extern HE_API void heWin32WindowSetCursorPosition(HeWindow* window, hm::vec2f const& position);
extern HE_API hm::vec2i heWin32WindowCalculateBorderSize(HeWindow const* window);
extern HE_API void heWin32WindowToggleFullscreen(HeWindow* window, hm::vec2i const& size, b8 const fullScreen, uint32_t const monitor = 0);


// -- basic timer

// starts a new time entry
extern HE_API void heWin32TimerStart();
// gets the latest time entry (last in first out) in milliseconds
extern HE_API double heWin32TimerGet();
// prints the latest time entry
extern HE_API inline void heWin32TimerPrint(std::string const& id);
// returns a time since program start (with queryPerformanceCounter)
extern HE_API inline __int64 heWin32TimeGet();
// calculates the given duration in cycles into milliseconds. The duration should be calculated using two different
// heWin32TimeGet() calls
extern HE_API inline double heWin32TimeCalculateMs(__int64 duration);


// -- utils

// returns a string from the clipboard (if available) or an empty string if no text is in the clipboard
extern HE_API std::string heWin32ClipboardGet();
// sleeps the active thread for that amount of ms
extern HE_API void heThreadSleep(__int64 ms);

#endif
