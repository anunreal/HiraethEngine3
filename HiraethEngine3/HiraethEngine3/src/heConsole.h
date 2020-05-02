#ifndef HE_CONSOLE_H
#define HE_CONSOLE_H

#include "heTypes.h"
#include "heAssets.h"

extern HE_API void heConsoleCreate(HeFont* backlogFont);
extern HE_API void heConsoleOpen(HeConsoleState const state);
extern HE_API void heConsoleToggleOpen(HeConsoleState const state);
extern HE_API void heConsolePrint(std::string const& message);
extern HE_API void heConsoleRender(float const delta);
extern HE_API void heConsoleRegisterCommand(std::string const& name, void(*proc)(std::vector<std::string> const& args));

extern HE_API HeConsoleState heConsoleGetState();

#endif
