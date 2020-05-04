#include "heGlLayer.h"
#include "heAssets.h"
#include "GLEW/glew.h"
#include "GLEW/wglew.h"
#include "heWindow.h"
#include "heCore.h"
#include "heWin32Layer.h"
#include "heDebugUtils.h"
#include "heBinary.h"
#include <fstream>
#include <algorithm>

#define GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX 0x9048
#define GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX 0x9049

#define STB_IMAGE_IMPLEMENTATION
#pragma warning(push, 0)
#include "..\stb_image.h"
#pragma warning(pop)


// --- Shaders

std::string heShaderLoadSource(std::string const& file) {
	std::ifstream stream(file);
	if (!stream) {
		HE_ERROR("Could not find shader file [" + file + "]");
		return "";
	}
	
	std::string source;
	std::string line;
	
	while (std::getline(stream, line))
		source += line + "\n";
	
	stream.close();
	return source;
};

std::unordered_map<HeShaderType, std::string> heShaderLoadSourceAll(std::string const& file) {
	std::ifstream stream(file);
	std::unordered_map<HeShaderType, std::string> sourceMap;
	if (!stream) {
		HE_ERROR("Could not find shader file [" + file + "]");
		return sourceMap;
	}
	
	std::string line;
	
	HeShaderType currentType;
	while (std::getline(stream, line)) {
		if(line[0] == '#') {
			if(line.compare(1, 6, "vertex") == 0) {
				currentType = HE_SHADER_TYPE_VERTEX;
				continue;
			} else if(line.compare(1, 8, "geometry") == 0) {
				currentType = HE_SHADER_TYPE_GEOMETRY;
				continue;
			} else if(line.compare(1, 8, "fragment") == 0) {
				currentType = HE_SHADER_TYPE_FRAGMENT;
				continue;
			}
		}
		
		sourceMap[currentType] += line + "\n";
	}
	
	stream.close();
	return sourceMap;
};

uint32_t heShaderLoadFromSource(std::string const& source, uint32_t type, std::string const& name = "") {
	const char* src_ptr = source.c_str();
	uint32_t id = glCreateShader(type);
	glShaderSource(id, 1, &src_ptr, NULL);
	glCompileShader(id);
	
#ifdef HE_ENABLE_NAMES
	glObjectLabel(GL_SHADER, id, (uint32_t) name.size(), name.c_str());
#endif
	
	int32_t compileStatus;
	glGetShaderiv(id, GL_COMPILE_STATUS, &compileStatus);
	if(compileStatus == 0) {
		char infoLog[512];
		memset(infoLog, 0, 512);
		glGetShaderInfoLog(id, 512, NULL, infoLog);
		HE_ERROR("While compiling shader [" + name + "]:\n" + std::string(infoLog));
		return 0;
	} else
		return id;
};

uint32_t heShaderLoadFromFile(std::string const& file, uint32_t type) {
	std::string source = heShaderLoadSource(file);
	if(source.empty())
		return 0;
	
	return heShaderLoadFromSource(source, type, file);
};


void heShaderCompileProgram(HeShaderProgram* program) {
	glLinkProgram(program->programId);
	
	int32_t success = 0;
	glGetProgramiv(program->programId, GL_LINK_STATUS, &success);
	if (!success) {
		char infoLog[512];
		memset(infoLog, 0, 512);
		glGetProgramInfoLog(program->programId, 512, NULL, infoLog);
		HE_ERROR("Unable to link shader program:\n" + std::string(infoLog));
		program->programId = 0;
	}
	
	glUseProgram(program->programId);
};

void heShaderLoadCompute(HeShaderProgram* program, std::string const& computeShader) {
	uint32_t sid = heShaderLoadFromFile(computeShader, GL_COMPUTE_SHADER);
	if(sid == 0)
		return;
	
	program->programId = glCreateProgram();
	glAttachShader(program->programId, sid);
	heShaderCompileProgram(program);
	glDeleteShader(sid);

	int32_t bytes;
	glGetProgramiv(program->programId, GL_PROGRAM_BINARY_LENGTH, &bytes);
	program->memory = (uint32_t) bytes;
	heMemoryTracker[HE_MEMORY_TYPE_SHADER] += program->memory;
		
#ifdef HE_ENABLE_NAMES
	if(program->name.empty())
		program->name = computeShader;
	glObjectLabel(GL_PROGRAM, program->programId, (uint32_t) program->name.size(), program->name.c_str());
#endif
};

void heShaderLoadProgram(HeShaderProgram* program, std::string const& vertexShader, std::string const& fragmentShader) {
	uint32_t vs = heShaderLoadFromFile(vertexShader, GL_VERTEX_SHADER);
	uint32_t fs = heShaderLoadFromFile(fragmentShader, GL_FRAGMENT_SHADER);
	
	if(vs == 0 || fs == 0)
		return;
	
	program->programId = glCreateProgram();
	glAttachShader(program->programId, vs);
	glAttachShader(program->programId, fs);
	heShaderCompileProgram(program);
	glDeleteShader(vs);
	glDeleteShader(fs);

	int32_t bytes;
	glGetProgramiv(program->programId, GL_PROGRAM_BINARY_LENGTH, &bytes);
	program->memory = (uint32_t) bytes;
	heMemoryTracker[HE_MEMORY_TYPE_SHADER] += program->memory;
		
#ifdef HE_ENABLE_NAMES
	glObjectLabel(GL_PROGRAM, program->programId, (uint32_t) program->name.size(), program->name.c_str());
#endif
};

void heShaderLoadProgram(HeShaderProgram* program, std::string const& vertexShader, std::string const& geometryShader, std::string const& fragmentShader) {
	uint32_t vs = heShaderLoadFromFile(vertexShader, GL_VERTEX_SHADER);
	uint32_t gs = heShaderLoadFromFile(geometryShader, GL_GEOMETRY_SHADER);
	uint32_t fs = heShaderLoadFromFile(fragmentShader, GL_FRAGMENT_SHADER);
	
	if(vs == 0 || gs == 0 || fs == 0)
		return;
	
	program->programId = glCreateProgram();
	glAttachShader(program->programId, vs);
	glAttachShader(program->programId, gs);
	glAttachShader(program->programId, fs);
	heShaderCompileProgram(program);
	glDeleteShader(vs);
	glDeleteShader(gs);
	glDeleteShader(fs);

	int32_t bytes;
	glGetProgramiv(program->programId, GL_PROGRAM_BINARY_LENGTH, &bytes);
	program->memory = (uint32_t) bytes;
	heMemoryTracker[HE_MEMORY_TYPE_SHADER] += program->memory;
		
#ifdef HE_ENABLE_NAMES
	glObjectLabel(GL_PROGRAM, program->programId, (uint32_t) program->name.size(), program->name.c_str());
#endif
};

void heShaderLoadProgram(HeShaderProgram* program, std::string const& file) {
	std::unordered_map<HeShaderType, std::string> source = heShaderLoadSourceAll(file);
	uint32_t vs = heShaderLoadFromSource(source[HE_SHADER_TYPE_VERTEX], GL_VERTEX_SHADER, file + "#vertex");
	uint32_t fs = heShaderLoadFromSource(source[HE_SHADER_TYPE_FRAGMENT], GL_FRAGMENT_SHADER, file + "#fragment");
	int32_t gs = -1;
	if(source.find(HE_SHADER_TYPE_GEOMETRY) != source.end())
		gs = heShaderLoadFromSource(source[HE_SHADER_TYPE_GEOMETRY], GL_GEOMETRY_SHADER, file + "#geoemtry");
	
	if(vs == 0 || fs == 0 || gs == 0)
		return;
	
	program->programId = glCreateProgram();
	glAttachShader(program->programId, vs);
	if(gs != -1) glAttachShader(program->programId, gs);
	glAttachShader(program->programId, fs);
	heShaderCompileProgram(program);
	glDeleteShader(vs);
	if(gs != -1) glDeleteShader(gs);
	glDeleteShader(fs);

	int32_t bytes;
	glGetProgramiv(program->programId, GL_PROGRAM_BINARY_LENGTH, &bytes);
	program->memory = (uint32_t) bytes;
	heMemoryTracker[HE_MEMORY_TYPE_SHADER] += program->memory;
	
#ifdef HE_ENABLE_NAMES
	if(program->name.empty())
		program->name = file;
	glObjectLabel(GL_PROGRAM, program->programId, (uint32_t) program->name.size(), program->name.c_str());
#endif
};

void heShaderCreateCompute(HeShaderProgram* program, std::string const& shaderFile) {
	program->files.emplace_back(shaderFile);
	program->computeShader = true;
	heShaderLoadCompute(program, shaderFile);
};

void heShaderCreateProgram(HeShaderProgram* program, std::string const& vertexShader, std::string const& fragmentShader) {
	program->files.emplace_back(vertexShader);
	program->files.emplace_back(fragmentShader);
	program->computeShader = false;
	heShaderLoadProgram(program, vertexShader, fragmentShader);
};

void heShaderCreateProgram(HeShaderProgram* program, std::string const& vertexShader, std::string const& geometryShader, std::string const& fragmentShader) {
	program->files.emplace_back(vertexShader);
	program->files.emplace_back(geometryShader);
	program->files.emplace_back(fragmentShader);
	program->computeShader = false;
	heShaderLoadProgram(program, vertexShader, geometryShader, fragmentShader);
};

void heShaderCreateProgram(HeShaderProgram* program, std::string const& file) {
	program->files.emplace_back(file);
	program->computeShader = false;
	heShaderLoadProgram(program, file);
};

void heShaderBind(HeShaderProgram* program) {
	heShaderCheckReload(program);
	glUseProgram(program->programId);
};

void heShaderUnbind() {
	glUseProgram(0);
};

void heShaderDestroy(HeShaderProgram* program) {
	glDeleteProgram(program->programId);
	program->programId = 0;
	program->uniforms.clear();
	program->samplers.clear();
};

void heShaderRunCompute(HeShaderProgram* program, uint32_t const groupsX, uint32_t const groupsY, uint32_t const groupsZ) {
	heShaderBind(program);
	glDispatchCompute(groupsX, groupsY, groupsZ);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
};

void heShaderReload(HeShaderProgram* program) {
	// store current uniform values
	struct Uniform {
		union {
			float	fdata[16];
			int32_t idata[16];
		};
		b8 isInt = false;
		HeDataType type = HE_DATA_TYPE_NONE;
	};
	
	int32_t uniformCount;
	glGetProgramInterfaceiv(program->programId, GL_UNIFORM, GL_ACTIVE_RESOURCES, &uniformCount);
	std::map<std::string, Uniform> uniforms;
	
	std::vector<GLchar> nameData(256);
	std::vector<GLenum> properties;
	properties.push_back(GL_NAME_LENGTH);
	properties.push_back(GL_TYPE);
	properties.push_back(GL_LOCATION);
	std::vector<GLint> values(properties.size());
	
	for(int32_t i = 0; i < uniformCount; ++i) {
		glGetProgramResourceiv(program->programId, GL_UNIFORM, i, (GLsizei) properties.size(), &properties[0], (GLsizei) values.size(), NULL, &values[0]);
		
		nameData.resize(values[0]); // length of the name
		glGetProgramResourceName(program->programId, GL_UNIFORM, i, (GLsizei) nameData.size(), NULL, &nameData[0]);
		std::string name((char*)&nameData[0], nameData.size() - 1);
		
		Uniform u;
		u.type = (HeDataType) values[1];
		
		if(u.type == HE_DATA_TYPE_INT || u.type == HE_DATA_TYPE_BOOL) {
			glGetUniformiv(program->programId, values[2], &u.idata[0]);
			u.isInt = true;
		} else
			glGetUniformfv(program->programId, values[2], &u.fdata[0]);
		
		uniforms[name] = u;
	}
	
	
	// reload shader
	heShaderDestroy(program);
	
	if(program->computeShader)
		heShaderLoadCompute(program, program->files[0]);
	else {
		if(program->files.size() == 1)
			// all in one file
			heShaderLoadProgram(program, program->files[0]);
		if(program->files.size() == 2)
			// vertex -> fragment
			heShaderLoadProgram(program, program->files[0], program->files[1]);
		else if(program->files.size() == 3)
			// vertex -> geometry -> fragment
			heShaderLoadProgram(program, program->files[0], program->files[1], program->files[2]);
	}
	
	heShaderBind(program);
	
	// upload data to it
	for(auto const& all : uniforms)
		heShaderLoadUniform(program, all.first, all.second.isInt ? (const void*) &all.second.idata[0] : (const void*) &all.second.fdata[0], all.second.type);
	
	std::string names;
	for(std::string const& all : program->files)
		names += all + ", ";
	
	HE_DEBUG("Reloaded program (" + names.substr(0, names.size() - 2) + ")");
};

void heShaderCheckReload(HeShaderProgram* program) {
#ifdef HE_ENABLE_HOTSWAP_SHADER
	b8 needsReloading = false;
	
	for(std::string const& files : program->files) {
		if(heWin32FileModified(files)) { // TODO(Victor): Add Platform abstraction!
			needsReloading = true;
			break;
		}
	}
	
	if(needsReloading)
		heShaderReload(program);
#endif
};


int heShaderGetUniformLocation(HeShaderProgram* program, std::string const& uniform) {
	int32_t location;
	if (program->uniforms.find(uniform) != program->uniforms.end()) {
		location = program->uniforms[uniform];
	} else {
		location = glGetUniformLocation(program->programId, uniform.c_str());
		program->uniforms[uniform] = location;
		
		if(location == -1) {
#ifdef HE_ENABLE_NAMES
			HE_DEBUG("Could not find shader uniform [" + uniform + "] in shader [" + program->name + "]");
#else
			HE_DEBUG("Could not find shader uniform [" + uniform + "]");
#endif
		}
	}
	
	return location;
};

int heShaderGetSamplerLocation(HeShaderProgram* program, std::string const& sampler, uint8_t const requestedSlot) {
	int32_t location;
	if (program->samplers.find(sampler) != program->samplers.end()) {
		location = program->samplers[sampler];
	} else {
		int32_t uloc = glGetUniformLocation(program->programId, sampler.c_str());
		glProgramUniform1i(program->programId, uloc, requestedSlot);
		program->samplers[sampler] = requestedSlot;
		location = requestedSlot;
		
		if(uloc == -1) {
#ifdef HE_ENABLE_NAMES
			HE_DEBUG("Could not find shader sampler [" + sampler + "] in shader [" + program->name + "]");
#else
			HE_DEBUG("Could not find shader sampler [" + sampler + "]");
#endif
		}
	}
	
	return location;
};

void heShaderClearSamplers(HeShaderProgram const* program) {
	for(auto const& all : program->samplers)
		heTextureBind(nullptr, all.second);
};

void heShaderLoadUniform(HeShaderProgram* program, std::string const& uniformName, void const* data, HeDataType const type) {
	int32_t location = heShaderGetUniformLocation(program, uniformName);
	
	switch(type) {
	case HE_DATA_TYPE_INT:
	case HE_DATA_TYPE_BOOL:
		heShaderLoadUniform(program, uniformName, ((int32_t*) data)[0]);
		break;
		
	case HE_DATA_TYPE_FLOAT:
		heShaderLoadUniform(program, uniformName, ((float*) data)[0]);
		break;
		
	case HE_DATA_TYPE_VEC2:
		heShaderLoadUniform(program, uniformName, hm::vec2f(((float*) data)[0], ((float*) data)[1]));
		break;
		
	case HE_DATA_TYPE_VEC3:
		heShaderLoadUniform(program, uniformName, hm::vec3f(((float*) data)[0], ((float*) data)[1], ((float*) data)[2]));
		break;
		
	case HE_DATA_TYPE_VEC4:
		heShaderLoadUniform(program, uniformName, hm::vec4f(((float*) data)[0], ((float*) data)[1], ((float*) data)[2], ((float*) data)[3]));
		break;
		
	case HE_DATA_TYPE_MAT4:
		heShaderLoadUniform(program, uniformName, hm::mat4f(&((float*) data)[0]));
		break;
		
	};
};

void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const float value) {
	int location = heShaderGetUniformLocation(program, uniformName);
	glUniform1f(location, value);
};

void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const double value) {
	int location = heShaderGetUniformLocation(program, uniformName);
	glUniform1d(location, value);
};

void heShaderLoadUniform(HeShaderProgram* program, std::string const& uniformName, int32_t const value) {
	int location = heShaderGetUniformLocation(program, uniformName);
	glUniform1i(location, value);
};

void heShaderLoadUniform(HeShaderProgram* program, std::string const& uniformName, uint32_t const value) {
	int location = heShaderGetUniformLocation(program, uniformName);
	glUniform1i(location, value);
};

void heShaderLoadUniform(HeShaderProgram* program, std::string const& uniformName, hm::mat3f const& value) {
	int location = heShaderGetUniformLocation(program, uniformName);
	glUniformMatrix3fv(location, 1, false, &value[0][0]);
};

void heShaderLoadUniform(HeShaderProgram* program, std::string const& uniformName, hm::mat4f const& value) {
	int location = heShaderGetUniformLocation(program, uniformName);
	glUniformMatrix4fv(location, 1, false, &value[0][0]);
};

void heShaderLoadUniform(HeShaderProgram* program, std::string const& uniformName, hm::vec2f const& value) {
	int location = heShaderGetUniformLocation(program, uniformName);
	glUniform2f(location, value.x, value.y);
};

void heShaderLoadUniform(HeShaderProgram* program, std::string const& uniformName, hm::vec3f const& value) {
	int location = heShaderGetUniformLocation(program, uniformName);
	glUniform3f(location, value.x, value.y, value.z);
};

void heShaderLoadUniform(HeShaderProgram* program, std::string const& uniformName, hm::vec4f const& value) {
	int location = heShaderGetUniformLocation(program, uniformName);
	glUniform4f(location, value.x, value.y, value.z, value.w);
};

void heShaderLoadUniform(HeShaderProgram* program, std::string const& uniformName, hm::colour const& value) {
	int location = heShaderGetUniformLocation(program, uniformName);
	glUniform4f(location, getR(&value), getG(&value), getB(&value), getA(&value));
};

void heShaderLoadUniform(HeShaderProgram* program, std::string const& uniformName, HeShaderData const* data) {
	switch (data->type) {
	case HE_DATA_TYPE_INT:
		heShaderLoadUniform(program, uniformName, data->_int);
		break;
		
	case HE_DATA_TYPE_FLOAT:
		heShaderLoadUniform(program, uniformName, data->_float);
		break;
		
	case HE_DATA_TYPE_VEC2:
		heShaderLoadUniform(program, uniformName, data->_vec2);
		break;
		
	case HE_DATA_TYPE_COLOUR:
		heShaderLoadUniform(program, uniformName, data->_colour);
		break;
	};
};


// --- Buffers

void heVboCreate(HeVbo* vbo, std::vector<float> const& data, uint8_t const dimensions, HeVboUsage const usage) {
	uint32_t size = (uint32_t) data.size() * sizeof(float); 
	glGenBuffers(1, &vbo->vboId);
	glBindBuffer(GL_ARRAY_BUFFER, vbo->vboId);
	glBufferData(GL_ARRAY_BUFFER, size, data.data(), usage);
	vbo->usage		   = usage;
	vbo->dimensions	   = dimensions;
	vbo->verticesCount = (uint32_t)data.size() / vbo->dimensions;
	vbo->type		   = HE_DATA_TYPE_FLOAT;
	vbo->memory		   = size;
	heMemoryTracker[HE_MEMORY_TYPE_VAO] += vbo->memory;
};

void heVboCreateInt(HeVbo* vbo, std::vector<int32_t> const& data, uint8_t const dimensions, HeVboUsage const usage) {
	uint32_t size = (uint32_t) data.size() * sizeof(int32_t);
	glGenBuffers(1, &vbo->vboId);
	glBindBuffer(GL_ARRAY_BUFFER, vbo->vboId);
	glBufferData(GL_ARRAY_BUFFER, (uint32_t)data.size() * sizeof(int32_t), data.data(), usage);
	vbo->usage		   = usage;
	vbo->dimensions	   = dimensions;
	vbo->verticesCount = (uint32_t)data.size() / vbo->dimensions;
	vbo->type		   = HE_DATA_TYPE_INT;
	vbo->memory		   = size;
	heMemoryTracker[HE_MEMORY_TYPE_VAO] += vbo->memory;
};

void heVboCreateUint(HeVbo* vbo, std::vector<uint32_t> const& data, uint8_t const dimensions, HeVboUsage const usage) {
	uint32_t size = (uint32_t) data.size() * sizeof(uint32_t);
	glGenBuffers(1, &vbo->vboId);
	glBindBuffer(GL_ARRAY_BUFFER, vbo->vboId);
	glBufferData(GL_ARRAY_BUFFER, (uint32_t)data.size() * sizeof(int32_t), data.data(), usage);
	vbo->usage		   = usage;
	vbo->dimensions	   = dimensions;
	vbo->verticesCount = (uint32_t)data.size() / vbo->dimensions;
	vbo->type		   = HE_DATA_TYPE_UINT;
	vbo->memory		   = size;
	heMemoryTracker[HE_MEMORY_TYPE_VAO] += vbo->memory;
};

void heVboAllocate(HeVbo* vbo, uint32_t const size, const uint8_t dimensions, HeVboUsage const usage, HeDataType const type) {
	uint32_t sizeBytes = (uint32_t) size * (type == HE_DATA_TYPE_FLOAT ? sizeof(float) : sizeof(int32_t));
	glGenBuffers(1, &vbo->vboId);
	glBindBuffer(GL_ARRAY_BUFFER, vbo->vboId);
	
	if(size > 0)
		glBufferData(GL_ARRAY_BUFFER, sizeBytes, nullptr, usage);
	
	vbo->usage		   = usage;
	vbo->dimensions	   = dimensions;
	vbo->verticesCount = 0;
	vbo->type		   = type;
	vbo->memory		   = sizeBytes;
	heMemoryTracker[HE_MEMORY_TYPE_VAO] += vbo->memory;
};

void heVboDestroy(HeVbo* vbo) {
	heMemoryTracker[HE_MEMORY_TYPE_VAO] -= vbo->memory;
	glDeleteBuffers(1, &vbo->vboId);
	vbo->vboId		   = 0;
	vbo->verticesCount = 0;
	vbo->dimensions	   = 0;
	vbo->type		   = HE_DATA_TYPE_NONE;
	vbo->memory		   = 0;
};

void heVaoCreate(HeVao* vao, HeVaoType const type) {
	glGenVertexArrays(1, &vao->vaoId);
	vao->type = type;
	
#ifdef HE_ENABLE_NAMES
	glBindVertexArray(vao->vaoId); // we have to bind it so that its valid for naming
	if(!vao->name.empty())
		glObjectLabel(GL_VERTEX_ARRAY, vao->vaoId, -1, vao->name.c_str());
	glBindVertexArray(0);
#endif
};

void heVaoAddVboData(HeVbo* vbo, int8_t const attributeIndex) {
	if(vbo->type == HE_DATA_TYPE_FLOAT) {
		heVboCreate(vbo, vbo->dataf, vbo->dimensions, vbo->usage);
		glVertexAttribPointer(attributeIndex, vbo->dimensions, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
		vbo->dataf.clear();
	} else if(vbo->type == HE_DATA_TYPE_INT) {
		heVboCreateInt(vbo, vbo->datai, vbo->dimensions, vbo->usage);
		glVertexAttribIPointer(attributeIndex, vbo->dimensions, vbo->type, 0, (GLvoid*)0);
		vbo->datai.clear();
	} else if(vbo->type == HE_DATA_TYPE_UINT) {
		heVboCreateUint(vbo, vbo->dataui, vbo->dimensions, vbo->usage);
		glVertexAttribIPointer(attributeIndex, vbo->dimensions, vbo->type, 0, (GLvoid*)0);
		vbo->dataui.clear();
	}
	
#ifdef HE_ENABLE_NAMES
	int32_t currentVao;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVao);
	char buf[256];
	int32_t bytes;
	glGetObjectLabel(GL_VERTEX_ARRAY, currentVao, 256, &bytes, (GLchar*) &buf);
	std::string name = std::string(buf, bytes) + "[" + std::to_string(attributeIndex) + "]";
	glObjectLabel(GL_BUFFER, vbo->vboId, (uint32_t) name.size(), name.c_str());
#endif
};

void heVaoAddVbo(HeVao* vao, HeVbo* vbo) {
	if(vbo->type == HE_DATA_TYPE_FLOAT)
		glVertexAttribPointer((uint32_t) vao->vbos.size(), vbo->dimensions, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	else
		glVertexAttribIPointer((uint32_t) vao->vbos.size(), vbo->dimensions, vbo->type, 0, (GLvoid*)0);
	
	vao->vbos.emplace_back(*vbo);
	if (vao->vbos.size() == 1)
		vao->verticesCount = vbo->verticesCount;
	
#ifdef HE_ENABLE_NAMES
	std::string name = vao->name + "[" + std::to_string(vao->vbos.size()) + "]";
	glObjectLabel(GL_BUFFER, vbo->vboId, (uint32_t) name.size(), name.c_str());
#endif
};

void heVaoAddData(HeVao* vao, std::vector<float> const& data, uint8_t const dimensions, HeVboUsage const usage) {
	if(heIsMainThread()) {
		HeVbo vbo;
		heVboCreate(&vbo, data, dimensions, usage);
		heVaoAddVbo(vao, &vbo);
	} else {
		HeVbo* vbo = &vao->vbos.emplace_back();
		vbo->dataf		= data;
		vbo->dimensions = dimensions;
		vbo->usage		= usage;
		vbo->type		= HE_DATA_TYPE_FLOAT;
	}
};

void heVaoAddDataInt(HeVao* vao, std::vector<int32_t> const& data, uint8_t const dimensions, HeVboUsage const usage) {
	if(heIsMainThread()) {
		HeVbo vbo;
		heVboCreateInt(&vbo, data, dimensions, usage);
		heVaoAddVbo(vao, &vbo);
	} else {
		HeVbo* vbo = &vao->vbos.emplace_back();
		vbo->datai		= data;
		vbo->dimensions = dimensions;
		vbo->usage		= usage;
		vbo->type		= HE_DATA_TYPE_INT;
	}
};

void heVaoAddDataUint(HeVao* vao, std::vector<uint32_t> const& data, uint8_t const dimensions, HeVboUsage const usage) {
	if(heIsMainThread()) {
		HeVbo vbo;
		heVboCreateUint(&vbo, data, dimensions, usage);
		heVaoAddVbo(vao, &vbo);
	} else {
		HeVbo* vbo = &vao->vbos.emplace_back();
		vbo->dataui		= data;
		vbo->dimensions = dimensions;
		vbo->usage		= usage;
		vbo->type		= HE_DATA_TYPE_UINT;
	}
};

void heVaoUpdateData(HeVao* vao, std::vector<float> const& data, uint8_t const vboIndex) {
	size_t size = data.size();
	size_t bytes = size * sizeof(float);
	HeVbo* vbo = &vao->vbos[vboIndex];
	glBindBuffer(GL_ARRAY_BUFFER, vbo->vboId);
	
	if(data.size() > vbo->verticesCount) {
		glBufferData(GL_ARRAY_BUFFER, bytes, data.data(), vbo->usage);
	} else {
		glBufferData(GL_ARRAY_BUFFER, bytes, nullptr, vbo->usage);
		glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, data.data());
	}
	
	vbo->verticesCount = (uint32_t)size / vbo->dimensions;
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	if(vboIndex == 0)
		vao->verticesCount = vbo->verticesCount;
};

void heVaoUpdateDataInt(HeVao* vao, std::vector<int32_t> const& data, uint8_t const vboIndex) {
	size_t size = data.size();
	size_t bytes = size * sizeof(float);
	HeVbo* vbo = &vao->vbos[vboIndex];
	glBindBuffer(GL_ARRAY_BUFFER, vbo->vboId);
	
	if(data.size() > vbo->verticesCount) {
		glBufferData(GL_ARRAY_BUFFER, bytes, data.data(), vbo->usage);
	} else {
		glBufferData(GL_ARRAY_BUFFER, bytes, nullptr, vbo->usage);
		glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, data.data());
	}
	
	vbo->verticesCount = (uint32_t)size / vbo->dimensions;
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	if(vboIndex == 0)
		vao->verticesCount = vbo->verticesCount;
};

void heVaoUpdateDataUint(HeVao* vao, std::vector<uint32_t> const& data, uint8_t const vboIndex) {
	size_t size = data.size();
	size_t bytes = size * sizeof(float);
	HeVbo* vbo = &vao->vbos[vboIndex];
	glBindBuffer(GL_ARRAY_BUFFER, vbo->vboId);
	
	if(data.size() > vbo->verticesCount) {
		glBufferData(GL_ARRAY_BUFFER, bytes, data.data(), vbo->usage);
	} else {
		glBufferData(GL_ARRAY_BUFFER, bytes, nullptr, vbo->usage);
		glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, data.data());
	}
	
	vbo->verticesCount = (uint32_t)size / vbo->dimensions;
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	if(vboIndex == 0)
		vao->verticesCount = vbo->verticesCount;
};

void heVaoBind(HeVao const* vao) {
	glBindVertexArray(vao->vaoId);
	for (uint32_t i = 0; i < (uint32_t)vao->vbos.size(); ++i)
		glEnableVertexAttribArray(i);
};

void heVaoUnbind(HeVao const* vao) {
	for (uint32_t i = 0; i < (uint32_t) vao->vbos.size(); ++i)
		glDisableVertexAttribArray(i);
	glBindVertexArray(0);
};

void heVaoDestroy(HeVao* vao) {
	for (HeVbo& vbos : vao->vbos)
		heVboDestroy(&vbos);
	
	glDeleteVertexArrays(1, &vao->vaoId);
	vao->vaoId = 0;
	vao->verticesCount = 0;
	vao->vbos.clear();
	vao->type = HE_VAO_TYPE_NONE;
	
#ifdef HE_ENABLE_NAMES
	vao->name.clear();
#endif
};

void heVaoRender(HeVao const* vao) {
	glDrawArrays(vao->type, 0, vao->verticesCount);
};


void heFboCreate(HeFbo* fbo) {
	glGenFramebuffers(1, &fbo->fboId);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo->fboId);
	
#ifdef HE_ENABLE_NAMES
	glObjectLabel(GL_FRAMEBUFFER, fbo->fboId, (uint32_t) fbo->name.size(), fbo->name.c_str());
#endif
};

void heFboCreateColourTextureAttachment(HeFbo* fbo, HeColourFormat const format, hm::vec2i const& size) {
	uint32_t f = GL_RGBA;
	uint8_t index = (uint8_t) fbo->colourAttachments.size();
	if(format == HE_COLOUR_FORMAT_RGB8 || format == HE_COLOUR_FORMAT_RGB16)
		f = GL_RGB;
	
	HeFboAttachment* attachment = &fbo->colourAttachments.emplace_back();
	attachment->texture = true;
	attachment->samples = 1;
	attachment->format	= format;
	if(size == hm::vec2i(-1))
		attachment->size = fbo->size;
	else
		attachment->size = size;
	glGenTextures(1, &attachment->id);
	glBindTexture(GL_TEXTURE_2D, attachment->id);
	glTexImage2D(GL_TEXTURE_2D, 0, format, attachment->size.x, attachment->size.y, 0, f, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, attachment->id, 0);

	// memory tracker
	uint32_t perPixel = heMemoryGetBytesPerPixel(format);
	uint32_t bytes = heMemoryRoundUsage(attachment->size.x * attachment->size.y * perPixel);
	//uint32_t bytes = attachment->size.x * attachment->size.y * perPixel;
	attachment->memory = bytes;
	heMemoryTracker[HE_MEMORY_TYPE_FBO] += bytes;
	
#ifdef HE_ENABLE_NAMES
	std::string name = fbo->name + "[" + std::to_string(index + 1) + "]";
	glObjectLabel(GL_TEXTURE, attachment->id, (uint32_t) name.size(), name.c_str());
#endif
};

void heFboCreateColourBufferAttachment(HeFbo* fbo, HeColourFormat const format, uint8_t const samples, hm::vec2i const& size) {
	uint8_t index = (uint8_t) fbo->colourAttachments.size();
	HeFboAttachment* attachment = &fbo->colourAttachments.emplace_back();
	attachment->texture = false;
	attachment->samples = samples;
	attachment->format	= format;
	attachment->size	= size;
	if(size == hm::vec2i(-1))
		attachment->size = fbo->size;
	else
		attachment->size = size;
	glGenRenderbuffers(1, &attachment->id);
	glBindRenderbuffer(GL_RENDERBUFFER, attachment->id);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, attachment->samples, attachment->format, attachment->size.x, attachment->size.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_RENDERBUFFER, attachment->id);
	
	if(samples > 1)
		fbo->useMultisampling = true;

	// memory tracker
	uint32_t perPixel = heMemoryGetBytesPerPixel(format);
	uint32_t bytes = attachment->size.x * attachment->size.y * perPixel * samples;
	attachment->memory = bytes;
	heMemoryTracker[HE_MEMORY_TYPE_FBO] += bytes;
	
#ifdef HE_ENABLE_NAMES
	std::string name = fbo->name + "[" + std::to_string(index) + "]";
	glObjectLabel(GL_RENDERBUFFER, attachment->id, (uint32_t) name.size(), name.c_str());
#endif
};

void heFboCreateDepthBufferAttachment(HeFbo* fbo, uint8_t const samples) {
	fbo->depthAttachment.texture = false;
	fbo->depthAttachment.size	 = fbo->size;
	fbo->depthAttachment.samples = samples;
	
	glGenRenderbuffers(1, &fbo->depthAttachment.id);
	glBindRenderbuffer(GL_RENDERBUFFER, fbo->depthAttachment.id);
	if (samples == 1)
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, fbo->depthAttachment.size.x, fbo->depthAttachment.size.y);
	else {
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, fbo->depthAttachment.samples, GL_DEPTH_COMPONENT24, fbo->depthAttachment.size.x, fbo->depthAttachment.size.y);
		fbo->useMultisampling = true;
	}
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fbo->depthAttachment.id);
	
	// memory tracker
	uint8_t perPixel = 4;
	uint32_t bytes = heMemoryRoundUsage(fbo->depthAttachment.size.x * fbo->depthAttachment.size.y * perPixel * samples);
	fbo->depthAttachment.memory = bytes;
	heMemoryTracker[HE_MEMORY_TYPE_FBO] += bytes;
	
#ifdef HE_ENABLE_NAMES
	std::string name = fbo->name + "[depth]";
	glObjectLabel(GL_RENDERBUFFER, fbo->depthAttachment.id, (uint32_t) name.size(), name.c_str());
#endif
};

void heFboCreateDepthTextureAttachment(HeFbo* fbo) {
	fbo->depthAttachment.texture = true;
	fbo->depthAttachment.size	 = fbo->size;
	fbo->depthAttachment.samples = 1;
	
	glGenTextures(1, &fbo->depthAttachment.id);
	glBindTexture(GL_TEXTURE_2D, fbo->depthAttachment.id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, fbo->depthAttachment.size.x, fbo->depthAttachment.size.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, fbo->depthAttachment.id, 0);

	// memory tracker
	uint32_t perPixel = 3;
	uint32_t bytes = heMemoryRoundUsage(fbo->size.x * fbo->size.y * perPixel);
	fbo->depthAttachment.memory = bytes;
	heMemoryTracker[HE_MEMORY_TYPE_FBO] += bytes;
	
#ifdef HE_ENABLE_NAMES
	std::string name = fbo->name + "[depth]";
	glObjectLabel(GL_TEXTURE, fbo->depthAttachment.id, (uint32_t) name.size(), name.c_str());
#endif
};

void heFboBind(HeFbo* fbo) {
	glBindFramebuffer(GL_FRAMEBUFFER, fbo->fboId);
	glViewport(0, 0, fbo->size.x, fbo->size.y);
	
	if (fbo->useMultisampling)
		glEnable(GL_MULTISAMPLE);
	
	// enable draw buffers
	int8_t count = (int8_t) fbo->colourAttachments.size();
	std::vector<uint32_t> drawBuffers;
	for (int8_t i = 0; i < count; ++i)
		drawBuffers.emplace_back(GL_COLOR_ATTACHMENT0 + i);
	
	glDrawBuffers(count, drawBuffers.data());
};

void heFboUnbind(hm::vec2i const& windowSize) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, windowSize.x, windowSize.y);
};

void heFboUnbind() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
};

void heFboDestroy(HeFbo* fbo) {
	//glDeleteTextures(1, &fbo->depthTextureAttachment);
	//glDeleteRenderbuffers(1, &fbo->depthBufferAttachment);
	for(auto& all : fbo->colourAttachments) {
		heMemoryTracker[HE_MEMORY_TYPE_FBO] -= all.memory;
		if(all.texture)
			glDeleteTextures(1, &all.id);
		else
			glDeleteRenderbuffers(1, &all.id);
		all.id = 0;
	}
	glDeleteFramebuffers(1, &fbo->fboId);
	fbo->fboId = 0;
};

void heFboResize(HeFbo* fbo, hm::vec2i const& newSize) {
	// manually destroy fbo without overwriting any data

	uint8_t depthAttachment = 0;
	if(fbo->depthAttachment.id > 0) {
		heMemoryTracker[HE_MEMORY_TYPE_FBO] -= fbo->depthAttachment.memory;
		if(fbo->depthAttachment.texture)
			glDeleteTextures(1, &fbo->depthAttachment.id);
		else
			glDeleteRenderbuffers(1, &fbo->depthAttachment.id);
		depthAttachment = fbo->depthAttachment.texture + 1;
	}
	
	for(auto& all : fbo->colourAttachments) {
		heMemoryTracker[HE_MEMORY_TYPE_FBO] -= all.memory;
		if(all.texture)
			glDeleteTextures(1, &all.id);
		else
			glDeleteRenderbuffers(1, &all.id);
		all.id = 0;
	}
	glDeleteFramebuffers(1, &fbo->fboId);
	
	hm::vec2f scaleFactor = hm::vec2f(newSize) / fbo->size;
	fbo->size = newSize;
	std::vector<HeFboAttachment> attachments = fbo->colourAttachments;
	fbo->colourAttachments.clear();
	
	heFboCreate(fbo);
	for(HeFboAttachment& all : attachments) {
		if(all.resize)
			all.size = hm::vec2i(scaleFactor * all.size);
		
		if(all.texture)
			heFboCreateColourTextureAttachment(fbo, all.format, all.size);
		else
			heFboCreateColourBufferAttachment(fbo, all.format, all.samples, all.size);
	}
	
	if(depthAttachment == 1)
		heFboCreateDepthBufferAttachment(fbo);
	if(depthAttachment == 2)
		heFboCreateDepthTextureAttachment(fbo);
};

void heFboRender(HeFbo* sourceFbo, HeFbo* targetFbo) {
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, targetFbo->fboId);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, sourceFbo->fboId);
	//glDrawBuffer(GL_BACK); (Invalid operation?)
	glBlitFramebuffer(0, 0, sourceFbo->size.x, sourceFbo->size.y, 0, 0, targetFbo->size.x, targetFbo->size.y, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	heFboUnbind();
};

void heFboRender(HeFbo* sourceFbo, hm::vec2i const& windowSize) {
	HeFbo fake;
	fake.fboId = 0;
	fake.size = windowSize;
	heFboRender(sourceFbo, &fake);
};

void heFboValidate() {
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		HE_ERROR("Fbo not set up correctly (" + std::to_string(glGetError()) + ")");
};


// --- Textures

void heTextureCreateEmptyCubeMap(HeTexture* texture) {
	texture->cubeMap = true;
	glGenTextures(1, &texture->textureId);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture->textureId);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, texture->mipMapCount);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
	glTexStorage2D(GL_TEXTURE_CUBE_MAP, (texture->parameters & HE_TEXTURE_FILTER_TRILINEAR) ? texture->mipMapCount : 1, texture->format, texture->size.x, texture->size.y);

	if(texture->parameters & HE_TEXTURE_CLAMP_EDGE)
		heTextureClampEdge(texture);
	if(texture->parameters & HE_TEXTURE_CLAMP_BORDER)
		heTextureClampBorder(texture);
	if(texture->parameters & HE_TEXTURE_CLAMP_REPEAT)
		heTextureClampRepeat(texture);

	if(texture->parameters & HE_TEXTURE_FILTER_LINEAR)
		heTextureFilterLinear(texture);
	if(texture->parameters & HE_TEXTURE_FILTER_BILINEAR)
		heTextureFilterBilinear(texture);
	if(texture->parameters & HE_TEXTURE_FILTER_TRILINEAR)
		heTextureFilterTrilinear(texture, texture->mipMapCount);
	if(texture->parameters & HE_TEXTURE_FILTER_ANISOTROPIC)
		heTextureFilterAnisotropic(texture);
	
	// add to memory tracker
	uint8_t perPixel = heMemoryGetBytesPerPixel(texture->format);
	uint32_t bytes = (texture->size.x * texture->size.y * perPixel) * 6;
	texture->memory = bytes;
	heMemoryTracker[HE_MEMORY_TYPE_TEXTURE] += bytes;
	
	texture->referenceCount = 1;
#ifdef HE_ENABLE_NAMES
	if(!texture->name.empty())
		glObjectLabel(GL_TEXTURE, texture->textureId, -1, texture->name.c_str());
#endif
};

void heTextureCreateEmpty(HeTexture* texture) {
	heTextureCreateFromBuffer(texture);
};

void heTextureLoadFromFile(HeTexture* texture, std::string const& fileName) {
	unsigned char* buffer = nullptr;
	
	//#ifdef HE_USE_STBI
	if(fileName.find("bin") == std::string::npos) {
		stbi_set_flip_vertically_on_load(true);
		buffer = stbi_load(fileName.c_str(), &texture->size.x, &texture->size.y, &texture->channels, 0);
	} else //#else
		buffer = heBinaryLoadTexture(fileName, &texture->size.x, &texture->size.y, &texture->channels);
	//#endif
	
	if(!buffer) {
		HE_ERROR("Could not open texture [" + fileName + "]!");
		texture->textureId = 0;
		return;
	}
	
	if(texture->channels == 4)
		texture->format = HE_COLOUR_FORMAT_RGBA8;
	else
		texture->format = HE_COLOUR_FORMAT_RGB8;
	
	texture->bufferc = buffer;
	if (heIsMainThread()) {
#ifdef HE_ENABLE_NAMES
		texture->name = fileName;
#endif
		heTextureCreateFromBuffer(texture);
	} else {
		// no context in the current thread, add it to the thread loader
		heThreadLoaderRequestTexture(texture);
	}
};

void heTextureLoadHdrFromFile(HeTexture* texture, std::string const& fileName) {
	stbi_set_flip_vertically_on_load(true);
	float* buffer = stbi_loadf(fileName.c_str(), &texture->size.x, &texture->size.y, &texture->channels, 4);
	if(!buffer) {
		HE_ERROR("Could not open texture [" + fileName + "]!");
		texture->textureId = 0;
		return;
	}
	
	texture->channels = 4;
	texture->format	  = HE_COLOUR_FORMAT_RGBA16;
	
	texture->bufferf = buffer;
	if(heIsMainThread()) {
#ifdef HE_ENABLE_NAMES
		texture->name = fileName;
#endif
		heTextureCreateFromBuffer(texture);
	} else {
		// no context in the current thread, add it to the thread loader
		heThreadLoaderRequestTexture(texture);
	}
};

void heTextureLoadFromFile(HeTexture* texture, FILE* stream) {
	stbi_set_flip_vertically_on_load(true);
	unsigned char* buffer = stbi_load_from_file(stream, &texture->size.x, &texture->size.y, &texture->channels, 0);
	if(buffer == nullptr) {
		HE_ERROR("Could not open texture from FILE!");
		texture->textureId = 0;
		return;
	}
	
	if(texture->channels == 4)
		texture->format = HE_COLOUR_FORMAT_RGBA8;
	else
		texture->format = HE_COLOUR_FORMAT_RGB8;
	
	texture->bufferc = buffer;
	if(heIsMainThread()) {
		// TODO(Victor): Texture FILE* naming
		heTextureCreateFromBuffer(texture);
	} else {
		// no context in the current thread, add it to the thread loader
		heThreadLoaderRequestTexture(texture);
	}
};

void heTextureCreateFromBuffer(HeTexture* texture) {
	glGenTextures(1, &texture->textureId);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture->textureId);
	if(texture->format == HE_COLOUR_FORMAT_RGBA16) {
		glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
		glTexImage2D(GL_TEXTURE_2D, 0, texture->format, texture->size.x, texture->size.y, 0, (texture->channels == 4) ? GL_RGBA : GL_RGB, GL_FLOAT, texture->bufferf);
		
		//#ifdef HE_USE_STBI
		//stbi_image_free(texture->bufferf);
		//#else
		free(texture->bufferf);
		//#endif
		texture->bufferf = nullptr;
	} else {
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, texture->format, texture->size.x, texture->size.y, 0, (texture->channels == 4) ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, texture->bufferc);
		
		//#ifdef HE_USE_STBI
		//stbi_image_free(texture->bufferc);
		//#else
		free(texture->bufferc);
		//#endif
		texture->bufferc = nullptr;
	}

	if(texture->parameters & HE_TEXTURE_CLAMP_EDGE)
		heTextureClampEdge(texture);
	if(texture->parameters & HE_TEXTURE_CLAMP_BORDER)
		heTextureClampBorder(texture);
	if(texture->parameters & HE_TEXTURE_CLAMP_REPEAT)
		heTextureClampRepeat(texture);

	if(texture->parameters & HE_TEXTURE_FILTER_LINEAR)
		heTextureFilterLinear(texture);
	if(texture->parameters & HE_TEXTURE_FILTER_BILINEAR)
		heTextureFilterBilinear(texture);
	if(texture->parameters & HE_TEXTURE_FILTER_TRILINEAR)
		heTextureFilterTrilinear(texture, texture->mipMapCount);
	if(texture->parameters & HE_TEXTURE_FILTER_ANISOTROPIC)
		heTextureFilterAnisotropic(texture);
	
	// memory tracker
	uint8_t perPixel = heMemoryGetBytesPerPixel(texture->format);
	uint32_t bytes = heMemoryRoundUsage(texture->size.x * texture->size.y * perPixel);
	if(texture->parameters & HE_TEXTURE_FILTER_TRILINEAR)
		bytes = (uint32_t) (bytes * 1.33);
	texture->memory = bytes;
	heMemoryTracker[HE_MEMORY_TYPE_TEXTURE] += bytes;
	texture->referenceCount = 1;
#ifdef HE_ENABLE_NAMES
	glObjectLabel(GL_TEXTURE, texture->textureId, (uint32_t) texture->name.size(), texture->name.c_str());
#endif
};

void heTextureBind(HeTexture const* texture, int8_t const slot) {
	if(texture != nullptr) {
		glActiveTexture(GL_TEXTURE0 + slot);
		uint32_t format = (texture->cubeMap) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
		glBindTexture(format, texture->textureId);
	} else {
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}
};

void heTextureBind(uint32_t const texture, int8_t const slot, b8 const cubeMap) {
	uint32_t format = (cubeMap) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(format, texture);
};

void heImageTextureBind(HeTexture const* texture, int8_t const slot, int8_t const level, int8_t const layer, HeAccessType const access) {
	if(texture != nullptr)
		glBindImageTexture(slot, texture->textureId, level, (layer == -1) ? GL_TRUE : GL_FALSE, (layer == -1) ? 0 : layer, access, texture->format);
};

void heTextureUnbind(int8_t const slot, b8 const cubeMap) {
	uint32_t format = (cubeMap) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(format, 0);
};

void heTextureDestroy(HeTexture* texture) {
	if(--texture->referenceCount == 0) {
		heMemoryTracker[HE_MEMORY_TYPE_TEXTURE] -= texture->memory;
		glDeleteTextures(1, &texture->textureId);
		texture->textureId = 0;
		texture->size = hm::vec2i(0);
		texture->channels  = 0;
		texture->memory	   = 0;
	}
};

void heTextureFilterLinear(HeTexture const* texture) {
	uint32_t format = (texture->cubeMap) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
	
	glTexParameteri(format, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(format, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
};

void heTextureFilterBilinear(HeTexture const* texture) {
	uint32_t format = (texture->cubeMap) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

	glTexParameteri(format, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(format, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
};

void heTextureFilterTrilinear(HeTexture const* texture, uint16_t const count) {
	uint32_t format = (texture->cubeMap) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

	glTexParameteri(format, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(format, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	if(!texture->cubeMap) {
		glTexParameteri(format, GL_TEXTURE_MAX_LEVEL, count);
		glTexParameteri(format, GL_TEXTURE_BASE_LEVEL, 0);
		glGenerateMipmap(format);
	}
};

void heTextureFilterAnisotropic(HeTexture const* texture) {
	uint32_t format = (texture->cubeMap) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

	float value;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &value);
	const float amount = std::fmin(4.0f, value);
	glTexParameterf(format, GL_TEXTURE_MAX_ANISOTROPY_EXT, amount);
};

void heTextureClampEdge(HeTexture const* texture) {
	uint32_t format = (texture->cubeMap) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

	glTexParameteri(format, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(format, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	if(texture->cubeMap)
		glTexParameteri(format, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
};

void heTextureClampBorder(HeTexture const* texture) {
	uint32_t format = (texture->cubeMap) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

	glTexParameteri(format, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(format, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	if(texture->cubeMap)
		glTexParameteri(format, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
};

void heTextureClampRepeat(HeTexture const* texture) {
	uint32_t format = (texture->cubeMap) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

	glTexParameteri(format, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(format, GL_TEXTURE_WRAP_T, GL_REPEAT);
	if(texture->cubeMap)
		glTexParameteri(format, GL_TEXTURE_WRAP_R, GL_REPEAT);
};


// --- Utils

void heFrameClear(hm::colour const& colour, HeFrameBufferBits const type) {
	glClearColor(getR(&colour), getG(&colour), getB(&colour), getA(&colour));
	glClear(type);
};

void heBlendMode(int8_t const mode) {
	if(mode == -1) {
		glDisable(GL_BLEND);
	} else {
		glEnable(GL_BLEND);
		switch (mode) {
		case 0:
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			break;
			
		case 1:
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			break;
			
		case 2:
			glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
			break;
		}
	}
};

void heBufferBlendMode(int8_t const attachmentIndex, int8_t const mode) {
	if(mode == -1)
		glDisable(GL_BLEND);
	else {
		glEnable(GL_BLEND);
		switch (mode) {
		case 0:
			glBlendFunciARB(attachmentIndex, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			break;
			
		case 1:
			glBlendFunciARB(attachmentIndex, GL_SRC_ALPHA, GL_ONE);
			break;
			
		case 2:
			glBlendFunciARB(attachmentIndex, GL_SRC_ALPHA, GL_ZERO);
			break;
		}
	}
};

void heDepthEnable(b8 const depth) {
	if (depth)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);
};

void heDepthFunc(HeFragmentTestFunction const function) {
	glDepthFunc(function);
};

void heCullEnable(b8 const culling) {
	if(culling) {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	} else
		glDisable(GL_CULL_FACE);
};

void heStencilEnable(b8 const depth) {
	if(depth)
		glEnable(GL_STENCIL_TEST);
	else
		glDisable(GL_STENCIL_TEST);
};

void heStencilMask(uint32_t const mask) {
	glStencilMask(mask);
};

void heStencilFunc(HeFragmentTestFunction const function, uint32_t const threshold, uint32_t const mask) {
	glStencilFunc(function, threshold, mask);
};

void heViewport(hm::vec2i const& lowerleft, hm::vec2i const& size) {
	glViewport(lowerleft.x, lowerleft.y, size.x, size.y);
};

int32_t heMemoryGetUsage() {
	GLint x = 0, y = 0, z = 0;
	glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX,&x);
	glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX,&y);
	glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX,&z);
	//HE_LOG("X = " + std::to_string(x) + "/" + std::to_string(y) + "/" + std::to_string(z));
	//txt+=AnsiString().sprintf("GPU memory: %i/%i/%i MByte\r\n",x>>10,y>>10,z>>10); // GPU free/GPU total/GPU+CPU shared total
	//x=0; glGetIntegerv(GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX,&x);
	//y=0; glGetIntegerv(GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX,&y);
	//txt+=AnsiString().sprintf("GPU blocks: %i used: %i MByte\r\n",x,y>>10);
	return (z) * 1000;
};

void heGlPrintInfo() {
	std::string vendor, renderer, version;
	vendor	 = std::string((const char*) glGetString(GL_VENDOR));
	renderer = std::string((const char*) glGetString(GL_RENDERER));
	version	 = std::string((const char*) glGetString(GL_VERSION));
	HE_LOG("Engine running on " + vendor + ", " + renderer + " on version " + version);
};


#ifdef HE_ENABLE_ERROR_CHECKING

std::vector<uint32_t> errors;

void heGlErrorClear() {
	while(glGetError() != GL_NO_ERROR) {}
};

void heGlErrorSaveAll() {
	errors.clear();
	uint32_t error;
	while((error = (uint32_t) glGetError()) != GL_NO_ERROR)
		errors.emplace_back(error);
};

uint32_t heGlErrorGet() {
	if(errors.size() > 0) {
		uint32_t e = errors[0];
		errors.erase(errors.begin());
		return e;
	} else
		return 0;
};

uint32_t heGlErrorCheck() {
	return glGetError();
};

#else

void heGlErrorClear()		  {};
void heGlErrorSaveAllErrors() {};
uint32_t heGlErrorGet()		  { return 0; };
uint32_t heGlErrorCheck()	  { return 0; };

#endif
