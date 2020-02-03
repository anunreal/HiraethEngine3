#include "heWindow.h"
#include "heD3.h"
#include "heRenderer.h"
#include "GLEW/glew.h"
#include <thread>

static bool inputActive = true;
hm::vec2f lastPosition;

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
	lastPosition.x = current_mouse_pos.x;
	lastPosition.y = current_mouse_pos.y;
	
	camera->rotation.x += dmouse.x;
	camera->rotation.y += dmouse.y;


	hm::vec3f velocity;
	float angle = hm::to_radians(camera->rotation.y);
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

	if (window->keyboardInfo.keyStatus[HE_KEY_LSHIFT])
		velocity.y = -speed;

	camera->position += velocity;
	camera->viewMatrix = hm::createViewMatrix(camera->position, camera->rotation);

}

int main() {
	HeWindow window;
	HeWindowInfo windowInfo;
	windowInfo.title = L"He3 Test";
	windowInfo.backgroundColour = hm::colour(20, 20, 20);
	windowInfo.fpsCap = 144;
	windowInfo.size = hm::vec2i(1336, 768);
	window.windowInfo = windowInfo;

	heCreateWindow(&window);

	const float y = -.2f;

	HeVao vao;
	heCreateVao(&vao);
	heBindVao(&vao);
	heAddVaoData(&vao, { -1, y, 1,  1, y, 1,  -1, y, -1,  -1, y, -1,  1, y, 1,  1, y, -1 }, 3);
	heBindVao(&vao);

	HeShaderProgram* shader = heGetShader("def");
	heBindShader(shader);
	heGetShaderSamplerLocation(shader, "u_texture", 0);

	HeTexture* tex = heGetTexture("res/textures/test.JPG");
	heBindTexture(tex, 0);

	HeD3Camera camera;
	camera.position = hm::vec3f(0, 2, 0);
	camera.rotation = hm::vec3f(90, 90, 0);
	camera.viewMatrix = hm::createViewMatrix(camera.position, camera.rotation);
	camera.projectionMatrix = hm::createPerspectiveProjectionMatrix(90.f, window.windowInfo.size.x / (float)window.windowInfo.size.y, 0.1f, 1000.0f);

	HeD3Instance instance;
	instance.mesh = &vao;
	instance.material.shader = shader;
	instance.material.diffuseTexture = tex;


	while (!window.shouldClose) {
		if (heThreadLoader.updateRequested)
			heUpdateThreadLoader();

		heUpdateWindow(&window);

		if (window.active) {
			//if(inputActive)
				//heSetMousePosition(&window, hm::vec2f(-.5f));
			checkWindowInput(&window, &camera, window.frameTime);
		}
		
		heRenderD3Instance(&instance, &camera);

		heSwapWindow(&window);
		heSyncToFps(&window);
	}

	heDestroyTexture(tex);
	heDestroyShader(shader);
	heDestroyVao(&vao);
	heDestroyWindow(&window);
}