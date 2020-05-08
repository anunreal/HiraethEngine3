#include "src/heWindow.h"
#include "src/heD3.h"
#include "src/heRenderer.h"
#include "src/heLoader.h"
#include "src/heCore.h"
#include "src/heDebugUtils.h"
#include "src/hePhysics.h"
#include "src/heBinary.h"
#include "src/heWin32Layer.h"
#include "src/heConsole.h"
#include <windows.h>
#include <thread>
#include <vector>
#include "hnClient.h"

struct Player {
	HnLocalClient* client = nullptr;
	HeD3Instance* model	  = nullptr;
	std::string name;
};

bool						   inputActive = true;
bool						   freeCam = false;
hm::vec2i					   lastPosition;
hm::vec3f					   velocity;
HnClient					   client;
HeD3Level					   level;
HePhysicsActor				   actor;
hm::quatf					   cameraRotation;
std::map<unsigned int, Player> players;

#define USE_PHYSICS 1
#define USE_NETWORKING 0

void checkWindowInput(HeWindow* window, HeD3Camera* camera, const float delta) {
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

	hm::vec3f v = velocity * (float) window->frameTime * 30.f * (window->keyboardInfo.keyStatus[HE_KEY_LSHIFT] ? 2.f : 1.f);
	if(!freeCam) {
		hePhysicsActorSetVelocity(&actor, v);
		if(heWindowKeyWasPressed(window, HE_KEY_SPACE) && hePhysicsActorOnGround(&actor))
			hePhysicsActorJump(&actor);
	} else {
		hePhysicsActorSetVelocity(&actor, hm::vec3f(0));
		camera->position += v;
	}
};

void _onClientConnect(HnClient* client, HnLocalClient* local) {
	HeD3Instance* instance = &level.instances.emplace_back();
	instance->mesh = heAssetPoolGetMesh("res/models/player.obj");
	instance->material = heAssetPoolGetMaterial("level");
	hnLocalClientHookVariable(client, local, "position", &instance->transformation.position);
	hnLocalClientHookVariable(client, local, "rotation", &instance->transformation.rotation);
	
	Player* p = &players[local->id];
	p->client = local;
	p->model = instance;
};

void _onClientDisconnect(HnClient* client, HnLocalClient* local) {
	Player* p = &players[local->id];
	heD3LevelRemoveInstance(&level, p->model);
};

void createClient() {
	client.callbacks.clientConnect = &_onClientConnect;
	client.callbacks.clientDisconnect = &_onClientDisconnect;
	//hnConnectClient(&client, "tealfire.de", 9876);
	hnClientConnect(&client, "localhost", 9876, HN_PROTOCOL_TCP);
	hnClientSync(&client);
	hnClientCreateVariable(&client, "position", HN_DATA_TYPE_VEC3, 1);
	hnClientHookVariable(&client, "position", &level.camera.position);
	hnClientCreateVariable(&client, "rotation", HN_DATA_TYPE_VEC4, 1);
	hnClientHookVariable(&client, "rotation", &cameraRotation);
	
	while(client.socket.status == HN_STATUS_CONNECTED) {
		cameraRotation = hm::fromEulerDegrees(-level.camera.rotation);
		hnClientUpdateInput(&client);
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
	
	HeD3Camera* camera = &level.camera;
	camera->position = hm::vec3f(0, 1, 0);
	camera->rotation = hm::vec3f(0);
	camera->viewMatrix = hm::createViewMatrix(camera->position, camera->rotation);
	camera->projectionMatrix = hm::createPerspectiveProjectionMatrix(90.f, window->windowInfo.size.x / (float)window->windowInfo.size.y, 0.1f, 1000.0f);
	heWin32TimerStart();
	heD3LevelLoad("res/level/level0.h3level", &level, USE_PHYSICS);

	// temporary: bloom test
	for(HeD3Instance& all : level.instances) {
		if(all.name == "Suzanne.h3asset")
			all.material->emission = hm::colour(255, 100, 100, 255, 10.f);
	}
		

	heWin32TimerPrint("LEVEL LOAD");
	
	heD3SkyboxCreate(&level.skybox, "res/textures/hdr/pink_sunrise.hdr");
	heD3Level = &level;
	
	HE_LOG("Successfully loaded lvl");
	
#if USE_PHYSICS == 1
	HePhysicsShapeInfo actorShape;
	actorShape.type = HE_PHYSICS_SHAPE_CAPSULE;
	actorShape.capsule = hm::vec2f(0.75f, 2.0f);
	
	HePhysicsActorInfo actorInfo;
	
	hePhysicsActorCreate(&actor, actorShape, actorInfo);
	hePhysicsLevelSetActor(&level.physics, &actor);
	hePhysicsActorSetEyePosition(&actor, hm::vec3f(-5.f, 0.1f, 5.f));
#endif
};

// TODO(Victor): Multiple shapes per component

int main() {
	heWin32TimerStart();
	std::thread commandThread(heCommandThread, nullptr);
	
	HeWindow window;
	HeWindowInfo windowInfo;
	windowInfo.title			= L"He3 Test";
	windowInfo.backgroundColour = hm::colour(135, 206, 235);
	windowInfo.fpsCap			= 61;
	windowInfo.size				= hm::vec2i(1366, 768);
	window.windowInfo			= windowInfo;
	
	heWindowCreate(&window);
	heWindowToggleCursor(true);

	heGlPrintInfo();
	
	HeRenderEngine engine;
	heRenderEngineCreate(&engine, &window);
	hePostProcessEngineCreate(&engine.postProcess, &window);
	heRenderEngine = &engine;

	createWorld(&window);

	HeFont* font = heAssetPoolGetFont("inconsolata");
	heConsoleCreate(font);
	heFontCreateScaled(font, &heProfiler.font, 10);
	
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
	
	while (!window.shouldClose) {
		if (heThreadLoader.updateRequested)
			heThreadLoaderUpdate();

		heProfilerFrameStart();
		
		heWindowUpdate(&window);

		heRenderEnginePrepare(&engine);
		level.time += window.frameTime;

		heProfilerFrameMark("window", hm::colour(200, 100, 200));

		{ // input
			if (window.active)
				checkWindowInput(&window, &level.camera, (float) window.frameTime);

			if(heWindowKeyWasPressed(&window, HE_KEY_F1))
				heConsoleToggleOpen((window.keyboardInfo.keyStatus[HE_KEY_LSHIFT]) ? HE_CONSOLE_STATE_OPEN_FULL : HE_CONSOLE_STATE_OPEN);
		}

		heProfilerFrameMark("input", hm::colour(255, 0, 0));

		
		{ // d3 rendering
#if USE_PHYSICS == 1
			hePhysicsLevelUpdate(&level.physics, (float) window.frameTime);
			if(!freeCam)
				level.camera.position = hePhysicsActorGetEyePosition(&actor);
			heProfilerFrameMark("physics update", hm::colour(0, 255, 0));
#endif
			level.camera.viewMatrix = hm::createViewMatrix(level.camera.position, level.camera.rotation);
			heRenderEnginePrepareD3(&engine);
			heD3LevelUpdate(&level);
			heD3LevelRender(&engine, &level);
			heProfilerFrameMark("d3 render", hm::colour(0, 0, 255));
#if USE_PHYSICS == 1
			hePhysicsLevelDebugDraw(&level.physics);
#endif
			heRenderEngineFinishD3(&engine);
			heProfilerFrameMark("d3 final", hm::colour(255, 0, 255));
		}
				
		{ // ui rendering
			heRenderEnginePrepareUi(&engine);
			heConsoleRender((float) window.frameTime);
			heUiQueueRender(&engine);
			heProfilerFrameMark("ui render", hm::colour(255, 255, 0));
			heProfilerAddEntry("sleep", sleepTime, hm::colour(100));
			heProfilerRender(&engine);
			heRenderEngineFinishUi(&engine);
		}
		
		
		heWin32TimerStart();
		heRenderEngineFinish(&engine);
		heWindowSyncToFps(&window);
		sleepTime = heWin32TimerGet();
		
#if USE_NETWORKING
		hnClientUpdateVariables(&client);
#endif
	}
	
#if USE_NETWORKING
	hnClientDisconnect(&client);
	thread.join();
#endif
	commandThread.detach();
	heRenderEngineDestroy(&engine);
	heWindowDestroy(&window);
	return 0;
};
