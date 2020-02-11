#pragma once
#include "heTypes.h"

// returns true if the file was modified since the last time it was checked (with this function). If this is
// the first time this file is checked, it is assumed to not have changed. From then on the last access time will
// be stored
extern HE_API bool heFileModified(const std::string& file);