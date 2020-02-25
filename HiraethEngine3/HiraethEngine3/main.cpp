#include "src/heWindow.h"
#include "src/heD3.h"
#include "src/heRenderer.h"
#include "GLEW/glew.h"
#include <thread>

#include "hnClient.h"

static bool inputActive = true;
hm::vec2i lastPosition;

const unsigned int SIZEX = 150, SIZEZ = 100;
float heightmap[SIZEX + 1][SIZEZ + 1];

void checkWindowInput(HeWindow* window, HeD3Camera* camera, const float delta) {
    
    if (window->mouseInfo.leftButtonDown)
        inputActive = !inputActive;
    
    if (!inputActive)
        return;
    
    POINT current_mouse_pos = {};
    ::GetCursorPos(&current_mouse_pos);
    hm::vec2i current(current_mouse_pos.x, current_mouse_pos.y);
    hm::vec2f dmouse = hm::vec2f(current - lastPosition) * 0.05f;
    
    heSetMousePosition(window, window->windowInfo.size / 2);
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


HnClient client;
HeD3Level level;
hm::quatf cameraRotation;

void _onClientConnect(HnClient* client, HnLocalClient* local) {
    
    std::cout << "Client connected" << std::endl;
    HeD3Instance* player = &level.instances.emplace_back();
    player->mesh = heGetMesh("res/models/player.obj");
    player->material = heGetMaterial("level");
    hnHookVariable(client, local, "position", &player->transformation.position);
    hnHookVariable(client, local, "rotation", &player->transformation.rotation);
    std::cout << "Handled connection" << std::endl;
    
};

void createClient() {
    
    client.callbacks.clientConnect = &_onClientConnect;
    hnConnectClient(&client, "localhost", 9876);
    hnSyncClient(&client);
    hnCreateVariable(&client, "position", HN_DATA_TYPE_VEC3, 1);
    hnHookVariable(&client, "position", &level.camera.position);
    hnCreateVariable(&client, "rotation", HN_DATA_TYPE_VEC4, 1);
    hnHookVariable(&client, "rotation", &cameraRotation);
    
    while(client.socket.status == HN_STATUS_CONNECTED) {
        //cameraRotation = hm::fromEuler(level.camera.rotation);
        
        hnUpdateClientInput(&client);
        Sleep(16);
    };
    
};




void createWorld(HeWindow* window) {
    
    HeD3Camera* camera = &level.camera;
    camera->position = hm::vec3f(0, 0, 2);
    camera->rotation = hm::vec3f(0);
    camera->viewMatrix = hm::createViewMatrix(camera->position, camera->rotation);
    camera->projectionMatrix = hm::createPerspectiveProjectionMatrix(90.f, window->windowInfo.size.x / (float)window->windowInfo.size.y, 0.1f, 1000.0f);
    
    heCreatePointLight(&level, hm::vec3f(100, 100, 20), 1.0f, 0.045f, 0.0075f, hm::colour(255, 5.0f));
    
    
    HeD3Instance* levelModel = &level.instances.emplace_back();
    //levelModel->material = heCreatePbrMaterial("testMaterial", heGetTexture("res/textures/models/diffuse.jpg"),heGetTexture("res/textures/blank_normal.png"), heGetTexture("res/textures/models/arm.jpg"));
    levelModel->material = heCreatePbrMaterial("level", heGetTexture("res/textures/test/placeholder.png"), nullptr, heGetTexture("res/textures/blankArm.png"));
    levelModel->mesh = heGetMesh("res/models/level.obj");
    levelModel->transformation.scale = hm::vec3f(1);
    
    heBindShader(levelModel->material->shader);
    heLoadShaderUniform(levelModel->material->shader, "u_projMat", camera->projectionMatrix);
    
}


int main() {
    HeWindow window;
    HeWindowInfo windowInfo;
    windowInfo.title            = L"He3 Test";
    windowInfo.backgroundColour = hm::colour(135, 206, 235);
    windowInfo.fpsCap           = 144;
    windowInfo.size             = hm::vec2i(1920, 1080);
    window.windowInfo           = windowInfo;
    
    heCreateWindow(&window);
    
    
    HeRenderEngine engine;
    heCreateRenderEngine(&engine, &window);
    
    createWorld(&window);
    std::thread thread(createClient);
    
    while (!window.shouldClose) {
        if (heThreadLoader.updateRequested)
            heUpdateThreadLoader();
        
        heUpdateWindow(&window);
        level.time += window.frameTime;
        
        if (window.active) {
            //if(inputActive)
				//heSetMousePosition(&window, hm::vec2f(-.5f));
            checkWindowInput(&window, &level.camera, (float) window.frameTime);
        }
        
        hePrepareD3RenderEngine(&engine);
        heRenderD3Level(&level);
        heEndD3RenderEngine(&engine);
        
        heSwapWindow(&window);
        heSyncToFps(&window);
        hnUpdateClientVariables(&client);
    }
    
    thread.detach();
    heDestroyRenderEngine(&engine);
    heDestroyWindow(&window);
};