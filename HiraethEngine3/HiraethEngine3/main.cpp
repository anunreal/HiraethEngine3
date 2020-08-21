#include "hepch.h"

#ifdef HE_ENABLE_MEMORY_CHECKING
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include "main.h"
#include "src/heWin32Layer.h"
#include "src/heConsole.h"
#include "src/heLoader.h"
#include "src/heCore.h"
#include "src/heDebugUtils.h"
#include "src/heUi.h"
#include <windows.h>
#include <thread>
#include <vector>

App app;

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

    if(heWindowKeyWasPressed(&app.window, HE_KEY_F3)) {
        heProfilerToggleDisplay();
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
            app.level.freeCamera = !app.level.freeCamera;

        // rotation
		app.level.camera.rotation.x += app.window.mouseInfo.deltaMousePosition.y * 0.05f;
		app.level.camera.rotation.y += app.window.mouseInfo.deltaMousePosition.x * 0.05f;
        
        // movement
        hm::vec3f velocity = hm::vec3f(0.f);
        double angle = hm::to_radians(app.level.camera.rotation.y);
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
	
        if (app.level.freeCamera && app.window.keyboardInfo.keyStatus[HE_KEY_E])
            velocity.y = speed;
	
        if (app.level.freeCamera && app.window.keyboardInfo.keyStatus[HE_KEY_Q])
            velocity.y = -speed;

        velocity *= 30.f * (app.window.keyboardInfo.keyStatus[HE_KEY_LSHIFT] ? 2.f : 1.f);

        if(app.level.freeCamera) {
            hePhysicsActorSimpleSetVelocity(&app.actor, hm::vec3f(0.f));
            app.level.camera.position += velocity * (float) app.window.frameTime;
        } else {
            hePhysicsActorSimpleSetVelocity(&app.actor, velocity * 0.016f);
            if(heWindowKeyWasPressed(&app.window, HE_KEY_SPACE) && hePhysicsActorSimpleOnGround(&app.actor))
                hePhysicsActorSimpleJump(&app.actor);
        }
    }
};

void enterGameState() {
    app.state = GAME_STATE_INGAME;

    heWin32TimerStart();
    heD3LevelLoad("res/level/level0.h3level", &app.level, USE_PHYSICS, true);
    heD3LevelGetInstance(&app.level, 13)->material->emission = hm::colour(255, 0, 0, 255, 10); // suzanne
    app.level.camera.frustum.viewInfo.fov = 90;

    heAssetPoolGetSpriteAtlas("res/textures/particleAtlas.png", 2, 2, 4); // load and set up once so we can use it later
    HeParticleSource* lampParticles = &app.level.particles.emplace_back();
    lampParticles->maxNewParticlesPerUpdate = 10;
    lampParticles->gravity           = 0.f;
    lampParticles->additive          = false;
    lampParticles->emitter.type      = HE_PARTICLE_EMITTER_TYPE_BOX;
    lampParticles->emitter.box       = hm::vec3f(0.6f);
    lampParticles->emitter.maxSpeed  = lampParticles->emitter.minSpeed = 0.f;
    lampParticles->emitter.maxColour = lampParticles->emitter.minColour = hm::colour(255, 7.f);
    lampParticles->enableShadows     = false;
    heParticleSourceCreate(lampParticles, HeD3Transformation(hm::vec3f(-3.07f, 4.07f, -2.03f)), heAssetPoolGetSpriteAtlas("res/textures/particleAtlas.png"), 3, 200);

    HeParticleSource* dustParticles = &app.level.particles.emplace_back();
    dustParticles->maxNewParticlesPerUpdate = 100;
    dustParticles->gravity           = .05f;
    dustParticles->emitter.type      = HE_PARTICLE_EMITTER_TYPE_BOX;
    dustParticles->emitter.box       = hm::vec3f(10.f, .75f, 10.f);
    dustParticles->emitter.minSpeed  = 0.3f;
    dustParticles->emitter.maxSpeed  = 1.0f;
    dustParticles->emitter.minTime   = 1.0f;
    dustParticles->emitter.maxTime   = 4.f;
    dustParticles->emitter.minSize   = 0.005f; 
    dustParticles->emitter.maxSize   = 0.03f; 
    dustParticles->emitter.minColour = hm::colour(20, 3.f);
    dustParticles->emitter.maxColour = hm::colour(100, 3.f);
    heParticleSourceCreate(dustParticles, HeD3Transformation(hm::vec3f(0.f, .75f, 0.f)), heAssetPoolGetSpriteAtlas("res/textures/particleAtlas.png"), 0, 200);

#if USE_PHYSICS == 1
	HePhysicsShapeInfo actorShape;
	actorShape.type = HE_PHYSICS_SHAPE_CAPSULE;
	actorShape.capsule = hm::vec2f(0.75f, 2.0f);
	
	HePhysicsActorInfo actorInfo;
	actorInfo.eyeOffset = 1.6f;
	
	hePhysicsActorSimpleCreate(&app.actor, actorShape, actorInfo);
    hePhysicsLevelSetActor(&app.level.physics, &app.actor);
	hePhysicsActorSimpleSetEyePosition(&app.actor, hm::vec3f(-5.f, 1.7f, 5.f));
#endif

    heWin32TimerPrint("LEVEL LOAD");

    heWin32TimerStart();
    heD3SkyboxLoad(&app.level.skybox, "pink_sunrise");
    heWin32TimerPrint("SKYBOX CREATION");

    app.state = GAME_STATE_INGAME;
    heD3Level = &app.level;
    if(app.window.active) {
        app.window.mouseInfo.cursorLock = hm::vec2f(-.5f);
        heWindowToggleCursor(false);
    }
};

int main() {
#ifdef HE_ENABLE_MEMORY_CHECKING
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
#endif

	// create engine
	heWin32TimerStart(); // total
	heWin32TimerStart(); // engine
	HeWindowInfo windowInfo;
	windowInfo.title = L"He3 Test";
	windowInfo.backgroundColour = hm::colour(135, 206, 235);
	windowInfo.fpsCap = 70;
	windowInfo.size = hm::vec2i(1280, 720);
	windowInfo.mode = HE_WINDOW_MODE_WINDOWED;
	heWindowCreate(&app.window, windowInfo);
	heGlPrintInfo();

	heRenderEngineCreate(&app.engine, &app.window, HE_RENDER_MODE_FORWARD);
	hePostProcessEngineCreate(&app.engine.postProcess, &app.window);
	heUiCreate(&app.engine);

    // set shit up
    HeScaledFont font;
    heFontCreateScaled(heAssetPoolGetFont("inconsolata"), &font, 13);
    heConsoleCreate(&app.engine, heAssetPoolGetFont("inconsolata"));
	heProfilerCreate(heAssetPoolGetFont("inconsolata"));
    heWin32TimerPrint("ENGINE STARTUP");    
    
    enterGameState();

    double sleepTime = 0.;
    double profilerTime = 0.;
    heWin32TimerPrint("TOTAL STARTUP");    

    while(!app.window.shouldClose) {
        // frame setup
        heProfilerFrameStart();
        {
            heWindowUpdate(&app.window);
            heRenderEnginePrepare(&app.engine);
        }
        heProfilerFrameMark("prepare", hm::colour(100));

        // input
        {
            updateInput();
        }
		heProfilerFrameMark("input", hm::colour(255, 0, 0));
        
        // render d3
        {
            heRenderEnginePrepareD3(&app.engine);
            heD3LevelUpdate(&app.level, (float) app.window.frameTime);
            heProfilerFrameMark("update", hm::colour(255, 100, 100));
            heD3LevelRender(&app.engine, &app.level);
            heRenderEngineFinishD3(&app.engine);
        }
        heProfilerFrameMark("render d3", hm::colour(0, 255, 0));

        // post process
        {
            hePostProcessRender(&app.engine);
        }
        heProfilerFrameMark("post process", hm::colour(0, 255, 255));

        // render ui
        {
            heRenderEnginePrepareUi(&app.engine);
            heUiPushText(&app.engine, &font, "FPS: " + std::to_string(app.window.fps), hm::vec2f(10, 10), hm::colour(255));
            heUiPushText(&app.engine, &font, "Position: " + hm::to_string(app.level.camera.position), hm::vec2f(10, 25), hm::colour(255));
            heUiPushText(&app.engine, &font, "Rotation: " + hm::to_string(app.level.camera.rotation), hm::vec2f(10, 40), hm::colour(255));
            heConsoleRender(&app.engine);
            heUiQueueRender(&app.engine, &app.level.camera);
            heRenderEngineFinishUi(&app.engine);
        }
        heProfilerFrameMark("ui render", hm::colour(0, 0, 255));
                
        // render profiler
        {
            heWin32TimerStart();
            heProfilerAddEntry("profiler", profilerTime, hm::colour(255, 255, 0));
            heProfilerAddEntry("sleep", sleepTime, hm::colour(20, 20, 20));
            heProfilerRender(&app.engine);            
            profilerTime = heWin32TimerGet();
        }        
        
        // frame finish
        {
            heWin32TimerStart();
            heRenderEngineFinish(&app.engine);
            heWindowSyncToFps(&app.window);
            sleepTime = heWin32TimerGet();
        }
    }    

    HE_DEBUG("Cleaning up...");
    heD3LevelDestroy(&app.level);
    heRenderEngineDestroy(&app.engine);
    heWindowDestroy(&app.window);
    HE_DEBUG("Successfull shutdown");
};


/*
#include "main.h"
#include "src/heWin32Layer.h"
#include "src/heConsole.h"
#include "src/heLoader.h"
#include "src/heCore.h"
#include "src/heDebugUtils.h"
#include "src/heUi.h"
#include <windows.h>
#include <thread>
#include <vector>

bool	  inputActive = false;
bool	  freeCam = false;
hm::vec2i lastPosition;
hm::vec3f velocity;
hm::quatf cameraRotation;

App app;

void checkWindowInput(HeWindow* window, HeD3Camera* camera, float const delta) {
	if (app.state == GAME_STATE_INGAME && window->mouseInfo.leftButtonPressed) {
		inputActive = !inputActive;
		heWindowToggleCursor(inputActive);
	}
	
	if(window->mouseInfo.rightButtonPressed)
		freeCam = !freeCam;
	
	if(inputActive && app.state == GAME_STATE_INGAME) {
		POINT current_mouse_pos = {};
		::GetCursorPos(&current_mouse_pos);
		hm::vec2i current(current_mouse_pos.x, current_mouse_pos.y);
		hm::vec2f dmouse = hm::vec2f(current - lastPosition) * 0.05f;
		
		heWindowSetCursorPosition(window, window->windowInfo.size / 2);
		::GetCursorPos(&current_mouse_pos);
		lastPosition.x = (int) current_mouse_pos.x;
		lastPosition.y = (int) current_mouse_pos.y;
		
		camera->rotation.x += dmouse.y;
		camera->rotation.y += dmouse.x;
	}

	if(heConsoleGetState() != HE_CONSOLE_STATE_CLOSED)
		return;
	
	velocity = hm::vec3f(0.f);
	double angle = hm::to_radians(camera->rotation.y);
	float speed = 4.0f * delta;
	
	if (window->keyboardInfo.keyStatus[HE_KEY_W]) {
		// move forward
		velocity.x = (float)std::sin(angle) * speed;
		velocity.z = (float)-std::cos(angle) * speed;
	}
	
	if (window->keyboardInfo.keyStatus[HE_KEY_A]) {
		// move left
		velocity.x -= (float)std::cos(angle) * (speed);
		velocity.z -= (float)std::sin(angle) * (speed);
	}
	
	if (window->keyboardInfo.keyStatus[HE_KEY_D]) {
		// move right
		velocity.x += (float)std::cos(angle) * (speed);
		velocity.z += (float)std::sin(angle) * (speed);
	}
	
	if (window->keyboardInfo.keyStatus[HE_KEY_S]) {
		// move backwards
		velocity.x += (float)-std::sin(angle) * speed;
		velocity.z += (float)std::cos(angle) * speed;
	}
	
	if (window->keyboardInfo.keyStatus[HE_KEY_E])
		velocity.y = speed;
	
	if (window->keyboardInfo.keyStatus[HE_KEY_Q])
		velocity.y = -speed;

	velocity *= 30.f * (window->keyboardInfo.keyStatus[HE_KEY_LSHIFT] ? 2.f : 1.f);
	hm::vec3f v = velocity * (float) window->frameTime;
	if(!freeCam) {
		hePhysicsActorSetVelocity(&app.actor, v);
		if(heWindowKeyWasPressed(window, HE_KEY_SPACE) && hePhysicsActorOnGround(&app.actor))
			hePhysicsActorJump(&app.actor);
	} else {
		hePhysicsActorSetVelocity(&app.actor, hm::vec3f(0));
		camera->position += v;
	}
};

void _onClientConnect(HnClient* client, HnLocalClient* local) {
	Player* p = &app.players[local->clientId];
	p->client = local;

	HeD3Instance* instance = &app.level.instances.emplace_back();
	heD3InstanceLoadBinary("res/assets/bin/player.h3asset", instance, nullptr);
	HE_LOG("Loaded instance!");
	hnLocalClientHookVariable(client, local, "position", &instance->transformation.position);
	hnLocalClientHookVariable(client, local, "rotation", &instance->transformation.rotation);
	hnLocalClientHookVariable(client, local, "velocity", &p->velocity);
	hnLocalClientHookVariable(client, local, "name",     &p->name);
	
	p->model = instance;
};

void _onClientDisconnect(HnClient* client, HnLocalClient* local) {
	Player* p = &app.players[local->clientId];
	heD3LevelRemoveInstance(&app.level, p->model);
	app.players.erase(local->clientId);
};

void createClient(std::string const& host, std::string const& name) {
	strcpy_s(app.ownName, name.c_str());
	
	app.client.callbacks.clientConnect = &_onClientConnect;
	app.client.callbacks.clientDisconnect = &_onClientDisconnect;
	//hnClientCreate(&app.client, "tealfire.de", 9876, HN_PROTOCOL_TCP);
	hnClientCreate(&app.client, host, 9876, HN_PROTOCOL_UDP, 75);

	if(app.client.socket.status != HN_STATUS_CONNECTED)
		return;

	hnClientCreateVariableFixed(&app.client, "position", HN_VARIABLE_TYPE_FLOAT, 3, 10);
	hnClientHookVariable(&app.client, "position", &app.level.physics.actor->feetPosition);
	hnClientCreateVariableFixed(&app.client, "velocity", HN_VARIABLE_TYPE_FLOAT, 3, 2);
	hnClientHookVariable(&app.client, "velocity", &app.level.physics.actor->velocity);
	hnClientCreateVariableFixed(&app.client, "rotation", HN_VARIABLE_TYPE_FLOAT, 4, 10);
	hnClientHookVariable(&app.client, "rotation", &cameraRotation);
	hnClientCreateVariableFixed(&app.client, "name",     HN_VARIABLE_TYPE_CHAR, 200, 60);
	hnClientHookVariable(&app.client, "name",     &app.ownName);
	
	while(app.client.socket.status == HN_STATUS_CONNECTED) {
		cameraRotation = hm::fromEulerDegrees(-app.level.camera.rotation);
		hnClientHandleInput(&app.client);
	};
};

void modelStressTest(std::string const& file) {
	const int8_t COUNT = 1;
	HeD3Instance instance;
	
	heWin32TimerStart();
	
	for(int i = 0; i < COUNT; ++i) {
		heD3InstanceLoadBinary("res/assets/bin/" + file + ".h3asset", &instance, nullptr);
		heVaoDestroy(instance.mesh);
	}
	
	double bin = heWin32TimerGet();
	
	heWin32TimerStart();
	
	for(int i = 0; i < COUNT; ++i) {
		heD3InstanceLoad("res/assets/" + file + ".h3asset", &instance, nullptr);
		heVaoDestroy(instance.mesh);
	}
	
	double ascii = heWin32TimerGet();
	HN_LOG("Binary loading took: " + std::to_string(bin)   + "ms (avg: " + std::to_string(bin	/ COUNT) + ")");
	HN_LOG("ascii  loading took: " + std::to_string(ascii) + "ms (avg: " + std::to_string(ascii / COUNT) + ")");
};

void createWorld(HeWindow* window) {
	//modelStressTest("cerberus");
	
	heWin32TimerStart();
	heD3LevelLoad("res/level/level0.h3level", &app.level, USE_PHYSICS);

	// temporary: bloom test
	for(HeD3Instance& all : app.level.instances) {
		if(all.name == "Suzanne.h3asset")
			all.material->emission = hm::colour(255, 100, 100, 255, 10.f);
	}
	
	heWin32TimerPrint("LEVEL LOAD");
	
	heD3SkyboxCreate(&app.level.skybox, "res/textures/hdr/pink_sunrise.hdr");
    heD3Level = &app.level;

	HeD3Instance* instance = &app.level.instances.emplace_back();
    heD3InstanceLoadBinary("res/assets/bin/player.h3asset", instance, nullptr);
	instance->transformation.position = hm::vec3f(5, 0, -5);

    HeParticleSource* particles = &app.level.particles.emplace_back();
    heParticleSourceCreate(particles, HeD3Transformation(), heAssetPoolGetSpriteAtlas("res/textures/particleAtlas.png", 1, 1, 1), 0, 1000);
    for(uint32_t i = 0; i < 1000; ++i)
        heParticleSourceSpawnParticle(particles, i);
    
	HE_LOG("Successfully loaded lvl");

#if USE_PHYSICS == 1
	HePhysicsShapeInfo actorShape;
	actorShape.type = HE_PHYSICS_SHAPE_CAPSULE;
	actorShape.capsule = hm::vec2f(0.75f, 2.0f);
	
	HePhysicsActorInfo actorInfo;
	actorInfo.eyeOffset = 1.6f;
	
	hePhysicsActorCreate(&app.actor, actorShape, actorInfo);
	hePhysicsLevelSetActor(&app.level.physics, &app.actor);
	hePhysicsActorSetEyePosition(&app.actor, hm::vec3f(-5.f, 0.1f, 5.f));
#endif
};

// TODO(Victor): Multiple shapes per component

int main() {
	heWin32TimerStart();
	std::thread commandThread(heCommandThread, nullptr);
	
	HeWindowInfo windowInfo;
	windowInfo.title			= L"He3 Test";
	windowInfo.backgroundColour = hm::colour(135, 206, 235);
	windowInfo.fpsCap			= 61;
	windowInfo.size				= hm::vec2i(1366, 768);
	app.window.windowInfo		= windowInfo;
	
	heWindowCreate(&app.window);
	
	heGlPrintInfo();
	
    app.engine.renderMode = HE_RENDER_MODE_FORWARD;
	heRenderEngineCreate(&app.engine, &app.window);
	hePostProcessEngineCreate(&app.engine.postProcess, &app.window);
	heRenderEngine = &app.engine;

	createWorld(&app.window);

	HeFont* font = heAssetPoolGetFont("inconsolata");
	HeScaledFont scaledFont;
	heFontCreateScaled(font, &scaledFont, 15);
	
	heConsoleCreate(font);
	heProfilerCreate(font);
    heUiCreate();
    
	HE_LOG("Set up engine");
	
#if USE_PHYSICS == 0
	freeCam = true;
#endif

#if USE_NETWORKING == 0
    app.state = GAME_STATE_INGAME;
#endif
    
	heGlErrorSaveAll();
	heErrorsPrint();

	heWin32TimerPrint("TOTAL STARTUP");
	HE_DEBUG("Starting game loop");
    double sleepTime = 0.;

    HeUiPanel mainPanel;
    mainPanel.textFields.resize(2);
    HeUiTextField* field     = &mainPanel.textFields[0];
    field->backgroundColour  = hm::colour(40);
    field->shape.size        = hm::vec2f(256, 32);
    field->shape.position    = app.window.windowInfo.size / 2.f;
    field->shape.position.x -= 128;
    field->input.font        = scaledFont;
    field->input.description = "host";
    
    HeUiTextField* field2     = &mainPanel.textFields[1];
    field2->backgroundColour  = hm::colour(40);
    field2->shape.size        = hm::vec2f(256, 32);
    field2->shape.position    = app.window.windowInfo.size / 2.f;
    field2->shape.position.y += 40;
    field2->shape.position.x -= 128;
    field2->input.font        = scaledFont;
    field2->input.description = "name";
    
    
    heUiSetActiveInput(&field->input);

    std::thread thread;
    
	while (!app.window.shouldClose) {
		if (heThreadLoader.updateRequested)
			heThreadLoaderUpdate();

		heProfilerFrameStart();

		heWindowUpdate(&app.window);
		heProfilerFrameMark("window", hm::colour(200, 100, 200));

		heRenderEnginePrepare(&app.engine);
		app.level.time += app.window.frameTime;
		
		{ // input
			if (app.window.active)
				checkWindowInput(&app.window, &app.level.camera, (float) app.window.frameTime);

			if(heWindowKeyWasPressed(&app.window, HE_KEY_F1))
				heConsoleToggleOpen((app.window.keyboardInfo.keyStatus[HE_KEY_LSHIFT]) ? HE_CONSOLE_STATE_OPEN_FULL : HE_CONSOLE_STATE_OPEN);
		}

		heProfilerFrameMark("input", hm::colour(255, 0, 0));

        if(true) {
            if(app.state == GAME_STATE_INGAME) { // d3 rendering
#if USE_PHYSICS == 1
                hePhysicsLevelUpdate(&app.level.physics, (float) app.window.frameTime);
                if(!freeCam)
                    app.level.camera.position = hePhysicsActorGetEyePosition(&app.actor);
                heProfilerFrameMark("physics update", hm::colour(0, 255, 0));
#endif
                app.level.camera.viewMatrix = hm::createViewMatrix(app.level.camera.position, app.level.camera.rotation);
                heRenderEnginePrepareD3(&app.engine);
                heD3LevelUpdate(&app.level);
                heD3LevelRender(&app.engine, &app.level);
                heProfilerFrameMark("d3 render", hm::colour(0, 0, 255));
#if USE_PHYSICS == 1
                hePhysicsLevelDebugDraw(&app.level.physics);
#endif
                heRenderEngineFinishD3(&app.engine);
                heProfilerFrameMark("d3 final", hm::colour(255, 0, 255));

                for(auto const& all : app.players) {
                    all.second.model->transformation.position += all.second.velocity;
                    hm::vec3f playerPosition = all.second.model->transformation.position + hm::vec3f(0, 2.2f, 0);
                    hm::vec3f screenSpace    = heSpaceWorldToScreen(playerPosition, &app.level.camera, &app.window);
                    if(screenSpace.z > 0.f)
                        heUiPushText(&app.engine, &scaledFont, std::string(all.second.name), hm::vec2f(screenSpace.x, screenSpace.y), hm::colour(255, 200, 100), HE_TEXT_ALIGN_CENTER);
                }
            }
				
            { // ui rendering
                heRenderEnginePrepareUi(&app.engine);
                heConsoleRender(&app.engine);

                if(app.state == GAME_STATE_MAIN_MENU) {
#if USE_NETWORKING
                    if(heUiPushButton(&app.engine, &scaledFont, "connect", app.window.windowInfo.size / 2.f + hm::vec2f(-128, 80), hm::vec2f(256, 32))) {
                        app.state = GAME_STATE_INGAME;
                        thread = std::thread(createClient, field->input.string, field2->input.string);
                        heWindowToggleCursor(true);
                        inputActive = true;
                    }
#endif

                    heUiRenderPanel(&app.engine, &mainPanel);
                }
            
                heUiQueueRender(&app.engine);
                heProfilerFrameMark("ui render", hm::colour(255, 255, 0));
                heProfilerAddEntry("sleep", sleepTime, hm::colour(100));
                heProfilerRender(&app.engine);
                heRenderEngineFinishUi(&app.engine);
            }		
        }
        
		heWin32TimerStart();
		heRenderEngineFinish(&app.engine);
		heWindowSyncToFps(&app.window);
		sleepTime = heWin32TimerGet();
		
#if USE_NETWORKING
		if(app.client.socket.status == HN_STATUS_CONNECTED)
			hnClientUpdate(&app.client);
#endif
	}
	
#if USE_NETWORKING
	if(app.client.socket.status == HN_STATUS_CONNECTED)
		hnClientDestroy(&app.client);
        
    if(thread.joinable())
        thread.join();
#endif

	commandThread.detach();
	heRenderEngineDestroy(&app.engine);
	heWindowDestroy(&app.window);
	return 0;
};
/**/
