#ifndef MAIN_H
#define MAIN_H

#include "heD3.h"
#include "heWindow.h"
#include "heRenderer.h"

#define USE_PHYSICS 1
#define USE_NETWORKING 0

enum GameState {
	GAME_STATE_MAIN_MENU,
	GAME_STATE_INGAME,
	GAME_STATE_PAUSED
};

struct Player {
	HeD3Instance* model = nullptr;
	char name[200] = { 0 };
	hm::vec3f velocity;
};

struct App {
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
