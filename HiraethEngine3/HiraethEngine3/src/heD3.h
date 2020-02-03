#pragma once
#include "heAssets.h"

struct HeD3Transformation {
	hm::vec3f position;
	hm::vec3f scale;
	hm::quatf rotation;

	HeD3Transformation() : position(0.f), scale(1.f), rotation(0.f, 0.f, 0.f, 1.f) {};
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

struct HeD3Camera {
	hm::vec3f position;
	hm::vec3f rotation;
	hm::mat4f viewMatrix;
	hm::mat4f projectionMatrix;
};