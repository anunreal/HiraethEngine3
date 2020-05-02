#include "heAssets.h"
#include "heLoader.h"
#include "heWindow.h"
#include "heUtils.h"
#include "heCore.h"
#include <fstream>

HeAssetPool heAssetPool;
HeThreadLoader heThreadLoader;
HeMemoryTracker heMemoryTracker;

std::unordered_map<std::string, uint32_t> shaderTypeIds;
uint32_t shaderTypeCounter = 0;


// -- Materials

HeMaterial* heMaterialCreatePbr(std::string const& name, HeTexture* diffuseTexture, HeTexture* normalTexture, HeTexture* armTexture) {
        HeMaterial* mat = heAssetPoolGetNewMaterial(name);
    mat->shader = heAssetPoolGetShader("3d_pbr");
    mat->textures["diffuse"] = diffuseTexture;
    mat->textures["normal"]  = normalTexture;
    mat->textures["arm"]     = armTexture;
    mat->type                = heMaterialGetType("3d_pbr");
    return mat;
};

uint32_t heMaterialGetType(std::string const& shaderName) {    
    auto it = shaderTypeIds.find(shaderName);
    if(it != shaderTypeIds.end())
        return it->second;
    
    uint32_t id = ++shaderTypeCounter;
    shaderTypeIds[shaderName] = id;
    return id;   
};


// -- font

std::unordered_map<std::string, std::string> getLineArguments(std::string const& line) {
	std::vector<std::string> entries = heStringSplit(line, ' ');
	std::unordered_map<std::string, std::string> out;

	for(std::string const& entry : entries) {
		size_t index = entry.find('=');
		if(index != std::string::npos)
		    out[entry.substr(0, index)] = entry.substr(index + 1);
	}
	
	return out;	
};

void heFontLoad(HeFont* font, std::string const& name) {
	std::ifstream stream("res/fonts/" + name + ".fnt");
	if(!stream) {
		HE_ERROR("Could not open font file [" + name + "]");
		return;
	}

#ifdef HE_ENABLE_NAMES
    font->name = name;
#endif
	
	std::string line;
	std::getline(stream, line);
	// parse basic info
	auto info     = getLineArguments(line); // info
    //font->size    = std::stoi(info["size"]);
	font->padding = hm::vec4<uint8_t>(info["padding"][0] - '0',
									  info["padding"][2] - '0',
									  info["padding"][4] - '0',
		                              info["padding"][6] - '0');
	
	std::getline(stream, line);
	info             = getLineArguments(line); // common
	font->lineHeight = std::stoi(info["lineHeight"]);
	font->size       = std::stoi(info["base"]);

	hm::vec2i textureSize(std::stoi(info["scaleW"]), std::stoi(info["scaleH"]));
	
    // skip next two lines, they are unnecessary
	std::getline(stream, line);
	std::getline(stream, line);

	while(std::getline(stream, line)) {
        if(!heStringStartsWith(line, "char"))
            continue;

        // parse characters
		info = getLineArguments(line);
		uint32_t id = std::stoi(info["id"]);

		if(id > 32) {
			HeFont::Character* c = &font->characters[id];
            uint32_t x      = std::stoi(info["x"]);
			uint32_t y      = std::stoi(info["y"]);
			uint32_t width  = std::stoi(info["width"]);
			uint32_t height = std::stoi(info["height"]);
			
			c->id       = id;
			//c->uv       = hm::vec4<float>((float)x / textureSize.x, 1.f - (float)(y + height) / textureSize.y, (float) (x + width) / textureSize.x, 1.f - (float) y / textureSize.y);
			c->uv       = hm::vec4<float>((float)x / textureSize.x, 1.f - (float)(y) / textureSize.y, (float) (x + width) / textureSize.x, 1.f - (float) (y + height) / textureSize.y);
			c->size     = hm::vec2<uint32_t>(width + font->padding[2], height + font->padding[3]);
			c->offset   = hm::vec2<int32_t>(std::stoi(info["xoffset"]) - font->padding[0], std::stoi(info["yoffset"]) - font->padding[1]);
			c->xadvance = std::stoi(info["xadvance"]) - uint32_t(font->padding[0] * 1.5);
		} else if(id == 32) {
			// space
			font->spaceWidth = std::stoi(info["xadvance"]) + std::stoi(info["xoffset"]) - uint32_t(font->padding[0] * 1.5);
		}
	}

	font->atlas = heAssetPoolGetTexture("res/fonts/" + name + ".png", HE_TEXTURE_FILTER_BILINEAR | HE_TEXTURE_CLAMP_EDGE);
	
	stream.close();
};

b8 heFontHasCharacter(HeFont const* font, uint32_t const asciiCode) {
	return asciiCode == 32 || font->characters.find(asciiCode) != font->characters.end();
};

hm::vec2i heFontGetCharacterSize(HeFont const* font, char const asciiCode, uint32_t const size) {
	float scale = size / (float)(font->size);
	if(asciiCode == 32)
		return hm::vec2i(font->spaceWidth * scale, 0);

	auto it = font->characters.find((uint32_t) asciiCode);
	if(it == font->characters.end()) {
		HE_LOG("Char not found: " + std::to_string(asciiCode) + " -> " + std::string(asciiCode, 1));
		return hm::vec2i(0);
	}
		
	HeFont::Character const* _char = &it->second;
	//return hm::vec2i(hm::vec2f(_char->size - hm::vec2i(font->padding.x, font->padding.y)) * scale);
	return hm::vec2i((uint32_t) ((_char->xadvance) * scale), (uint32_t) ((_char->size.y - font->padding.y) * scale));
};

uint32_t heFontGetStringWidthInPixels(HeFont const* font, std::string const& string, uint32_t const size) {
	float width = 0;

	float scale = size / (float)font->size;
	
	for(char const all : string) {
		if(all == 32)
			width += font->spaceWidth * scale;
		else {
			auto it = font->characters.find((uint32_t) (all));
			if(it == font->characters.end())
				continue;

			HeFont::Character const* _char = &it->second;
			width += _char->xadvance * scale;
		}		
	}
	
	return (uint32_t) width;
};


// -- Assets

HeVao* heAssetPoolGetMesh(std::string const& file) { 
    auto it = heAssetPool.meshPool.find(file);
    if (it != heAssetPool.meshPool.end())
        return &it->second;
    
    // load model
    HeVao* vao = &heAssetPool.meshPool[file];
    
#ifdef HE_ENABLE_NAMES
    vao->name = file;
#endif
    
    heMeshLoad(file, vao);
    
    return vao;
};

HeTexture* heAssetPoolGetTexture(std::string const& file, HeTextureParameter const parameters) {   
    auto it = heAssetPool.texturePool.find(file);
    if (it != heAssetPool.texturePool.end()) {
        it->second.referenceCount++;
        return &it->second;
    }
    
    HeTexture* t = &heAssetPool.texturePool[file];
	t->parameters = parameters;
    t->referenceCount = 1;
    
#ifdef HE_ENABLE_NAMES
    t->name = file;
#endif
    
    heTextureLoadFromFile(t, file);
    return t;    
};

HeShaderProgram* heAssetPoolGetShader(std::string const& name) {    
    auto it = heAssetPool.shaderPool.find(name);
    if (it != heAssetPool.shaderPool.end())
        return &it->second;
    
    HeShaderProgram* s = &heAssetPool.shaderPool[name];
    
#ifdef HE_ENABLE_NAMES
    s->name = name;
#endif
    
    heShaderCreateProgram(s, "res/shaders/" + name + ".glsl");
    
    return s;    
};

HeShaderProgram* heAssetPoolGetShader(std::string const& name, std::string const& vShader, std::string const& fShader) {    
    auto it = heAssetPool.shaderPool.find(name);
    if (it != heAssetPool.shaderPool.end())
        return &it->second;
    
    HeShaderProgram* s = &heAssetPool.shaderPool[name];
    
#ifdef HE_ENABLE_NAMES
    s->name = name;
#endif
    
    heShaderCreateProgram(s, vShader, fShader);
    return s;    
};

HeShaderProgram* heAssetPoolGetShader(std::string const& name, std::string const& vShader, std::string const& gShader, std::string const& fShader) {    
    auto it = heAssetPool.shaderPool.find(name);
    if (it != heAssetPool.shaderPool.end())
        return &it->second;
    
    HeShaderProgram* s = &heAssetPool.shaderPool[name];
    
#ifdef HE_ENABLE_NAMES
    s->name = name;
#endif
    
    heShaderCreateProgram(s, vShader, gShader, fShader);
    return s;    
};

HeMaterial* heAssetPoolGetMaterial(std::string const& name) {
    
    auto it = heAssetPool.materialPool.find(name);
    if (it != heAssetPool.materialPool.end())
        return &it->second;
    
    return nullptr;
    
}

HeMaterial* heAssetPoolGetNewMaterial(std::string const& name) {    
    auto it = heAssetPool.materialPool.find(name);
    if(it != heAssetPool.materialPool.end())
        return &it->second;
    
    HeMaterial* m = &heAssetPool.materialPool[name];
    return m;
};

HeFont* heAssetPoolGetFont(std::string const& name) {
	auto it = heAssetPool.fontPool.find(name);
	if(it != heAssetPool.fontPool.end())
		return &it->second;

	HeFont* f = &heAssetPool.fontPool[name];
	heFontLoad(f, name);
	return f;
};


// -- memory tracker

uint8_t heMemoryGetBytesPerPixel(HeColourFormat const format) {
	uint8_t bytes = 0;
	switch(format) {
	case HE_COLOUR_FORMAT_RGB8:
		bytes = 3;
		break;

	case HE_COLOUR_FORMAT_RGBA8:
		bytes = 4;
		break;

	case HE_COLOUR_FORMAT_RGB16:
		bytes = 3 * 4;
		break;

	case HE_COLOUR_FORMAT_RGBA16:
		bytes = 4 * 4;
		break;
	}
	return bytes;
};

uint32_t heMemoryRoundUsage(uint32_t const usage) {
	uint32_t u = usage - 1;
	u |= u >> 1;
	u |= u >> 2;
	u |= u >> 4;
	u |= u >> 8;
	u |= u >> 16;
	return u + 1;
};


// -- ThreadLoader

void heThreadLoaderRequestTexture(HeTexture* texture) {    
    if(!heIsMainThread()) {
        heThreadLoader.textures.emplace_back(texture);
        heThreadLoader.updateRequested = true;
    }    
};

void heThreadLoaderRequestVao(HeVao* vao) {    
    if(!heIsMainThread()) {
        heThreadLoader.vaos.emplace_back(vao);
        heThreadLoader.updateRequested = true;
    }    
};

void heThreadLoaderUpdate() {    
    // textures
    for (auto& all : heThreadLoader.textures)
        heTextureCreateFromBuffer(all);
    
    // vaos
    for(auto all : heThreadLoader.vaos) {
        heVaoCreate(all);
        heVaoBind(all);
        
        HeVbo* vbo = &all->vbos[0];
        uint32_t size = 0;
        switch(vbo->type) {
            case HE_DATA_TYPE_FLOAT:
            size = (uint32_t) vbo->dataf.size();
            break;
            
            case HE_DATA_TYPE_INT:
            size = (uint32_t) vbo->datai.size();
            break;
            
            case HE_DATA_TYPE_UINT:
            size = (uint32_t) vbo->dataui.size();
            break;
        }
        
        all->verticesCount = size / all->vbos[0].dimensions;
        
        uint32_t counter = 0;
        for(auto& vbos : all->vbos)
            heVaoAddVboData(&vbos, counter++);
        
        heVaoUnbind(all);
    }
    
    // clear
    heThreadLoader.textures.clear();
    heThreadLoader.vaos.clear();
    heThreadLoader.updateRequested = false;    
};
