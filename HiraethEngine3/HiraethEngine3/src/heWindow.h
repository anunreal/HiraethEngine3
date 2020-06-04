#ifndef HE_WINDOW_H
#define HE_WINDOW_H

#include "hm/hm.hpp"
#include "heTypes.h"
#include <map>
#include <vector>

struct HeWindow;

// called when a text input happened. A callback should return true if it handles this event. Returning true will
// block the other callbacks from receiving this event
typedef b8(*HeWindowTextInputCallback)  (HeWindow*, uint32_t  const);
// called when a key was pressed. A callback should return true if it handles this event. Returning true will
// block the other callbacks from receiving this event
typedef b8(*HeWindowKeyPressCallback)   (HeWindow*, HeKeyCode const);
// called when the mouse wheel is scrolled. If the argument is positive, the wheel is scrolled away from the user
// (in real life) -> towards the display. The value represents the wheel movement. The vector is the current mouse
// position in window space. Returning true will block the other callbacks from receiving this event
typedef b8(*HeWindowMouseScrollCallback)(HeWindow*, int8_t const, hm::vec2i const&);

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
    hm::vec2f deltaMousePosition;
    b8 leftButtonDown     = false; // true if the left button is currently being held down
    b8 rightButtonDown    = false; // true if the right button is currently being held down
    b8 leftButtonPressed  = false; // true if the left button was pressed in the current frame
    b8 rightButtonPressed = false; // true if the right button was pressed in the current frame
    
    // a vector of callbacks that should be called when the mouse wheel is scrolled
    std::vector<HeWindowMouseScrollCallback> scrollCallbacks;
    // the position that the cursor should be locked at. If this is exactly 0, the cursor is unlocked. If the
    // coordinatesss are above 0, the cursor will be locked at that pixel position (relative to top left corner
    // of the window). if this is between 0 and 1, the coordinates are in percentage of the window size, meaning
    // that -.5 is the center of the window
    hm::vec2f cursorLock = hm::vec2f(0.f);
};

struct HeKeyboardInfo {
    // maps every key to their status, if a key is down, the value will be true, if the key is not down
    // the value will be true
    std::map<HeKeyCode, b8> keyStatus;
    // a vector of all keys that were pushed down in the last frame, useful for i.e. typing. This vector is cleard
    //in the heUpdateWindow function
    std::vector<HeKeyCode> keysPressed;
    // a vector of callbacks that should be called when a text input happened (ascii characters, space, return...)
    std::vector<HeWindowTextInputCallback> textInputCallbacks;
    // a vector of callbacks that should be called when a key is pressed. When a key is pressed, this will loop
    // through all callbacks until one returns true
    std::vector<HeWindowKeyPressCallback> keyPressCallbacks;
};

// This is the platform independant window
struct HeWindow {
    // information
    HeKeyboardInfo keyboardInfo;
    HeWindowInfo   windowInfo;
    HeMouseInfo    mouseInfo;
    b8             shouldClose = false;
    b8             active      = false; // is this the topmost window?
    b8             resized     = false; // did the window get resized in the last frame

    // timing stuff
    double lastFrame  = 0.; // the last frame time (time_since_epoch)
    double frameTime  = 0.; // the duration of the last frame (in seconds)
    double currentFps = 0.; // current fps count, on good enough systems should be close to the requested fps count
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
