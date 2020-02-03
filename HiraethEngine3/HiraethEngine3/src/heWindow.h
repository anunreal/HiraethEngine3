#pragma once
#include "hm/hm.hpp"
#include "heTypes.h"
#include <map>
#include <vector>
#include <Windows.h>

struct HeWindowInfo {
	// the background colour of the window
	hm::colour   backgroundColour;
	// the size of the window, in pixels
	hm::vec2i    size;
	// the name of the window
	std::wstring title = L"";
	// maximum fps allowed. If this is set to 0, vsync will be enabled
	unsigned int fpsCap = 0;
};

struct HeMouseInfo {
	// in pixels, with (0|0) at top left
	hm::vec2i mousePosition;
	// pixels the mouse moved during the last frame. Positive if the mouse moved down/right
	hm::vec2i deltaMousePosition;
	bool leftButtonDown = false;
	bool rightButtonDown = false;
};

struct HeKeyboardInfo {
	// maps every key to their status, if a key is down, the value will be true, if the key is not down
	// the value will be true
	std::map<HeKeyCode, bool> keyStatus;
	// a vector of all keys that were pushed down in the last frame, useful for i.e. typing. This vector is cleard in
	// the heUpdateWindow function
	std::vector<HeKeyCode> keysPressed;
};

struct HeWindow {
	// information
	HeKeyboardInfo keyboardInfo;
	HeWindowInfo   windowInfo;
	HeMouseInfo    mouseInfo;
	bool           shouldClose = false;
	bool		   active = false; // is this the topmost window?

	// opengl stuff
	HGLRC        context = nullptr;
	HWND         handle = nullptr;
	HDC          dc = nullptr;

	// timing stuff
	double lastFrame = 0.; // the last frame time (time_since_epoch)
	double frameTime = 0.; // the duration of the last frame (in seconds)
};

// creates the windows class with name Hiraeth2D if it wasnt created before
extern HE_API bool heSetupClassInstance();
// creates a dummy context needed for retrieving function pointers (context creation with attributes...)
extern HE_API void heCreateDummyContext();
// creates the window
extern HE_API bool heCreateWindow(HeWindow* window);
// updates the input of the window and clears the buffer
extern HE_API void heUpdateWindow(HeWindow* window);
// destroys the window and its context
extern HE_API void heDestroyWindow(HeWindow* window);
// sleeps in the current thread until the requested fps cap is reached
extern HE_API void heSyncToFps(HeWindow* window);
// enables vsync. Should only be called once. Timestamp is the number of frames to wait before doing the next
// one. Should always be one. Called when the window is created and fpsCap is 0
extern HE_API void heEnableVsync(unsigned int timestamp);
// swaps the buffers of the given window. Should be called after rendering the frame
extern HE_API void heSwapWindow(const HeWindow* window);
// calculates the window border sizes (caption bar...) 
extern HE_API hm::vec2i heCalculateWindowBorderSize(const HeWindow* window);

// returns true if given key was pressed during the last frame on given window.
// This simply searches the vector of keys pressed of the window's keyboardInfo for key
extern inline HE_API bool heKeyWasPressed(const HeWindow* window, const HeKeyCode key);

// sets the mouse to the given position relative to the window.
// The position can either be in pixels (positive), or as a percentage (negative) meaning
// that -0.5 will be have the current window size
extern HE_API void heSetMousePosition(HeWindow* window, const hm::vec2f& position);

// the windows class instance
extern HINSTANCE classInstance;								