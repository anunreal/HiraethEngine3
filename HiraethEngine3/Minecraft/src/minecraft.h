#ifndef MINECRAFT_H
#define MINECRAFT_H

#include "heWindow.h"
#include "heRenderer.h"
#include "world.h"

#define USE_PHYSICS 1

enum GameState {
    GAME_STATE_MAIN_MENU,
    GAME_STATE_INGAME,
    GAME_STATE_PAUSED,
    GAME_STATE_DONE
};

struct Minecraft {
    HeWindow window;
    HeRenderEngine engine;
    GameState state;    
    World world;
    
    b8 freeCamera = false;
};

extern Minecraft app;

#endif
