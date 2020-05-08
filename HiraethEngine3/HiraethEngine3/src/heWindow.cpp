#include "heWindow.h"
#include "heWin32Layer.h"
#include "heGlLayer.h"
#include "heCore.h"
#include <map>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#ifdef HE_USE_WIN32
#include "heWin32Layer.h"
#else
#error "Please specify a platform"
#endif

b8 heIsMainThread() {
#ifdef HE_USE_WIN32
	return heWin32IsMainThread();
#endif
};

b8 heWindowCreate(HeWindow* window) {
#ifdef HE_USE_WIN32
	return heWin32WindowCreate(window);
#endif
};

void heWindowUpdate(HeWindow* window) {
	
	window->mouseInfo.leftButtonDown = false;
	window->mouseInfo.rightButtonDown = false;
	window->mouseInfo.deltaMousePosition = hm::vec2i(0);
	window->keyboardInfo.keysPressed.clear();
	
#ifdef HE_USE_WIN32
	heWin32WindowUpdate(window);
#endif
	
};

void heWindowDestroy(HeWindow* window) {
#ifdef HE_USE_WIN32
	heWin32WindowDestroy(window);
#endif
};

void heWindowSyncToFps(HeWindow* window) {
	
	auto now = std::chrono::system_clock::now();
	double nowTime = (double)std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

	if (window->lastFrame == 0.)
		window->lastFrame = nowTime;
	
	// make sure that vsync is not enabled
	if (window->windowInfo.fpsCap > 0) {
		double deltaTime = nowTime - window->lastFrame; // in ms
		
		double requestedTime = (1000. / window->windowInfo.fpsCap); // in ms
		double sleepTime = (requestedTime - deltaTime); // in ms

		if (sleepTime > 0.) {
			std::chrono::milliseconds duration((int)sleepTime);
			std::this_thread::sleep_for(duration);
		};
	}
	
	now = std::chrono::system_clock::now();
	nowTime = (double)std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
	window->frameTime = (nowTime - window->lastFrame) / 1000.f;
	window->lastFrame = nowTime;
};

void heWindowEnableVsync(const int8_t timestamp) {
#ifdef HE_USE_WIN32
	heWin32WindowEnableVsync(timestamp);
#endif
};

void heWindowSwapBuffers(const HeWindow* window) {
#ifdef HE_USE_WIN32
	heWin32WindowSwapBuffers(window);
#endif
};

hm::vec2i heWindowCalculateBorderSize(const HeWindow* window) {
#ifdef HE_USE_WIN32
	return heWin32WindowCalculateBorderSize(window);
#endif
};

void heWindowToggleCursor(const b8 hidden) {
#ifdef HE_USE_WIN32
	heWin32WindowToggleCursor(hidden);
#endif
};

void heWindowSetCursorPosition(HeWindow* window, const hm::vec2f& position) {
#ifdef HE_USE_WIN32
	heWin32WindowSetCursorPosition(window, position);
#endif
};


b8 heWindowKeyWasPressed(const HeWindow* window, const HeKeyCode key) {
	
	return std::find(window->keyboardInfo.keysPressed.begin(), window->keyboardInfo.keysPressed.end(), key)
		!= window->keyboardInfo.keysPressed.end();
	
};
