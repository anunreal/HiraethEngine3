#include "heWindow.h"
#include "heGlLayer.h"
#include "heAssets.h"
#include <thread>


void load_world() {
	std::cout << "Loading..." << std::endl;

	std::cout << "Done" << std::endl;
}




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
	heAddVaoData(&vao, { -1, 1, 1, 1, -1, -1, -1, -1, 1, 1, 1, -1 }, 2);
	heBindVao(&vao);

	HeShaderProgram* shader = heGetShader("def");
	heBindShader(shader);
	heGetShaderSamplerLocation(shader, "u_texture", 0);

	HeTexture* tex = heGetTexture("res/textures/test.JPG");
	heBindTexture(tex, 0);

	while (!window.shouldClose) {
		if (heThreadLoader.updateRequested)
			heUpdateThreadLoader();

		heUpdateWindow(&window);

		heRenderVao(&vao);

		heSwapWindow(&window);
		heSyncToFps(&window);
	}

	heDestroyTexture(tex);
	heDestroyShader(shader);
	heDestroyVao(&vao);
	heDestroyWindow(&window);
}