#ifndef MINECRAFT_H
#define MINECRAFT_H

#include "heWindow.h"
#include "heRenderer.h"

enum GameState {
    GAME_STATE_MAIN_MENU,
    GAME_STATE_INGAME,
    GAME_STATE_PAUSED
};

struct Minecraft {
    HeWindow window;
    HeRenderEngine engine;
    GameState state;    
};

extern Minecraft app;

#endif
