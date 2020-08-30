#ifndef BALLZ_H
#define BALLZ_H

#include "heWindow.h"
#include "heRenderer.h"
#include "heD3.h"

#define USE_PHYSICS 1

enum GameState {
    GAME_STATE_MAIN_MENU,
    GAME_STATE_INGAME,
    GAME_STATE_PAUSED,
    GAME_STATE_DONE
};

struct Ballz {
    HeWindow window;
    HeRenderEngine engine;
    GameState state = GAME_STATE_MAIN_MENU;

    HePhysicsActorCustom actor;
    //HePhysicsComponent actor;
    HeD3Level level;

    b8 freeCamera = false;
};

extern Ballz app;

#endif
