#ifndef HE_WINDOW_H
#define HE_WINDOW_H

#include "hm/hm.hpp"
#include "heTypes.h"
#include <map>
#include <vector>

struct HeWindow;

typedef bool(*HeWindowTextInputCallback)(HeWindow*, uint32_t const);

struct HeWindowInfo {
    // the background colour of the window
    hm::colour   backgroundColour;
    // the size of the window, in pixels
    hm::vec2i    size;
    // the name of the window
    std::wstring title   = L"";
    // maximum fps allowed. If this is set to 0, vsync will be enabled
    uint16_t fpsCap  = 0;
    // samples used for one pixel. If this is left at 1, no multisampling will be used. The higher this value,
    // the smoother the result will be at a higher performance and memory cost
    uint8_t samples = 1;
};

struct HeMouseInfo {
    // in pixels, with (0|0) at top left
    hm::vec2i mousePosition;
    // pixels the mouse moved during the last frame. Positive if the mouse moved down/right
    hm::vec2i deltaMousePosition;
    b8 leftButtonDown = false;
    b8 rightButtonDown = false;
};

struct HeKeyboardInfo {
    // maps every key to their status, if a key is down, the value will be true, if the key is not down
    // the value will be true
    std::map<HeKeyCode, b8> keyStatus;
    // a vector of all keys that were pushed down in the last frame, useful for i.e. typing. This vector is cleard in
    // the heUpdateWindow function
    std::vector<HeKeyCode> keysPressed;
	std::vector<HeWindowTextInputCallback> textInputCallbacks;
};

// This is the platform independant window
struct HeWindow {
    // information
    HeKeyboardInfo keyboardInfo;
    HeWindowInfo   windowInfo;
    HeMouseInfo    mouseInfo;
    b8             shouldClose = false;
    b8		     active = false; // is this the topmost window?
    
    // timing stuff
    double lastFrame = 0.; // the last frame time (time_since_epoch)
    double frameTime = 0.; // the duration of the last frame (in seconds)
};

// returns true if there is a valid rendering context in the current thread
extern HE_API b8 heIsMainThread();
// creates the window
extern HE_API b8 heWindowCreate(HeWindow* window);
// updates the input of the window and clears the buffer
extern HE_API void heWindowUpdate(HeWindow* window);
// destroys the window and its context
extern HE_API void heWindowDestroy(HeWindow* window);
// sleeps in the current thread until the requested fps cap is reached
extern HE_API void heWindowSyncToFps(HeWindow* window);
// enables vsync. Should only be called once. Timestamp is the number of frames to wait before doing the next
// one. Should always be one. Called when the window is created and fpsCap is 0
extern HE_API void heWindowEnableVsync(const int8_t timestamp);
// swaps the buffers of the given window. Should be called after rendering the frame
extern HE_API void heWindowSwapBuffers(const HeWindow* window);
// switches the state of the cursor between visible and hidden
extern HE_API void heWindowToggleCursor(const b8 hidden);
// sets the mouse to the given position relative to the window.
// The position can either be in pixels (positive), or as a percentage (negative) meaning
// that -0.5 will be have the current window size
extern HE_API void heWindowSetCursorPosition(HeWindow* window, const hm::vec2f& position);
// calculates the window border sizes (caption bar...) 
extern HE_API hm::vec2i heWindowCalculateBorderSize(const HeWindow* window);

// returns true if given key was pressed during the last frame on given window.
// This simply searches the vector of keys pressed of the window's keyboardInfo for key
extern inline HE_API b8 heWindowKeyWasPressed(const HeWindow* window, const HeKeyCode key);

#endif
