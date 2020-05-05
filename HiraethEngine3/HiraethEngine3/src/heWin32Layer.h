#ifndef HE_WIN32_LAYER_H
#define HE_WIN32_LAYER_H

#include "heTypes.h"
#include "heWindow.h"
#include <string>
#include <vector>

/* NOTE: ALL THESE FUNCTIONS ARE ONLY DEFINED IF HE_USE_WIN32 IS DEFINED */

// This struct is used for reading plain ascii files.
struct HeWin32TextFile {
	std::string   name;
	std::string   fullPath;
	uint32_t      lineNumber = 0;
	uint32_t      bufferSize = 0;
	
	b8            open   = false;
	char*         buffer = nullptr;
	std::ifstream stream;

	uint32_t version = 0;
};


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
// creates a folder with given relative or absolute path if it doesnt exist
extern HE_API void heWin32FolderCreate(std::string const& path);

// tries to open given file. Sets the open flag of the file to true on success. If the file could not be found,
// an error message is printed and the open flag is set to false. If bufferSize is greater than 0, this file
// is read using a buffer rather than std::getline
// This will also try to read the files version number
extern HE_API void heWin32FileOpen(HeWin32TextFile* file, std::string const& path, uint32_t const bufferSize);
// closes the file and sets all data to 0
extern HE_API void heWin32FileClose(HeWin32TextFile* file);
// gets the next line from the given file and increases the line number
extern HE_API std::string heWin32FileGetLine(HeWin32TextFile* file);


// -- window

// creates the windows class with name HiraethEngine3 if it wasnt created before
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
extern HE_API void heWin32TimerStart();
// gets the latest time entry (last in first out) in milliseconds
extern HE_API double heWin32TimerGet();
// prints the latest time entry
extern HE_API inline void heWin32TimerPrint(std::string const& id);


// -- utils

// returns a string from the clipboard (if available) or an empty string if no text is in the clipboard
extern HE_API std::string heWin32ClipboardGet();


#endif
