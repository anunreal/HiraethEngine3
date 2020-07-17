#include "minecraft.h"
#include "heUi.h"
#include "heConsole.h"
#include "heCore.h"
#include "heWin32Layer.h"

Minecraft app;
std::thread worldCreateThread;

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

    if(heWindowKeyWasPressed(&app.window, HE_KEY_ESCAPE)) {
        if(app.state == GAME_STATE_PAUSED && heConsoleGetState() == HE_CONSOLE_STATE_CLOSED) {
            toggleMenuMode(false);
        } else if(app.state == GAME_STATE_INGAME) {
            toggleMenuMode(true);
        }
    }

    // game input
    if(app.window.active && app.state == GAME_STATE_INGAME) {
        if(app.window.mouseInfo.rightButtonPressed)
            app.freeCamera = !app.freeCamera;

        // rotation
		app.world.camera.rotation.x += app.window.mouseInfo.deltaMousePosition.y * 0.05f;
		app.world.camera.rotation.y += app.window.mouseInfo.deltaMousePosition.x * 0.05f;
        
        // movement
        hm::vec3f velocity = hm::vec3f(0.f);
        double angle = hm::to_radians(app.world.camera.rotation.y);
        float speed = 4.0f * (float) app.window.frameTime;
	
        if (app.window.keyboardInfo.keyStatus[HE_KEY_W]) {
            // move forward
            velocity.x = (float)std::sin(angle) * speed;
            velocity.z = (float)-std::cos(angle) * speed;
        }
	
        if (app.window.keyboardInfo.keyStatus[HE_KEY_A]) {
            // move left
            velocity.x -= (float)std::cos(angle) * (speed);
            velocity.z -= (float)std::sin(angle) * (speed);
        }
	
        if (app.window.keyboardInfo.keyStatus[HE_KEY_D]) {
            // move right
            velocity.x += (float)std::cos(angle) * (speed);
            velocity.z += (float)std::sin(angle) * (speed);
        }
	
        if (app.window.keyboardInfo.keyStatus[HE_KEY_S]) {
            // move backwards
            velocity.x += (float)-std::sin(angle) * speed;
            velocity.z += (float)std::cos(angle) * speed;
        }
	
        if (app.freeCamera && app.window.keyboardInfo.keyStatus[HE_KEY_E])
            velocity.y = speed;
	
        if (app.freeCamera && app.window.keyboardInfo.keyStatus[HE_KEY_Q])
            velocity.y = -speed;

        velocity *= 50.f * (app.window.keyboardInfo.keyStatus[HE_KEY_LSHIFT] ? 2.f : 1.f);

        if(app.freeCamera) {
            hePhysicsActorSetVelocity(&app.world.actor, hm::vec3f(0.f));
            app.world.camera.position += velocity * (float) app.window.frameTime;
        } else {
            hePhysicsActorSetVelocity(&app.world.actor, velocity * 0.016f);
            if(heWindowKeyWasPressed(&app.window, HE_KEY_SPACE) && hePhysicsActorOnGround(&app.world.actor))
                hePhysicsActorJump(&app.world.actor);
        }
    }
};

void enterGameState() {
    app.state = GAME_STATE_INGAME;
    createWorld(&app.world);
    worldCreateThread = std::thread(worldCreationThread, &app.world);
    
    if(app.window.active) {
        app.window.mouseInfo.cursorLock = hm::vec2f(-.5f);
        heWindowToggleCursor(false);
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
    
    HeScaledFont font;
    heFontCreateScaled(heAssetPoolGetFont("inconsolata"), &font, 13);
    heConsoleCreate(&app.engine, heAssetPoolGetFont("inconsolata"));
    enterGameState();
    
    heWindowUpdate(&app.window);
        
    // main loop
    while(!app.window.shouldClose) {
        { // prepare frame
            heWindowUpdate(&app.window);
            heRenderEnginePrepare(&app.engine);
        }

        { // game logic
            updateInput();
            updateWorld(&app.world);
        }

        { // render
            if(app.state == GAME_STATE_INGAME || app.state == GAME_STATE_PAUSED)
                renderWorld(&app.engine, &app.world);

            heRenderEnginePrepareUi(&app.engine);
            heUiPushText(&app.engine, &font, "FPS: " + std::to_string(app.window.fps), hm::vec2f(10, 10), hm::colour(255));
            heUiPushText(&app.engine, &font, "Position: " + hm::to_string(app.world.camera.position), hm::vec2f(10, 25), hm::colour(255));
            heUiPushText(&app.engine, &font, "Rotation: " + hm::to_string(app.world.camera.rotation), hm::vec2f(10, 40), hm::colour(255));
            heConsoleRender(&app.engine);
            heUiQueueRender(&app.engine, &app.world.camera);
            heRenderEngineFinishUi(&app.engine);
        }

        { // finish frame
            heRenderEngineFinish(&app.engine);
            heWindowSyncToFps(&app.window);
        }
    }

    app.state = GAME_STATE_DONE;
    worldCreateThread.join();
    
    hePostProcessEngineDestroy(&app.engine.postProcess);
    heRenderEngineDestroy(&app.engine);
    heWindowDestroy(&app.window);
    return 0;
};
