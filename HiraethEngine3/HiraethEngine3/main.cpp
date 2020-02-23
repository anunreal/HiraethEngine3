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

void createClient() {
    hnConnectClient(&client, "localhost", 9876);

    hnDisconnectClient(&client);
}


int main() {
    HeWindow window;
    HeWindowInfo windowInfo;
    windowInfo.title = L"He3 Test";
    windowInfo.backgroundColour = hm::colour(200, 20, 20);
    windowInfo.fpsCap = 144;
    windowInfo.size = hm::vec2i(1920, 1080);
    window.windowInfo = windowInfo;
    
    heCreateWindow(&window);
    
    
    HeRenderEngine engine;
    heCreateRenderEngine(&engine, &window);
    
    
    HeD3Level level;
    
    HeD3Camera* camera = &level.camera;
    camera->position = hm::vec3f(0, 0, 2);
    camera->rotation = hm::vec3f(0);
    camera->viewMatrix = hm::createViewMatrix(camera->position, camera->rotation);
    camera->projectionMatrix = hm::createPerspectiveProjectionMatrix(90.f, window.windowInfo.size.x / (float)window.windowInfo.size.y, 0.1f, 1000.0f);
    
    heCreatePointLight(&level, hm::vec3f(100, 100, 20), 1.0f, 0.045f, 0.0075f, hm::colour(255, 5.0f));
    
    HeD3Instance* testInstance = &level.instances.emplace_back();
    testInstance->material = heCreatePbrMaterial("testMaterial", heGetTexture("res/textures/models/diffuse.jpg"),
                                                 heGetTexture("res/textures/models/normal.jpg"), heGetTexture("res/textures/models/arm.jpg"));
    testInstance->mesh = heGetMesh("res/models/cerberus.obj");
    testInstance->transformation.scale = hm::vec3f(5);
    
    heBindShader(testInstance->material->shader);
    heLoadShaderUniform(testInstance->material->shader, "u_projMat", camera->projectionMatrix);
    
    heEnableDepth(1);

    createClient();

    while (!window.shouldClose) {
        if (heThreadLoader.updateRequested)
            heUpdateThreadLoader();
        
        heUpdateWindow(&window);
        level.time += window.frameTime;
        
        if (window.active) {
            //if(inputActive)
				//heSetMousePosition(&window, hm::vec2f(-.5f));
            checkWindowInput(&window, camera, (float) window.frameTime);
        }
        
        hePrepareD3RenderEngine(&engine);
        heRenderD3Level(&level);
        heEndD3RenderEngine(&engine);
        
        heSwapWindow(&window);
        heSyncToFps(&window);
    }
    
    heDestroyWindow(&window);
};