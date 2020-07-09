#ifndef HE_CONSOLE_H
#define HE_CONSOLE_H

#include "heTypes.h"
#include "heAssets.h"

struct HeRenderEngine; // avoid include

// sets up the ingame console with the given font
extern HE_API void heConsoleCreate(HeRenderEngine* engine, HeFont* backlogFont);
// sets the next state of the console. The console will interpolate to that state now
extern HE_API void heConsoleOpen(HeConsoleState const state);
// if the current state of the console is already state, this will close the console. Else this will set the new
// state of the console to state
extern HE_API void heConsoleToggleOpen(HeConsoleState const state);
// adds given message to the backlog of the console
extern HE_API void heConsolePrint(std::string const& message);
// renders the console (if its not closed)
extern HE_API void heConsoleRender(HeRenderEngine* engine);
// registeres a new command for this console.
extern HE_API void heConsoleRegisterCommand(std::string const& name, void(*proc)(std::vector<std::string> const& args));
// tries to run the given command. Arguments must be seperated by a whitespace
extern HE_API void heConsoleCommandRun(std::string const& string);
// returns the current state of the console
extern HE_API HeConsoleState heConsoleGetState();

#endif
