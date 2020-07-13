#include "minecraft.h"
#include "heUi.h"
#include "heConsole.h"
#include "heCore.h"
#include "heWin32Layer.h"

Minecraft app;

void toggleMenuMode(b8 const menuOpen) {
    if(menuOpen) {
        heWindowToggleCursor(true);
        app.window.mouseInfo.cursorLock = hm::vec2f(0.f);
        app.state = GAME_STATE_PAUSED;
    } else {
        heWindowToggleCursor(false);
        heWindowSetCursorPosition(&app.window, hm::vec2f(-.5f));
        app.window.mouseInfo.cursorLock = hm::vec2f(-.5f);
        app.state = GAME_STATE_INGAME;
    }
};

void updateInput() {
    // key binds
    if(heWindowKeyWasPressed(&app.window, HE_KEY_F1)) {
        if(app.window.keyboardInfo.keyStatus[HE_KEY_LSHIFT])
            heConsoleToggleOpen(HE_CONSOLE_STATE_OPEN_FULL);    
        else
            heConsoleToggleOpen(HE_CONSOLE_STATE_OPEN);

        if(heConsoleGetState() == HE_CONSOLE_STATE_CLOSED)
            toggleMenuMode(false);
        else
            toggleMenuMode(true);
    }
};
    
int main() {
    HeWindowInfo windowInfo;
    windowInfo.mode             = HE_WINDOW_MODE_WINDOWED;
    windowInfo.backgroundColour = hm::colour(100);
    windowInfo.size             = hm::vec2i(1280, 720);
    windowInfo.title            = L"Minecraft";
    windowInfo.fpsCap           = 70;
    heWindowCreate(&app.window, windowInfo);
    heRenderEngineCreate(&app.engine, &app.window, HE_RENDER_MODE_FORWARD);
    hePostProcessEngineCreate(&app.engine.postProcess, &app.window);
    heUiCreate(&app.engine);
    
    heConsoleCreate(&app.engine, heAssetPoolGetFont("inconsolata"));
	
    // main loop
    while(!app.window.shouldClose) {
        { // prepare frame
            heWindowUpdate(&app.window);
            heRenderEnginePrepare(&app.engine);
        }

        { // game logic
            updateInput();
        }

        { // render
            heRenderEnginePrepareUi(&app.engine);
            heConsoleRender(&app.engine);
            heUiQueueRender(&app.engine);
            heRenderEngineFinishUi(&app.engine);
        }

        { // finish frame
            heRenderEngineFinish(&app.engine);
            heWindowSyncToFps(&app.window);
        }
    }

    hePostProcessEngineDestroy(&app.engine.postProcess);
    heRenderEngineDestroy(&app.engine);
    heWindowDestroy(&app.window);
    return 0;
};
