#ifndef MAIN_H
#define MAIN_H

#include "hnClient.h"
#include "src/heD3.h"
#include "src/heWindow.h"
#include "src/heRenderer.h"
#include <map>

#define USE_PHYSICS 1
#define USE_NETWORKING 0

enum GameState {
    GAME_STATE_MAIN_MENU,
    GAME_STATE_INGAME,
    GAME_STATE_PAUSED
};

struct Player {
    HnLocalClient* client = nullptr;
    HeD3Instance*  model  = nullptr;
    char name[200] = { 0 };
    hm::vec3f velocity;
};

struct App {
    HnClient       client;
    HeD3Level      level;
    HePhysicsActor actor;
    HeWindow       window;
    HeRenderEngine engine;
    std::map<unsigned int, Player> players;

    GameState state = GAME_STATE_MAIN_MENU;

    char ownName[200] = { 0 };
};

extern App app;

extern void checkWindowInput(HeWindow* window, HeD3Camera* camera, float const delta);

#endif
