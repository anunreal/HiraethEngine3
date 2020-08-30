#include "ballz.h"
#include "heUi.h"
#include "heConsole.h"
#include "heCore.h"
#include "heWin32Layer.h"
#include "heLoader.h"

Ballz app;

void toggleMenuMode(b8 const menuOpen) {
    if (menuOpen) {
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
    if (heWindowKeyWasPressed(&app.window, HE_KEY_F1)) {
        if (app.window.keyboardInfo.keyStatus[HE_KEY_LSHIFT])
            heConsoleToggleOpen(HE_CONSOLE_STATE_OPEN_FULL);
        else
            heConsoleToggleOpen(HE_CONSOLE_STATE_OPEN);

        if (heConsoleGetState() == HE_CONSOLE_STATE_CLOSED)
            toggleMenuMode(false);
        else
            toggleMenuMode(true);
    }

    if (heWindowKeyWasPressed(&app.window, HE_KEY_ESCAPE)) {
        if (app.state == GAME_STATE_PAUSED && heConsoleGetState() == HE_CONSOLE_STATE_CLOSED) {
            toggleMenuMode(false);
        } else if (app.state == GAME_STATE_INGAME) {
            toggleMenuMode(true);
        }
    }

    // game input
    if (app.window.active && app.state == GAME_STATE_INGAME) {

        // rotation
        app.level.camera.rotation.x += app.window.mouseInfo.deltaMousePosition.y * 0.05f;
        app.level.camera.rotation.y += app.window.mouseInfo.deltaMousePosition.x * 0.05f;

        // movement
        hm::vec3f velocity = hm::vec3f(0.f);
        double angle = hm::to_radians(app.level.camera.rotation.y);
        float speed = 4.0f * (float)app.window.frameTime;

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

        velocity *= 2.f * (app.window.keyboardInfo.keyStatus[HE_KEY_LSHIFT] ? 2.f : 1.f);

        if (app.freeCamera) {
            //hePhysicsActorSetVelocity(&app.actor, hm::vec3f(0.f));
            //hePhysicsComponentSetRotation(&app.actor, app.level.camera.rotation);
            //hePhysicsComponentSetVelocity(&app.actor, hm::vec3f(0.f));
            app.level.camera.position += velocity * (float)app.window.frameTime;
        } else {
            //hePhysicsComponentSetVelocity(&app.actor, velocity);
            hePhysicsActorCustomSetVelocity(&app.actor, velocity);
            //if (heWindowKeyWasPressed(&app.window, HE_KEY_SPACE) && hePhysicsActorOnGround(&app.actor))
                //hePhysicsActorJump(&app.actor);
        }
    }
};

HePhysicsActorCustom actor;

void enterGameState() {
    app.state = GAME_STATE_INGAME;

    heD3LevelLoad("res/level/spielfeld.h3level", &app.level, true, true);
    heD3SkyboxLoad(&app.level.skybox, "pink_sunrise");
    heD3Level = &app.level;

    heD3LevelGetLightSource(&app.level, 0)->shadows.shadowDistance = 20;
    heD3LevelGetLightSource(&app.level, 0)->shadows.offset = 5;
    
    std::srand(std::time(nullptr));
    hePhysicsComponentSetPosition(heD3LevelGetInstance(&app.level, 0)->physics, hm::vec3f(((std::rand() / (float) RAND_MAX) * 2.f - 1.f) * 2.f, 10.f, ((std::rand() / (float) RAND_MAX) * 2.f - 1.f) * 2.f));
    
    HePhysicsShapeInfo actorShape;
    actorShape.type = HE_PHYSICS_SHAPE_BOX;
    actorShape.box = hm::vec3f(0.15f, 0.2f, 0.15f);
    actorShape.mass = 10;
    
    /*
    hePhysicsComponentCreate(&app.actor, actorShape);
    hePhysicsComponentSetPosition(&app.actor, hm::vec3f(-1.f, 2.f, 1.f));
    hePhysicsComponentEnableRotation(&app.actor, hm::vec3f(0.f, 0.f, 0.f));
    hePhysicsLevelAddComponent(&app.level.physics, &app.actor);
    */
    
    HePhysicsActorInfo actorInfo = HePhysicsActorInfo();
    actorInfo.stepHeight = 0.001f;
    hePhysicsActorCustomCreate(&app.actor, actorShape, actorInfo);
    hePhysicsActorCustomSetPosition(&app.actor, hm::vec3f(0.f, 10.f, 0.f));
    hePhysicsLevelSetActor(&app.level.physics, &app.actor);
    
    if (app.window.active) {
        app.window.mouseInfo.cursorLock = hm::vec2f(-.5f);
        heWindowToggleCursor(false);
    }
};

int main() {
    HeWindowInfo windowInfo;
    windowInfo.mode = HE_WINDOW_MODE_WINDOWED;
    windowInfo.backgroundColour = hm::colour(100);
    windowInfo.size = hm::vec2i(1280, 720);
    windowInfo.title = L"ballz";
    windowInfo.fpsCap = 70;
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
    while (!app.window.shouldClose) {
        { // prepare frame
            heWindowUpdate(&app.window);
            heRenderEnginePrepare(&app.engine);
        }

        { // game logic
            updateInput();
            
            if (app.state == GAME_STATE_INGAME || app.state == GAME_STATE_PAUSED) {
                heD3LevelUpdate(&app.level, (float) app.window.frameTime);
                hePhysicsActorCustomUpdate(&app.actor, &app.level.physics, (float) app.window.frameTime);
                //app.level.camera.position = hePhysicsComponentGetPosition(&app.actor);
                app.level.camera.position = app.actor.position;
            }
        }

        { // render d3
            if (app.state == GAME_STATE_INGAME || app.state == GAME_STATE_PAUSED) {
                heRenderEnginePrepareD3(&app.engine);
                heD3LevelRender(&app.engine, &app.level);
                heRenderEngineFinishD3(&app.engine);
            }
        }

        { // post process
            hePostProcessRender(&app.engine);
        }

        { // render ui
            heRenderEnginePrepareUi(&app.engine);
            heUiPushText(&app.engine, &font, "FPS: " + std::to_string(app.window.fps), hm::vec2f(10, 10), hm::colour(255));
            heUiPushText(&app.engine, &font, "Position: " + hm::to_string(app.level.camera.position), hm::vec2f(10, 25), hm::colour(255));
            heUiPushText(&app.engine, &font, "Rotation: " + hm::to_string(app.level.camera.rotation), hm::vec2f(10, 40), hm::colour(255));
            heConsoleRender(&app.engine);
            heUiQueueRender(&app.engine, &app.level.camera);
            heRenderEngineFinishUi(&app.engine);
        }

        { // finish frame
            heRenderEngineFinish(&app.engine);
            heWindowSyncToFps(&app.window);
        }
    }

    app.state = GAME_STATE_DONE;

    hePostProcessEngineDestroy(&app.engine.postProcess);
    heRenderEngineDestroy(&app.engine);
    heWindowDestroy(&app.window);
    return 0;
};
