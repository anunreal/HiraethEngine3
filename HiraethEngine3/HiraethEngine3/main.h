#ifndef MAIN_H
#define MAIN_H

#include "hnClient.h"
#include "src/heD3.h"
#include "src/heWindow.h"
#include "src/heRenderer.h"
#include <map>

#define USE_PHYSICS 1
#define USE_NETWORKING 1

struct Player {
	HnLocalClient* client = nullptr;
	HeD3Instance*  model  = nullptr;
	char name[256];
	hm::vec3f velocity;
};

struct App {
	HnClient	   client;
	HeD3Level	   level;
	HePhysicsActor actor;
	HeWindow       window;
	HeRenderEngine engine;
	std::map<unsigned int, Player> players;

	char ownName[256];
};

extern App app;

extern void checkWindowInput(HeWindow* window, HeD3Camera* camera, float const delta);

#endif
