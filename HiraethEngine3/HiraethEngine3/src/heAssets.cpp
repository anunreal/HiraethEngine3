#include "heAssets.h"
#include "heLoader.h"
#include "heWindow.h"
#include "heUtils.h"
#include "heCore.h"
#include "heWin32Layer.h"
#include <fstream>

HeAssetPool heAssetPool;
HeThreadLoader heThreadLoader;
HeMemoryTracker heMemoryTracker;

std::unordered_map<std::string, uint32_t> shaderTypeIds;
uint32_t shaderTypeCounter = 0;


// -- text files

b8 heTextFileFillBuffer(HeTextFile* file) {
	file->stream.read(file->buffer, file->maxBufferSize);
	file->currentBufferSize = (uint32_t) file->stream.gcount();
	file->bufferOffset      = 0;
	return file->currentBufferSize;
};

void heTextFileOpen(HeTextFile* file, std::string const& path, uint32_t const bufferSize, b8 const loadVersionTag) {
	file->stream.open(path);
	if(!file) {
		// error log
		file->stream.close();
		file->open = false;
		HE_ERROR("Could not open text file [" + path + "]");
		return;
	}

	file->maxBufferSize = bufferSize;
	file->fullPath      = path;
	file->name          = path.substr(path.find_last_of('/') + 1);
	file->lineNumber    = 0;
	file->open          = true;
	
	if(file->maxBufferSize > 0) {
		file->buffer = (char*) malloc(file->maxBufferSize);
		heTextFileFillBuffer(file);
	}

	if(loadVersionTag) {
		// read version number
		std::string versionLine;
		heTextFileGetLine(file, &versionLine);
		if(!heStringStartsWith(versionLine, "#version")) {
			HE_ERROR("Expected version number at beginning of text file [" + path + "]");
			file->version = 0;
		} else {
			versionLine = versionLine.substr(8);
			heStringEatSpacesLeft(versionLine);
			heStringEatSpacesRight(versionLine);
			file->version = std::stoi(versionLine);
		}
	}
};

void heTextFileClose(HeTextFile* file) {
	if(file->stream.is_open()) { // only close if we have an open file
		if(file->buffer)
			free(file->buffer);
		file->stream.close();
		file->name.clear();
		file->fullPath.clear();
		file->lineNumber        = 0;
		file->open              = false;
		file->maxBufferSize     = 0;
		file->currentBufferSize = 0;
		file->version           = 0;
	}
};

b8 heTextFileGetLine(HeTextFile* file, std::string* result) {
	if(!file->open)
		return false;
	
	if(file->maxBufferSize == 0) {
		result->clear();
		while(result->empty() || (*result)[0] == ';') {
			if(!std::getline(file->stream, *result)) {
				file->open = false;
				break;
			}
			
			heStringEatSpacesLeft(*result);
		}
		
		file->lineNumber++;
		return file->open;
	} else {
		result->clear();
		while(file->open && (result->empty() || (*result)[0] == ';')) {
			while(file->open && file->buffer[file->bufferOffset] != '\n') {
				char c;
				if(!heTextFileGetChar(file, &c))
					break;
				//heTextFileGetChar(file, &c);
				result->push_back(c);
			}
			file->bufferOffset++; // skip \n
		}
		
		return result->size();
	}
};

b8 heTextFileGetChar(HeTextFile* file, char* result) {
	if(file->maxBufferSize == 0) {
		file->stream.get(*result);
		file->open = file->stream.good();
		return file->open;
	} else {
		if(file->bufferOffset >= file->currentBufferSize) {
			if(!heTextFileFillBuffer(file)) {
				heTextFileClose(file);
				*result = -1;
			}
		}

		if(file->open)
			*result = file->buffer[file->bufferOffset++];
		return file->open;
	}
};

b8 heTextFileGetFloat(HeTextFile* file, float* result) {
	char c;
    heTextFileGetChar(file, &c);
    if(c == '\n')
        return false;
    
    int sign = (c == '-') ? -1 : 1;
    *result = 0;
    
    // 0 = 3;
    // 1 = 2;
    // 2 = 1;
    // 3 = 0
    // 4 = -1
    // 5 = -2
    // 6 = -3
    
    for(uint8_t i = 0; i < 7; ++i) {
        heTextFileGetChar(file, &c);
        int exp = -(i - 4) - 1;
        int ch = (int) (c - '0');
        *result = *result + (float) (ch * std::pow(10, exp));

		if(i == 3)
			heTextFileGetChar(file, &c);
	}
    
    *result *= sign;
    return true;
};

b8 heTextFileGetFloats(HeTextFile* file, uint8_t const count, void* ptr) {
    for(uint8_t i = 0; i < count; ++i) {
        if(!heTextFileGetFloat(file, &((float*)ptr)[i]))
            return false;
    }
    
    return true;
};

template<typename T>
b8 heTextFileGetInt(HeTextFile* file, T* result) {
    char c;
	heTextFileGetChar(file, &c);
    if(c == '\n')
        return false;
    
    uint8_t sign = (c == '-') ? -1 : 1;
    *result = 0;
    
    for(uint8_t i = 0; i < 4; ++i) {
		heTextFileGetChar(file, &c);
        uint16_t exp = (3 - i);
        T ch = (T) (c - '0');
        *result = *result + ch * (T) std::pow(10, exp);
    }
    
    *result *= sign;
    return true;

};

template<typename T>
b8 heTextFileGetInts(HeTextFile* file, uint8_t const count, void* ptr) {
    for(uint8_t i = 0; i < count; ++i)
        if(!heTextFileGetInt(file, &((T*)ptr)[i]))
			return false;
    
    return true;
};

template b8 heTextFileGetInts<uint8_t>(HeTextFile*, uint8_t const, void*);
template b8 heTextFileGetInts<int32_t>(HeTextFile*, uint8_t const, void*);

b8 heTextFileGetContent(HeTextFile* file, std::string* result) {
	if(file->maxBufferSize == 0) {
		file->stream.seekg(0, std::ios::end);
		result->reserve(file->stream.tellg());
		file->stream.seekg(0, std::ios::beg);

		result->assign((std::istreambuf_iterator<char>(file->stream)), std::istreambuf_iterator<char>());
	} else {
		result->clear();
		//result->reserve(file->currentBufferSize - 1);
		//strcpy(result->data(), file->buffer, file->currentBufferSize - 1);
		do {
			*result += std::string(file->buffer, file->currentBufferSize);
		} while(heTextFileFillBuffer(file));
	}
	return true;
};

char heTextFilePeek(HeTextFile* file) {
	if(file->maxBufferSize == 0 || file->bufferOffset == file->currentBufferSize)
		// we either dont use buffers or the current buffer was already completely read
		return file->stream.peek();
	else
		return file->buffer[file->bufferOffset + 1];
};

void heTextFileSkipLine(HeTextFile* file) {
	file->stream.ignore(100000, file->stream.widen('\n'));
};



// -- Materials

HeMaterial* heMaterialCreatePbr(std::string const& name, HeTexture* diffuseTexture, HeTexture* normalTexture, HeTexture* armTexture) {
	HeMaterial* mat = heAssetPoolGetNewMaterial(name);
	mat->shader = heAssetPoolGetShader("3d_pbr");
	mat->textures["diffuse"] = diffuseTexture;
	mat->textures["normal"]	 = normalTexture;
	mat->textures["arm"]	 = armTexture;
	mat->type				 = heMaterialGetType("3d_pbr");
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
	HeTextFile file;
	heTextFileOpen(&file, "res/fonts/" + name + ".fnt", 0);
	
#ifdef HE_ENABLE_NAMES
	font->name = name;
#endif
	
	std::string line;
	heTextFileGetLine(&file, &line);
	
	// parse basic info
	auto info	  = getLineArguments(line); // info
	//font->size	= std::stoi(info["size"]);
	font->padding = hm::vec4<uint8_t>(info["padding"][0] - '0',
									  info["padding"][2] - '0',
									  info["padding"][4] - '0',
									  info["padding"][6] - '0');
	
	heTextFileGetLine(&file, &line);
	info			 = getLineArguments(line); // common
	font->lineHeight = std::stoi(info["lineHeight"]);
	font->size		 = std::stoi(info["base"]);
	
	hm::vec2i textureSize(std::stoi(info["scaleW"]), std::stoi(info["scaleH"]));
	
	// skip next two lines, they are unnecessary
	heTextFileSkipLine(&file);
	heTextFileSkipLine(&file);
	
	while(file.open) {
		heTextFileGetLine(&file, &line);
		if(!heStringStartsWith(line, "char"))
			continue;
		
		// parse characters
		info = getLineArguments(line);
		int32_t id = std::stoi(info["id"]);
		
		if(id > 32 && id != 127 /* backspace */) {
			HeFont::Character* c = &font->characters[id];
			uint32_t x		= std::stoi(info["x"]);
			uint32_t y		= std::stoi(info["y"]);
			uint32_t width	= std::stoi(info["width"]);
			uint32_t height = std::stoi(info["height"]);
			
			c->id		= id;
			c->uv		= hm::vec4<float>((float)x / textureSize.x, 1.f - (float)(y) / textureSize.y, (float) (x + width) / textureSize.x, 1.f - (float) (y + height) / textureSize.y);
			c->size		= hm::vec2<uint32_t>(width + font->padding[2], height + font->padding[3]);
			c->offset	= hm::vec2<int32_t>(std::stoi(info["xoffset"]) - font->padding[0], std::stoi(info["yoffset"]) - font->padding[1]);
			c->xadvance = std::stoi(info["xadvance"]) - uint32_t(font->padding[0] * 1.5);
		} else if(id == 32) {
			// space
			font->spaceWidth = std::stoi(info["xadvance"]) - uint32_t(font->padding[0] * 1.5);;
		}
	}
	
	font->atlas = heAssetPoolGetTexture("res/fonts/" + name + ".png", HE_TEXTURE_FILTER_BILINEAR | HE_TEXTURE_CLAMP_EDGE);
	heTextFileClose(&file);
};

void heFontCreateScaled(HeFont const* font, HeScaledFont* scaled, uint32_t const size) {
	scaled->scale      = size / (float) font->size;
	scaled->font       = font;
	scaled->size       = size;
	scaled->lineHeight = font->lineHeight * scaled->scale;
	scaled->baseLine   = font->baseLine   * scaled->scale;
	scaled->spaceWidth = font->spaceWidth * scaled->scale;
	scaled->padding    = hm::vec4f(font->padding) * scaled->scale;
};

b8 heFontHasCharacter(HeFont const* font, int32_t const asciiCode) {
	return asciiCode == 32 || font->characters.find(asciiCode) != font->characters.end();
};


b8 heScaledFontHasCharacter(HeScaledFont const* font, int32_t const asciiCode) {
	return asciiCode == 32 || font->font->characters.find(asciiCode) != font->font->characters.end();
};

hm::vec2i heFontGetCharacterSize(HeFont const* font, int32_t const asciiCode) {
	if(asciiCode == ' ')
		return hm::vec2i(font->spaceWidth, 0);
	
	auto it = font->characters.find(asciiCode);
	if(it == font->characters.end()) {
		HE_LOG("Char not found: " + std::to_string(asciiCode) + " -> " + std::string(asciiCode, 1));
		return hm::vec2i(0);
	}
	
	HeFont::Character const* _char = &it->second;
	return hm::vec2i(_char->xadvance, _char->size.y - font->padding.y);
};

hm::vec2f heScaledFontGetCharacterSize(HeScaledFont const* font, int32_t const asciiCode) {
	if(asciiCode == ' ')
		return hm::vec2f(font->spaceWidth, 0);
	
	auto it = font->font->characters.find(asciiCode);
	if(it == font->font->characters.end()) {
		HE_LOG("Char not found: " + std::to_string(asciiCode) + " -> " + std::string(asciiCode, 1));
		return hm::vec2f(0);
	}
	
	HeFont::Character const* _char = &it->second;
	return hm::vec2f(_char->xadvance * font->scale, (_char->size.y - font->padding.y) * font->scale);
};

int32_t heFontGetStringWidthInPixels(HeFont const* font, std::string const& string) {
	int32_t width = 0;
	
	for(int32_t const all : string) {
		if(all == 32)
			width += font->spaceWidth;
		else {
			auto it = font->characters.find(all);
			if(it == font->characters.end())
				it = font->characters.find('?');

			HeFont::Character const* _char = &it->second;
			width += _char->xadvance;
		}
	}
	
	return width;
};

float heScaledFontGetStringWidthInPixels(HeScaledFont const* font, std::string const& string) {
	float width = 0;
	
	for(int32_t const all : string) {
		if(all == 32)
			width += font->spaceWidth;
		else {
			auto it = font->font->characters.find(all);
			if(it == font->font->characters.end())
				it = font->font->characters.find('?');

			HeFont::Character const* _char = &it->second;
			width += _char->xadvance * font->scale;
		}
	}
	
	return width;
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

