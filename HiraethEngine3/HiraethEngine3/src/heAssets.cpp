#include "heAssets.h"

HeAssetPool heAssetPool;
HeThreadLoader heThreadLoader;


// --- Assets

HeVao* heGetMesh(const std::string& file) {

	auto it = heAssetPool.meshPool.find(file);
	if (it != heAssetPool.meshPool.end())
		return &it->second;

	// load model
	return nullptr;

};

HeTexture* heGetTexture(const std::string& file) {

	auto it = heAssetPool.texturePool.find(file);
	if (it != heAssetPool.texturePool.end())
		return &it->second;

	HeTexture* t = &heAssetPool.texturePool[file];
	heLoadTexture(t, file);
	return t;

};

HeShaderProgram* heGetShader(const std::string& name) {

	auto it = heAssetPool.shaderPool.find(name);
	if (it != heAssetPool.shaderPool.end())
		return &it->second;

	HeShaderProgram* s = &heAssetPool.shaderPool[name];
	heLoadShader(s, "res/shaders/" + name + "_v.glsl", "res/shaders/" + name + "_f.glsl");
	return s;

};


// --- ThreadLoader

void heRequestTexture(HeTexture* texture, unsigned char* buffer) {

	heThreadLoader.textures[texture] = buffer;
	heThreadLoader.updateRequested = true;

};

void heUpdateThreadLoader() {

	for (auto& all : heThreadLoader.textures)
		heCreateTexture(all.second, &all.first->textureId, all.first->width, all.first->height, all.first->channels);
	
	heThreadLoader.textures.clear();
	heThreadLoader.updateRequested = false;

};