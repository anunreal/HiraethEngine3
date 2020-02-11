#include "heWindow.h"
#include "heD3.h"
#include "heRenderer.h"
#include "GLEW/glew.h"
#include <thread>

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
    
}

void fillHeightmap() {
    for (int x = 0; x <= SIZEX; ++x) {
        for (int z = 0; z <= SIZEZ; ++z) {
            heightmap[x][z] = (std::rand() / (float)RAND_MAX) * 2.f;
        }
    }
}

void createTestMesh() {
    std::vector<float> vertices;
    std::vector<float> uvs;
    std::vector<float> normals;
    
    vertices.reserve(SIZEZ * SIZEX);
    
    for (int x = 0; x < SIZEX; ++x) {
        for (int z = 0; z < SIZEZ; ++z) {
            float fx = (float)x;
            float fz = (float)z;
            hm::vec3f points[4];
            points[0] = hm::vec3f(fx,     heightmap[x][z],         fz);
            points[1] = hm::vec3f(fx + 1, heightmap[x + 1][z],     fz);
            points[2] = hm::vec3f(fx,     heightmap[x][z + 1],     fz + 1);
            points[3] = hm::vec3f(fx + 1, heightmap[x + 1][z + 1], fz + 1);
            
            vertices.emplace_back(points[0].x);
            vertices.emplace_back(points[0].y);
            vertices.emplace_back(points[0].z);
            
            vertices.emplace_back(points[1].x);
            vertices.emplace_back(points[1].y);
            vertices.emplace_back(points[1].z);
            
            vertices.emplace_back(points[2].x);
            vertices.emplace_back(points[2].y);
            vertices.emplace_back(points[2].z);
            
            vertices.emplace_back(points[2].x);
            vertices.emplace_back(points[2].y);
            vertices.emplace_back(points[2].z);
            
            vertices.emplace_back(points[3].x);
            vertices.emplace_back(points[3].y);
            vertices.emplace_back(points[3].z);
            
            vertices.emplace_back(points[1].x);
            vertices.emplace_back(points[1].y);
            vertices.emplace_back(points[1].z);
            
            uvs.emplace_back((float)x / SIZEX);
            uvs.emplace_back((float)z / SIZEZ);
            
            uvs.emplace_back((float)(x + 1) / SIZEX);
            uvs.emplace_back((float)z / SIZEZ);
            
            uvs.emplace_back((float)x / SIZEX);
            uvs.emplace_back((float)(z + 1) / SIZEZ);
            
            uvs.emplace_back((float)x / SIZEX);
            uvs.emplace_back((float)(z + 1) / SIZEZ);
            
            uvs.emplace_back((float)(x + 1) / SIZEX);
            uvs.emplace_back((float)(z + 1) / SIZEZ);
            
            uvs.emplace_back((float)(x + 1) / SIZEX);
            uvs.emplace_back((float)z / SIZEZ);
            
            hm::vec3f normal1 = hm::cross(points[1] - points[0], points[2] - points[0]);
            normal1 = hm::normalize(normal1);
            
            hm::vec3f normal2 = hm::cross(points[3] - points[2], points[1] - points[2]);
            normal2 = hm::normalize(-normal2);
            
            normals.emplace_back(normal1.x);
            normals.emplace_back(normal1.y);
            normals.emplace_back(normal1.z);
            
            normals.emplace_back(normal1.x);
            normals.emplace_back(normal1.y);
            normals.emplace_back(normal1.z);
            
            normals.emplace_back(normal1.x);
            normals.emplace_back(normal1.y);
            normals.emplace_back(normal1.z);
            
            normals.emplace_back(normal2.x);
            normals.emplace_back(normal2.y);
            normals.emplace_back(normal2.z);
            
            normals.emplace_back(normal2.x);
            normals.emplace_back(normal2.y);
            normals.emplace_back(normal2.z);
            
            normals.emplace_back(normal2.x);
            normals.emplace_back(normal2.y);
            normals.emplace_back(normal2.z);
        }
    }
    
    HeVao* vao = &heAssetPool.meshPool["testMesh"];
    heCreateVao(vao);
    heBindVao(vao);
    heAddVaoData(vao, vertices, 3);
    heAddVaoData(vao, uvs, 2);
    heAddVaoData(vao, normals, 3);
    
}

int main() {
    HeWindow window;
    HeWindowInfo windowInfo;
    windowInfo.title = L"He3 Test";
    windowInfo.backgroundColour = hm::colour(20, 20, 20);
    windowInfo.fpsCap = 144;
    windowInfo.size = hm::vec2i(1366, 768);
    window.windowInfo = windowInfo;
    
    heCreateWindow(&window);
    
    
    HeRenderEngine engine;
    heCreateRenderEngine(&engine, &window);
    
    
    HeD3Level level;
    
    HeD3Camera* camera = &level.camera;
    camera->position = hm::vec3f(0, 2, 0);
    camera->rotation = hm::vec3f(90, 90, 0);
    camera->viewMatrix = hm::createViewMatrix(camera->position, camera->rotation);
    camera->projectionMatrix = hm::createPerspectiveProjectionMatrix(90.f, window.windowInfo.size.x / (float)window.windowInfo.size.y, 0.1f, 1000.0f);
    
    heEnableDepth(1);
    fillHeightmap();
    createTestMesh();
    
    HeD3Light* testLight = &level.lights.emplace_back();
    testLight->colour  = hm::colour(255);
    testLight->type    = HE_LIGHT_TYPE_POINT;
    testLight->vector  = hm::vec3f(0, 100, 0);
    testLight->data[0] = 1.0f;
    testLight->data[1] = 0.045f;
    testLight->data[2] = 0.0075f;
    testLight->update  = true;
    
    HeD3Instance* testInstance = &level.instances.emplace_back();
    testInstance->material = heCreatePbrMaterial("testMaterial", heGetTexture("res/textures/test/placeHolder.png"),
                                                 heGetTexture("res/textures/models/normal.jpg"), heGetTexture("res/textures/models/arm.jpg"));
    
    
    heBindShader(testInstance->material->shader);
    heLoadShaderUniform(testInstance->material->shader, "u_projMat", camera->projectionMatrix);
    
    testInstance->mesh = heGetMesh("res/models/camp_fire.obj");
    testInstance->transformation.scale = hm::vec3f(5);
    
    HeTexture* diffuse = heGetTexture("res/textures/test/placeHolder.png");
    heBindTexture(diffuse, 0);
    heTextureClampRepeat();
    heTextureFilterTrilinear();
    heTextureFilterAnisotropic();
    
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
        
        //heRenderD3Instance(testInstance, camera, level.time);
        heRenderD3Level(&level);
        
        heSwapWindow(&window);
        heSyncToFps(&window);
    }
    
    heDestroyWindow(&window);
}