#ifndef HE_WIN32_LAYER_H
#define HE_WIN32_LAYER_H

#include "heTypes.h"
#include <string>

/* NOTE: ALL THESE FUNCTIONS ARE ONLY DEFINED IF HE_USE_WIN32 IS DEFINED */

// returns true if the file was modified since the last time it was checked (with this function). If this is
// the first time this file is checked, it is assumed to not have changed. From then on the last access time will
// be stored
extern HE_API bool heFileModified(const std::string& file);

// creates the windows class with name Hiraeth2D if it wasnt created before
extern HE_API bool heWin32SetupClassInstance();
// creates a dummy context needed for retrieving function pointers (context creation with attributes...)
extern HE_API void heWin32CreateDummyContext();

#endif