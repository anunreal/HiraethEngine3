#include "main.h"
#include "src/heWin32Layer.h"
#include "src/heConsole.h"
#include "src/heLoader.h"
#include "src/heCore.h"
#include "src/heDebugUtils.h"
#include <windows.h>
#include <thread>
#include <vector>

bool						   inputActive = true;
bool						   freeCam = false;
hm::vec2i					   lastPosition;
hm::vec3f					   velocity;
hm::quatf					   cameraRotation;

App app;

void checkWindowInput(HeWindow* window, HeD3Camera* camera, float const delta) {
	if (window->mouseInfo.leftButtonDown) {
		inputActive = !inputActive;
		heWindowToggleCursor(inputActive);
	}
	
	if(window->mouseInfo.rightButtonDown)
		freeCam = !freeCam;
	
	if(inputActive) {
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
	Player* p = &app.players[local->id];
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
	Player* p = &app.players[local->id];
	heD3LevelRemoveInstance(&app.level, p->model);
	app.players.erase(local->id);
};

void createClient() {
	strcpy_s(app.ownName, "player");
	
	app.client.callbacks.clientConnect = &_onClientConnect;
	app.client.callbacks.clientDisconnect = &_onClientDisconnect;
	//hnClientConnect(&app.client, "tealfire.de", 9876, HN_PROTOCOL_TCP);
	hnClientConnect(&app.client, "localhost", 9876, HN_PROTOCOL_UDP);

	hnClientSync(&app.client);

	if(app.client.socket.status != HN_STATUS_CONNECTED)
		return;

	hnClientCreateVariable(&app.client, "position", HN_DATA_TYPE_VEC3, 10);
	hnClientHookVariable(  &app.client, "position", &app.level.physics.actor->feetPosition);
	hnClientCreateVariable(&app.client, "velocity", HN_DATA_TYPE_VEC3, 2);
	hnClientHookVariable(  &app.client, "velocity", &app.level.physics.actor->velocity);
	hnClientCreateVariable(&app.client, "rotation", HN_DATA_TYPE_VEC4, 10);
	hnClientHookVariable(  &app.client, "rotation", &cameraRotation);
	hnClientCreateVariable(&app.client, "name",     HN_DATA_TYPE_STRING, 60, 256);
	hnClientHookVariable(  &app.client, "name",     &app.ownName);
	hnClientUpdateVariable(&app.client, "name");
	
	while(app.client.socket.status == HN_STATUS_CONNECTED) {
		cameraRotation = hm::fromEulerDegrees(-app.level.camera.rotation);
		hnClientUpdateInput(&app.client);
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
	
	HeD3Camera* camera = &app.level.camera;
	camera->position = hm::vec3f(0, 1, 0);
	camera->rotation = hm::vec3f(0);
	camera->viewMatrix = hm::createViewMatrix(camera->position, camera->rotation);
	camera->projectionMatrix = hm::createPerspectiveProjectionMatrix(90.f, window->windowInfo.size.x / (float)window->windowInfo.size.y, 0.1f, 1000.0f);
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
	heWindowToggleCursor(true);

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

	HE_LOG("Set up engine");
	
#if USE_NETWORKING
	std::thread thread(createClient);
#endif
	
#if USE_PHYSICS == 0
	freeCam = true;
#endif
	
	heGlErrorSaveAll();
	heErrorsPrint();

	heWin32TimerPrint("TOTAL STARTUP");
	HE_DEBUG("Starting game loop");

	double sleepTime = 0;
	
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
		
		{ // d3 rendering
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
				all.second.model->transformation.position += all.second.velocity /* * app.window.frameTime */;
				hm::vec3f playerPosition = all.second.model->transformation.position + hm::vec3f(0, 2.2f, 0);
				hm::vec3f screenSpace    = heSpaceWorldToScreen(playerPosition, &app.level.camera, &app.window);
				if(screenSpace.z > 0.f)
					heUiPushText(&app.engine, &scaledFont, std::string(all.second.name), hm::vec2f(screenSpace.x, screenSpace.y), hm::colour(255, 200, 100), HE_TEXT_ALIGN_CENTER);
			}
		}
				
		{ // ui rendering
			heRenderEnginePrepareUi(&app.engine);
			heConsoleRender((float) app.window.frameTime);
			heUiQueueRender(&app.engine);
			heProfilerFrameMark("ui render", hm::colour(255, 255, 0));
			heProfilerAddEntry("sleep", sleepTime, hm::colour(100));
			heProfilerRender(&app.engine);
			heRenderEngineFinishUi(&app.engine);
		}		
		
		heWin32TimerStart();
		heRenderEngineFinish(&app.engine);
		heWindowSyncToFps(&app.window);
		sleepTime = heWin32TimerGet();
		
#if USE_NETWORKING
		if(app.client.socket.status == HN_STATUS_CONNECTED)
			hnClientUpdateVariables(&app.client);
#endif
	}
	
#if USE_NETWORKING
	if(app.client.socket.status == HN_STATUS_CONNECTED) {
		hnClientDisconnect(&app.client);
	}
	thread.join();
#endif

	commandThread.detach();
	heRenderEngineDestroy(&app.engine);
	heWindowDestroy(&app.window);
	return 0;
};
