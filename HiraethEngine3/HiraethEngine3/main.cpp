#include "src/heWindow.h"
#include "src/heD3.h"
#include "src/heRenderer.h"
#include "src/heLoader.h"
#include "src/heCore.h"
#include "src/heDebugUtils.h"
#include "src/hePhysics.h"
#include <windows.h>
#include <thread>
#include "hnClient.h"

static bool inputActive = true;
static bool freeCam = false;
hm::vec2i lastPosition;
hm::vec3f velocity;

void checkWindowInput(HeWindow* window, HeD3Camera* camera, const float delta) {
    
    if (window->mouseInfo.leftButtonDown) {
        inputActive = !inputActive;
        heWindowToggleCursor(inputActive);
    }
    
    if(window->mouseInfo.rightButtonDown)
        freeCam = !freeCam;
    
    /*
    if (!inputActive) {
        velocity = hm::vec3f(0);
        return;
    }
    */
    
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
    
    //camera->position += velocity;
    camera->viewMatrix = hm::createViewMatrix(camera->position, camera->rotation);
    
};


struct Player {
    HnLocalClient* client = nullptr;
    HeD3Instance* model   = nullptr;
    std::string name;
};


HnClient client;
HeD3Level level;
HePhysicsActor actor;
hm::quatf cameraRotation;
std::map<unsigned int, Player> players;


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


void modelStressTest(const std::string& file) {
    const int COUNT = 10;
    
    LARGE_INTEGER frequency;        // ticks per second
    LARGE_INTEGER t1, t2;           // ticks
    double elapsedTime;
    
    // get ticks per second
    QueryPerformanceFrequency(&frequency);
    
    // start timer
    QueryPerformanceCounter(&t1);
    
    for(int i = 0; i < COUNT; ++i) {
        HeVao v;
        heMeshLoad(file + ".obj", &v);
        heVaoDestroy(&v);
    }
    
    // stop timer
    QueryPerformanceCounter(&t2);
    
    // compute and print the elapsed time in millisec
    double elapsedTimeObj = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;
    
    
    
    
    // start timer
    QueryPerformanceCounter(&t1);
    
    HeD3Instance instance;
    
    for(int i = 0; i < COUNT; ++i) {
        heD3InstanceLoad(file + ".h3asset", &instance, nullptr);
        heVaoDestroy(instance.mesh);
    }
    
    // stop timer
    QueryPerformanceCounter(&t2);
    
    // compute and print the elapsed time in millisec
    elapsedTime = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;
    HN_LOG("OBJ loading took: " + std::to_string(elapsedTimeObj) + "ms");
    HN_LOG("h3asset loading took: " + std::to_string(elapsedTime) + "ms");
};


void createWorld(HeWindow* window) {
    
    //modelStressTest("res/assets/stage");
    
    HeD3Camera* camera = &level.camera;
    camera->position = hm::vec3f(0, 1, 0);
    camera->rotation = hm::vec3f(0);
    camera->viewMatrix = hm::createViewMatrix(camera->position, camera->rotation);
    camera->projectionMatrix = hm::createPerspectiveProjectionMatrix(90.f, window->windowInfo.size.x / (float)window->windowInfo.size.y, 0.1f, 1000.0f);
    heD3LevelLoad("res/level/level0.h3level", &level);
    heD3Level = &level;
    
    HE_LOG("Successfully loaded lvl");
    
    HePhysicsShapeInfo actorShape;
    actorShape.type = HE_PHYSICS_SHAPE_CAPSULE;
    actorShape.capsule = hm::vec2f(0.75f, 2.0f);
    
    HePhysicsActorInfo actorInfo;
    
    hePhysicsActorCreate(&actor, actorShape, actorInfo);
    hePhysicsLevelSetActor(&level.physics, &actor);
    hePhysicsActorSetEyePosition(&actor, hm::vec3f(-5.f, 0.1f, 5.f));
    //hePhysicsLevelEnableDebugDraw(&level.physics);
    
};

// TODO(Victor): Multiple shapes per component


#define USE_NETWORKING 0

int main() {
    HeWindow window;
    HeWindowInfo windowInfo;
    windowInfo.title            = L"He3 Test";
    windowInfo.backgroundColour = hm::colour(135, 206, 235);
    windowInfo.fpsCap           = 61;
    windowInfo.size             = hm::vec2i(1366, 768);
    window.windowInfo           = windowInfo;
    
    heWindowCreate(&window);
    heWindowToggleCursor(true);
    
    HeRenderEngine engine;
    heRenderEngineCreate(&engine, &window);
    heRenderEngine = &engine;
    
    createWorld(&window);
    
    std::thread commandThread(heCommandThread, nullptr);
    
#if USE_NETWORKING
    std::thread thread(createClient);
#endif
    
    heGlErrorSaveAll();
    // TODO(Victor): Check for possible errors during setup and print them
    
    while (!window.shouldClose) {
        if (heThreadLoader.updateRequested)
            heThreadLoaderUpdate();
        
        heWindowUpdate(&window);
        heRenderEnginePrepare(&engine);
        level.time += window.frameTime;
        
        { // input
            if (window.active)
                checkWindowInput(&window, &level.camera, (float) window.frameTime);
            
            hm::vec3f v = velocity * (float) window.frameTime * 30.f * (window.keyboardInfo.keyStatus[HE_KEY_LSHIFT] ? 2.f : 1.f);
            if(!freeCam)
                hePhysicsActorSetVelocity(&actor, v);
            else {
                hePhysicsActorSetVelocity(&actor, hm::vec3f(0));
                level.camera.position += v;
			}
            
            if(heWindowKeyWasPressed(&window, HE_KEY_SPACE) && hePhysicsActorOnGround(&actor))
                hePhysicsActorJump(&actor, 1);
            
            hePhysicsLevelUpdate(&level.physics, (float) window.frameTime);
            if(!freeCam)
                level.camera.position = hePhysicsActorGetEyePosition(&actor);
        }
        
        { // d3 rendering
            heRenderEnginePrepareD3(&engine);
            heD3LevelUpdate(&level);
            heD3LevelRender(&engine, &level);
            hePhysicsLevelDebugDraw(&level.physics);
            heRenderEngineFinishD3(&engine);
        }
        
        if(true) { // ui rendering
            // TODO(Victor): check if there is any ui content to render
            heRenderEnginePrepareUi(&engine);
            heUiQueueRender(&engine);
            heRenderEngineFinishUi(&engine);
        }
        
        heRenderEngineFinish(&engine);
        heWindowSyncToFps(&window);
        
#if USE_NETWORKING
        hnClientUpdateVariables(&client);
#endif
        
        heGlErrorSaveAll();
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