#include "heWindow.h"
#include "heGlLayer.h"

int main() {
	HeWindow window;
	HeWindowInfo windowInfo;
	windowInfo.title = L"He3 Test";
	windowInfo.backgroundColour = hm::colour(255);
	windowInfo.fpsCap = 60;
	windowInfo.size = hm::vec2i(1336, 768);
	window.windowInfo = windowInfo;

	heCreateWindow(&window);

	HeVao vao;
	heCreateVao(&vao);
	heBindVao(&vao);
	heAddVaoData(&vao, { -1, 1, 1, 1, -1, -1 }, 2);
	heBindVao(&vao);

	HeShaderProgram shader;
	heLoadShader(&shader, "res/shaders/def_v.glsl", "res/shaders/def_f.glsl");
	heBindShader(&shader);

	while (!window.shouldClose) {
		heUpdateWindow(&window);
		
		heRenderVao(&vao);

		heSwapWindow(&window);
		heSyncToFps(&window);
	}

	heDestroyWindow(&window);
}