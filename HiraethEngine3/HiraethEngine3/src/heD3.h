#pragma once
#include "heAssets.h"

struct HeD3Transformation {
	hm::vec3f position;
	hm::vec3f scale;
	hm::quatf rotation;
};

struct HeMaterial {
	HeShaderProgram* shader = nullptr;
	HeTexture* diffuseTexture = nullptr;
};

struct HeD3Instance {
	HeVao* mesh;
	HeMaterial material;
	HeD3Transformation transformation;
};