#include "src/heWindow.h"
#include "src/heD3.h"
#include "src/heRenderer.h"
#include "src/heLoader.h"
#include "GLEW/glew.h"
#include <thread>

#include "hnClient.h"

static bool inputActive = true;
hm::vec2i lastPosition;

void checkWindowInput(HeWindow* window, HeD3Camera* camera, const float delta) {
    
    if (window->mouseInfo.leftButtonDown) {
        inputActive = !inputActive;
        heWindowToggleCursor(inputActive);
    }
    
    if (!inputActive)
        return;
    
    POINT current_mouse_pos = {};
    ::GetCursorPos(&current_mouse_pos);
    hm::vec2i current(current_mouse_pos.x, current_mouse_pos.y);
    hm::vec2f dmouse = hm::vec2f(current - lastPosition) * 0.05f;
    
    heWindowSetMousePosition(window, window->windowInfo.size / 2);
    ::GetCursorPos(&current_mouse_pos);
    lastPosition.x = (int) current_mouse_pos.x;
    lastPosition.y = (int) current_mouse_pos.y;
    
    camera->rotation.x += dmouse.y;
    camera->rotation.y += dmouse.x;
    
    
    hm::vec3f velocity;
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
    
    if (window->keyboardInfo.keyStatus[HE_KEY_SPACE])
        velocity.y = speed;
    
    if (window->keyboardInfo.keyStatus[HE_KEY_Q])
        velocity.y = -speed;
    
    camera->position += velocity;
    camera->viewMatrix = hm::createViewMatrix(camera->position, camera->rotation);
    
};


struct Player {
    HnLocalClient* client = nullptr;
    HeD3Instance* model   = nullptr;
    std::string name;
};


HnClient client;
HeD3Level level;
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
    LARGE_INTEGER frequency;        // ticks per second
    LARGE_INTEGER t1, t2;           // ticks
    double elapsedTime;
    
    // get ticks per second
    QueryPerformanceFrequency(&frequency);
    
    // start timer
    QueryPerformanceCounter(&t1);
    
    for(int i = 0; i < 1000; ++i) {
        heMeshLoad(file);
    }
    
    // stop timer
    QueryPerformanceCounter(&t2);
    
    // compute and print the elapsed time in millisec
    elapsedTime = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;
    HN_LOG("OBJ loading took: " + std::to_string(elapsedTime) + "ms");
    
    
    
    
    // start timer
    QueryPerformanceCounter(&t1);
    
    HeD3Instance instance;
    
    for(int i = 0; i < 1000; ++i) {
        heD3InstanceLoad(file, &instance);
    }
    
    // stop timer
    QueryPerformanceCounter(&t2);
    
    // compute and print the elapsed time in millisec
    elapsedTime = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;
    HN_LOG("h3asset loading took: " + std::to_string(elapsedTime) + "ms");
};


void createWorld(HeWindow* window) {
    
    HeD3Camera* camera = &level.camera;
    camera->position = hm::vec3f(0, 1, 0);
    camera->rotation = hm::vec3f(0);
    camera->viewMatrix = hm::createViewMatrix(camera->position, camera->rotation);
    camera->projectionMatrix = hm::createPerspectiveProjectionMatrix(90.f, window->windowInfo.size.x / (float)window->windowInfo.size.y, 0.1f, 1000.0f);
    
    heD3LightSourceCreatePoint(&level, hm::vec3f(100, 100, 20), 1.0f, 0.045f, 0.0075f, hm::colour(255, 5.0f));
    heD3LightSourceCreatePoint(&level, hm::vec3f(-5, 6, 7), 1.0f, 0.045f, 0.0075f, hm::colour(255, 0, 0, 2.0f));
    
    if(false) {
        HeD3Instance* levelModel = &level.instances.emplace_back();
        levelModel->material = heMaterialCreatePbr("level", heAssetPoolGetTexture("res/textures/test/placeholder.png"), nullptr, heAssetPoolGetTexture("res/textures/blankArm.png"));
        levelModel->mesh = heAssetPoolGetMesh("res/models/level.obj");
        levelModel->transformation.scale = hm::vec3f(1);
        
        HeD3Instance* lampModel = &level.instances.emplace_back();
        lampModel->transformation.position = hm::vec3f(-5, 0, 7);
        lampModel->mesh = heAssetPoolGetMesh("res/models/lamp.obj");
        lampModel->material = heAssetPoolGetMaterial("level");
        
        HeD3Instance* thingyModel = &level.instances.emplace_back();
        thingyModel->transformation.position = hm::vec3f(-3, 1, -5);
        heD3InstanceLoad("res/assets/road.h3asset", thingyModel);
    } else {
        heD3LevelLoad("res/level/level0.h3level", &level);
    }
    
};

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
    
    createWorld(&window);
    
#if USE_NETWORKING
    std::thread thread(createClient);
#endif
    
    while (!window.shouldClose) {
        if (heThreadLoader.updateRequested)
            heThreadLoaderUpdate();
        
        heWindowUpdate(&window);
        level.time += window.frameTime;
        
        if (window.active)
            checkWindowInput(&window, &level.camera, (float) window.frameTime);
        
        heRenderEnginePrepareD3(&engine);
        heD3LevelRender(&level);
        heRenderEngineFinishD3(&engine);
        
        heWindowSwapBuffers(&window);
        heWindowSyncToFps(&window);
#if USE_NETWORKING
        hnClientUpdateVariables(&client);
#endif
    }
    
#if USE_NETWORKING
    hnClientDisconnect(&client);
    thread.join();
#endif
    heRenderEngineDestroy(&engine);
    heWindowDestroy(&window);
    return 0;
};