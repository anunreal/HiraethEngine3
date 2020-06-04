#include "heGlLayer.h"
#include "heAssets.h"
#include "GLEW/glew.h"
#include "GLEW/wglew.h"
#include "heWindow.h"
#include "heCore.h"
#include "heWin32Layer.h"
#include "heDebugUtils.h"
#include "heBinary.h"
#include "heUtils.h"
#include <fstream>
#include <algorithm>

#define GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX 0x9048
#define GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX 0x9049

#define STB_IMAGE_IMPLEMENTATION
#pragma warning(push, 0)
#include "..\stb_image.h"
#pragma warning(pop)

float maxAnisotropicValue = -1.f;


// -- ubos

void heUboCreate(HeUbo* ubo) {
    glGenBuffers(1, &ubo->uboId);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo->uboId);
    
    if(ubo->totalSize > 0) {
        // round size to next multiple of 16
        int x = (int) std::ceil(ubo->totalSize / 16.f);
        ubo->totalSize = x * 16;
        
        ubo->buffer = (unsigned char*) malloc(ubo->totalSize);
        glBufferData(GL_UNIFORM_BUFFER, ubo->totalSize, ubo->buffer, GL_DYNAMIC_DRAW);
    }
};

void heUboAllocate(HeUbo* ubo, std::string const& variable, uint32_t const size) {
    ubo->variableOffset[variable] = ubo->totalSize;
    ubo->variableSize[variable] = size;
    ubo->totalSize += size;
};

void heUboUploadData(HeUbo const* ubo) {
    glBindBuffer(GL_UNIFORM_BUFFER, ubo->uboId);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, ubo->totalSize, ubo->buffer);
};

void heUboUpdateVariable(HeUbo* ubo, std::string const& variable, void const* ptr) {
    uint32_t offset = ubo->variableOffset[variable];
    uint32_t size   = ubo->variableSize[variable];
    memcpy(ubo->buffer + offset, (unsigned char*) ptr, size); 
};

void heUboUpdateData(HeUbo* ubo, uint32_t const offset, uint32_t const size, void const* ptr) {
    memcpy(ubo->buffer + offset, (unsigned char*) ptr, size);
};

void heUboBind(HeUbo const* ubo, uint32_t const location) {
    glBindBufferBase(GL_UNIFORM_BUFFER, location, ubo->uboId);
};

void heUboDestroy(HeUbo* ubo) {
    free(ubo->buffer);
    glDeleteBuffers(1, &ubo->uboId);
    ubo->uboId     = 0;
    ubo->totalSize = 0;
    ubo->variableOffset.clear();
    ubo->variableSize.clear();
};


// --- Shaders

std::string heShaderLoadSource(std::string const& file, std::vector<std::string>& includeFiles);

// loads the source code of all include files specified in source and replaces the #include line with that code
void heShaderLoadIncludeFiles(std::string& source, std::vector<std::string>& includeFiles) {
    size_t index = source.find('#');
    while(index != std::string::npos) {
        std::string line = source.substr(index);
        line = line.substr(0, line.find('\n'));
        if(heStringStartsWith(line, "#include")) {
            // load include file
            std::string file = line.substr(9);
            heStringEatSpacesLeft(file);
            heStringEatSpacesRight(file);
            // cut quotation marks
            file = file.substr(1, file.size() - 2);
            source.erase(index, line.size());
            source.insert(index, heShaderLoadSource(file, includeFiles));
            includeFiles.emplace_back(file);
        }
        
        index = source.find('#', index + 1);
    }
};

std::string heShaderLoadSource(std::string const& file, std::vector<std::string>& includeFiles) {
    HE_LOG("Loading shader file [" + file + "]");
    HeTextFile stream;
    stream.skipEmptyLines = false;
    heTextFileOpen(&stream, file, 4096, false);
    if(!stream.open)
        return "";
    
    std::string source;
    heTextFileGetContent(&stream, &source);

    // check for include files
    heShaderLoadIncludeFiles(source, includeFiles);    
    
    heTextFileClose(&stream);
    return source;
};

std::unordered_map<HeShaderType, std::string> heShaderLoadSourceAll(std::string const& file, std::vector<std::string>& includeFiles) {
    HE_LOG("Loading shader [" + file + "]");
    HeTextFile stream;
    stream.skipEmptyLines = false;
    heTextFileOpen(&stream, file, 4096, false);
    if(!stream.open)
        return std::unordered_map<HeShaderType, std::string>();
    
    std::unordered_map<HeShaderType, std::string> sourceMap;    
    std::string line;
    
    HeShaderType currentType;
    while (heTextFileGetLine(&stream, &line)) {
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

    heShaderLoadIncludeFiles(sourceMap[HE_SHADER_TYPE_VERTEX], includeFiles);
    if(sourceMap.find(HE_SHADER_TYPE_GEOMETRY) != sourceMap.end())
        heShaderLoadIncludeFiles(sourceMap[HE_SHADER_TYPE_GEOMETRY], includeFiles);    
    heShaderLoadIncludeFiles(sourceMap[HE_SHADER_TYPE_FRAGMENT], includeFiles);    
    
    heTextFileClose(&stream);
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

uint32_t heShaderLoadFromFile(std::string const& file, uint32_t type, std::vector<std::string>& includeFiles) {
    std::string source = heShaderLoadSource(file, includeFiles);
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
    uint32_t sid = heShaderLoadFromFile(computeShader, GL_COMPUTE_SHADER, program->includeFiles);
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
    program->name = computeShader;
    glObjectLabel(GL_PROGRAM, program->programId, (uint32_t) program->name.size(), program->name.c_str());
#endif
};

void heShaderLoadProgram(HeShaderProgram* program, std::string const& vertexShader, std::string const& fragmentShader) {
    uint32_t vs = heShaderLoadFromFile(vertexShader, GL_VERTEX_SHADER, program->includeFiles);
    uint32_t fs = heShaderLoadFromFile(fragmentShader, GL_FRAGMENT_SHADER, program->includeFiles);
    
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
    uint32_t vs = heShaderLoadFromFile(vertexShader, GL_VERTEX_SHADER, program->includeFiles);
    uint32_t gs = heShaderLoadFromFile(geometryShader, GL_GEOMETRY_SHADER, program->includeFiles);
    uint32_t fs = heShaderLoadFromFile(fragmentShader, GL_FRAGMENT_SHADER, program->includeFiles);
    
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
    std::unordered_map<HeShaderType, std::string> source = heShaderLoadSourceAll(file, program->includeFiles);
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
            float   fdata[16];
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
    program->includeFiles.clear();
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
    

    for(std::string const& files : program->includeFiles) {
        if(heWin32FileModified(files)) { // TODO(Victor): Add Platform abstraction!
            needsReloading = true;
            break;
        }
    }

    if(needsReloading)
        heShaderReload(program);
#endif
};


int32_t heShaderGetUniformLocation(HeShaderProgram* program, std::string const& uniform) {
    int32_t location;
    auto it = program->uniforms.find(uniform); 
    if (it != program->uniforms.end()) {
        location = it->second;
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

int32_t heShaderGetUboLocation(HeShaderProgram* program, std::string const& ubo) {
    int32_t location;
    auto it = program->ubos.find(ubo);
    if (it != program->ubos.end())
        location = it->second;
    else {
        location = glGetUniformBlockIndex(program->programId, ubo.c_str());
        program->ubos[ubo] = location;
        
        if(location == -1) {
#ifdef HE_ENABLE_NAMES
            HE_DEBUG("Could not find ubo [" + ubo + "] in shader [" + program->name + "]");
#else
            HE_DEBUG("Could not find ubo [" + ubo + "]");
#endif
        }
    }
    
    return location;
};

int32_t heShaderGetSamplerLocation(HeShaderProgram* program, std::string const& sampler, int8_t const requestedSlot) {
    int32_t location;
    auto it = program->samplers.find(sampler);
    if (it != program->samplers.end()) {
        location = it->second;
    } else {
        int32_t uloc = glGetUniformLocation(program->programId, sampler.c_str());
        uint8_t slot = requestedSlot;
        if(requestedSlot == -1)
            slot = (uint8_t) program->samplers.size();

        glProgramUniform1i(program->programId, uloc, slot);
        program->samplers[sampler] = slot;
        location = slot;
        
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
    vbo->usage         = usage;
    vbo->dimensions    = dimensions;
    vbo->verticesCount = (uint32_t)data.size() / vbo->dimensions;
    vbo->type          = HE_DATA_TYPE_FLOAT;
    vbo->memory        = size;
    heMemoryTracker[HE_MEMORY_TYPE_VAO] += vbo->memory;
};

void heVboCreateInt(HeVbo* vbo, std::vector<int32_t> const& data, uint8_t const dimensions, HeVboUsage const usage) {
    uint32_t size = (uint32_t) data.size() * sizeof(int32_t);
    glGenBuffers(1, &vbo->vboId);
    glBindBuffer(GL_ARRAY_BUFFER, vbo->vboId);
    glBufferData(GL_ARRAY_BUFFER, (uint32_t)data.size() * sizeof(int32_t), data.data(), usage);
    vbo->usage         = usage;
    vbo->dimensions    = dimensions;
    vbo->verticesCount = (uint32_t)data.size() / vbo->dimensions;
    vbo->type          = HE_DATA_TYPE_INT;
    vbo->memory        = size;
    heMemoryTracker[HE_MEMORY_TYPE_VAO] += vbo->memory;
};

void heVboCreateUint(HeVbo* vbo, std::vector<uint32_t> const& data, uint8_t const dimensions, HeVboUsage const usage) {
    uint32_t size = (uint32_t) data.size() * sizeof(uint32_t);
    glGenBuffers(1, &vbo->vboId);
    glBindBuffer(GL_ARRAY_BUFFER, vbo->vboId);
    glBufferData(GL_ARRAY_BUFFER, (uint32_t)data.size() * sizeof(int32_t), data.data(), usage);
    vbo->usage         = usage;
    vbo->dimensions    = dimensions;
    vbo->verticesCount = (uint32_t)data.size() / vbo->dimensions;
    vbo->type          = HE_DATA_TYPE_UINT;
    vbo->memory        = size;
    heMemoryTracker[HE_MEMORY_TYPE_VAO] += vbo->memory;
};

void heVboAllocate(HeVbo* vbo, uint32_t const size, const uint8_t dimensions, HeVboUsage const usage, HeDataType const type) {
    uint32_t sizeBytes = (uint32_t) size * (type == HE_DATA_TYPE_FLOAT ? sizeof(float) : sizeof(int32_t));
    glGenBuffers(1, &vbo->vboId);
    glBindBuffer(GL_ARRAY_BUFFER, vbo->vboId);
    
    if(size > 0)
        glBufferData(GL_ARRAY_BUFFER, sizeBytes, nullptr, usage);
    
    vbo->usage         = usage;
    vbo->dimensions    = dimensions;
    vbo->verticesCount = 0;
    vbo->type          = type;
    vbo->memory        = sizeBytes;
    heMemoryTracker[HE_MEMORY_TYPE_VAO] += vbo->memory;
};

void heVboDestroy(HeVbo* vbo) {
    heMemoryTracker[HE_MEMORY_TYPE_VAO] -= vbo->memory;
    glDeleteBuffers(1, &vbo->vboId);
    vbo->vboId         = 0;
    vbo->verticesCount = 0;
    vbo->dimensions    = 0;
    vbo->type          = HE_DATA_TYPE_NONE;
    vbo->memory        = 0;
};

void heVaoCreate(HeVao* vao, HeVaoType const type) {
    HE_CRASH_LOG();
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
    HE_CRASH_LOG();
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

    if(vbo->instanced)
        glVertexAttribDivisor(attributeIndex, 1);
        
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
    HE_CRASH_LOG();
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

void heVaoAddInstancedVbo(HeVao* vao, HeVbo* vbo) {
    uint32_t attributeIndex = (uint32_t) vao->vbos.size();

    if(vbo->type == HE_DATA_TYPE_FLOAT)
        glVertexAttribPointer(attributeIndex, vbo->dimensions, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    else
        glVertexAttribIPointer(attributeIndex, vbo->dimensions, vbo->type, 0, (GLvoid*)0);

    vao->vbos.emplace_back(*vbo);
    glVertexAttribDivisor(attributeIndex, 1);
    
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
        vbo->dataf      = data;
        vbo->dimensions = dimensions;
        vbo->usage      = usage;
        vbo->type       = HE_DATA_TYPE_FLOAT;
    }
};

void heVaoAddDataInt(HeVao* vao, std::vector<int32_t> const& data, uint8_t const dimensions, HeVboUsage const usage) {
    if(heIsMainThread()) {
        HeVbo vbo;
        heVboCreateInt(&vbo, data, dimensions, usage);
        heVaoAddVbo(vao, &vbo);
    } else {
        HeVbo* vbo = &vao->vbos.emplace_back();
        vbo->datai      = data;
        vbo->dimensions = dimensions;
        vbo->usage      = usage;
        vbo->type       = HE_DATA_TYPE_INT;
    }
};

void heVaoAddDataUint(HeVao* vao, std::vector<uint32_t> const& data, uint8_t const dimensions, HeVboUsage const usage) {
    if(heIsMainThread()) {
        HeVbo vbo;
        heVboCreateUint(&vbo, data, dimensions, usage);
        heVaoAddVbo(vao, &vbo);
    } else {
        HeVbo* vbo = &vao->vbos.emplace_back();
        vbo->dataui     = data;
        vbo->dimensions = dimensions;
        vbo->usage      = usage;
        vbo->type       = HE_DATA_TYPE_UINT;
    }
};

void heVaoAddInstancedData(HeVao* vao, std::vector<float> const& data, uint8_t const dimensions, HeVboUsage const usage) {
    if(heIsMainThread()) {
        HeVbo vbo;
        heVboCreate(&vbo, data, dimensions, usage);
        heVaoAddInstancedVbo(vao, &vbo);
    } else {
        HeVbo* vbo = &vao->vbos.emplace_back();
        vbo->dataf      = data;
        vbo->dimensions = dimensions;
        vbo->usage      = usage;
        vbo->type       = HE_DATA_TYPE_FLOAT;
        vbo->instanced  = true;
    }
};

void heVaoAllocate(HeVao* vao, uint32_t const size, uint32_t const dimensions, HeVboUsage const usage, HeDataType const type) {
    HeVbo vbo;
    heVboAllocate(&vbo, size, dimensions, usage, type);
    heVaoAddVbo(vao, &vbo);
};

void heVaoAllocateInstanced(HeVao* vao, uint32_t const size, uint32_t const dimensions, HeVboUsage const usage, HeDataType const type) {
    HeVbo vbo;
    heVboAllocate(&vbo, size, dimensions, usage, type);
    heVaoAddInstancedVbo(vao, &vbo);
};

void heVaoUpdateData(HeVao* vao, std::vector<float> const& data, uint8_t const vboIndex) {
    HE_CRASH_LOG();
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
    
    if(vboIndex == 0)
        vao->verticesCount = vbo->verticesCount;
};

void heVaoUpdateDataInt(HeVao* vao, std::vector<int32_t> const& data, uint8_t const vboIndex) {
    HE_CRASH_LOG();
    
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
    HE_CRASH_LOG();
    
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
    HE_CRASH_LOG();
    glBindVertexArray(vao->vaoId);
    for (uint32_t i = 0; i < (uint32_t)vao->vbos.size(); ++i)
        glEnableVertexAttribArray(i);
};

void heVaoUnbind(HeVao const* vao) {
    HE_CRASH_LOG();
    for (uint32_t i = 0; i < (uint32_t) vao->vbos.size(); ++i)
        glDisableVertexAttribArray(i);
    glBindVertexArray(0);
};

void heVaoDestroy(HeVao* vao) {
    HE_CRASH_LOG();
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
    HE_CRASH_LOG();
    glDrawArrays(vao->type, 0, vao->verticesCount);
};

void heVaoRenderInstanced(HeVao const* vao, uint32_t const count) {
    HE_CRASH_LOG();
    glDrawArraysInstanced(vao->type, 0, vao->verticesCount, count);
};


void heFboCreate(HeFbo* fbo) {
    glGenFramebuffers(1, &fbo->fboId);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo->fboId);
    
#ifdef HE_ENABLE_NAMES
    glObjectLabel(GL_FRAMEBUFFER, fbo->fboId, (uint32_t) fbo->name.size(), fbo->name.c_str());
#endif
};

void heFboCreateColourTextureAttachment(HeFbo* fbo, HeColourFormat const format, hm::vec2i const& size, uint16_t const mipCount) {
    uint32_t f = GL_RGBA;
    uint8_t index = (uint8_t) fbo->colourAttachments.size();
    if(format == HE_COLOUR_FORMAT_RGB8 || format == HE_COLOUR_FORMAT_RGB16)
        f = GL_RGB;
    
    HeFboAttachment* attachment = &fbo->colourAttachments.emplace_back();
    attachment->texture  = true;
    attachment->samples  = 1;
    attachment->format   = format;
    attachment->mipCount = mipCount;
    
    if(size == hm::vec2i(-1))
        attachment->size = fbo->size;
    else
        attachment->size = size;

    glGenTextures(1, &attachment->id);
    glBindTexture(GL_TEXTURE_2D, attachment->id);
    glTexImage2D(GL_TEXTURE_2D, 0, format, attachment->size.x, attachment->size.y, 0, f, GL_FLOAT, 0);
    if(mipCount == 0) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipCount - 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
        
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, attachment->id, 0);

    // memory tracker
    uint32_t perPixel = heColourFormatGetBytesPerPixel(format);
    uint32_t bytes = hm::ceilPowerOfTwo(attachment->size.x * attachment->size.y * perPixel);
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
    attachment->format  = format;
    attachment->size    = size;
    if(size == hm::vec2i(-1))
        attachment->size = fbo->size;

    glGenRenderbuffers(1, &attachment->id);
    glBindRenderbuffer(GL_RENDERBUFFER, attachment->id);
    if(samples > 1) {
        GL_CALL(glRenderbufferStorageMultisample(GL_RENDERBUFFER, attachment->samples, attachment->format, attachment->size.x, attachment->size.y));
        fbo->useMultisampling = true;
    } else
        glRenderbufferStorage(GL_RENDERBUFFER, attachment->format, attachment->size.x, attachment->size.y);

    
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_RENDERBUFFER, attachment->id);
    
    // memory tracker
    uint32_t perPixel = heColourFormatGetBytesPerPixel(format);
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
    fbo->depthAttachment.size    = fbo->size;
    fbo->depthAttachment.samples = samples;
    
    glGenRenderbuffers(1, &fbo->depthAttachment.id);
    glBindRenderbuffer(GL_RENDERBUFFER, fbo->depthAttachment.id);
    if (samples > 1) {
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, fbo->depthAttachment.samples, GL_DEPTH_COMPONENT24, fbo->depthAttachment.size.x, fbo->depthAttachment.size.y);
        fbo->useMultisampling = true;
    } else
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, fbo->depthAttachment.size.x, fbo->depthAttachment.size.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fbo->depthAttachment.id);
    
    // memory tracker
    uint8_t perPixel = 4;
    uint32_t bytes = hm::ceilPowerOfTwo(fbo->depthAttachment.size.x * fbo->depthAttachment.size.y * perPixel * samples);
    fbo->depthAttachment.memory = bytes;
    heMemoryTracker[HE_MEMORY_TYPE_FBO] += bytes;
    
#ifdef HE_ENABLE_NAMES
    std::string name = fbo->name + "[depth]";
    glObjectLabel(GL_RENDERBUFFER, fbo->depthAttachment.id, (uint32_t) name.size(), name.c_str());
#endif
};

void heFboCreateDepthTextureAttachment(HeFbo* fbo) {
    fbo->depthAttachment.texture = true;
    fbo->depthAttachment.size    = fbo->size;
    fbo->depthAttachment.samples = 1;
    
    glGenTextures(1, &fbo->depthAttachment.id);
    glBindTexture(GL_TEXTURE_2D, fbo->depthAttachment.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, fbo->depthAttachment.size.x, fbo->depthAttachment.size.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, fbo->depthAttachment.id, 0);

    // memory tracker
    uint32_t perPixel = 3;
    uint32_t bytes = hm::ceilPowerOfTwo(fbo->size.x * fbo->size.y * perPixel);
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
    uint8_t samples = 0;
    if(fbo->depthAttachment.id > 0) {
        heMemoryTracker[HE_MEMORY_TYPE_FBO] -= fbo->depthAttachment.memory;
        if(fbo->depthAttachment.texture)
            glDeleteTextures(1, &fbo->depthAttachment.id);
        else {
            glDeleteRenderbuffers(1, &fbo->depthAttachment.id);
            samples = fbo->depthAttachment.samples;
        }

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
            heFboCreateColourTextureAttachment(fbo, all.format, all.size, all.mipCount);
        else
            heFboCreateColourBufferAttachment(fbo, all.format, all.samples, all.size);
    }
    
    if(depthAttachment == 1)
        heFboCreateDepthBufferAttachment(fbo, samples);
    if(depthAttachment == 2)
        heFboCreateDepthTextureAttachment(fbo);
};

void heFboRender(HeFbo* sourceFbo, HeFbo* targetFbo) {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, targetFbo->fboId);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, sourceFbo->fboId);
    glBlitFramebuffer(0, 0, sourceFbo->size.x, sourceFbo->size.y, 0, 0, targetFbo->size.x, targetFbo->size.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
};

void heFboRender(HeFbo* sourceFbo, hm::vec2i const& windowSize) {
    HeFbo fake;
    fake.fboId = 0;
    fake.size = windowSize;
    heFboRender(sourceFbo, &fake);
};

void heFboValidate(HeFbo const* fbo) {
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
#ifdef HE_ENABLE_NAMES
        HE_ERROR("Fbo [" + fbo->name + "] not set up correctly (" + he_gl_error_to_string(glGetError()) + ")");
#else
        HE_ERROR("Fbo not set up correctly (" + he_gl_error_to_string(glGetError()) + ")");
#endif
    }
};

HeTexture heFboCreateColourTextureWrapper(HeFboAttachment const* attachment) {
    HeTexture texture;
    texture.textureId  = attachment->id;
    texture.format     = attachment->format;
    texture.size       = attachment->size;
    texture.parameters = HE_TEXTURE_FILTER_BILINEAR | HE_TEXTURE_CLAMP_EDGE;
    texture.channels = (texture.format == HE_COLOUR_FORMAT_RGBA8 || texture.format == HE_COLOUR_FORMAT_RGBA16) ? 4 : 3;
    return texture;
};


// --- Textures

void heTextureCreateEmptyCubeMap(HeTexture* texture) {
    HE_CRASH_LOG();

    if(!(texture->parameters & HE_TEXTURE_FILTER_TRILINEAR))
        texture->mipmapCount = 1;
    
    texture->cubeMap = true;
    glGenTextures(1, &texture->textureId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture->textureId);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, texture->mipmapCount);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
    glTexStorage2D(GL_TEXTURE_CUBE_MAP, (texture->parameters & HE_TEXTURE_FILTER_TRILINEAR) ? texture->mipmapCount : 1, texture->format, texture->size.x, texture->size.y);
    
    heTextureApplyParameters(texture);

    // add to memory tracker
    texture->memory = heTextureCalculateMemorySize(texture, 1);
    heMemoryTracker[HE_MEMORY_TYPE_TEXTURE] += texture->memory;
    
    texture->referenceCount = 1;
#ifdef HE_ENABLE_NAMES
    if(!texture->name.empty())
        glObjectLabel(GL_TEXTURE, texture->textureId, -1, texture->name.c_str());
#endif
};

void heTextureLoadFromImageFile(HeTexture* texture, std::string const& fileName, b8 const compress) {
    HE_CRASH_LOG();
    stbi_set_flip_vertically_on_load(true);
    unsigned char* buffer = stbi_load(fileName.c_str(), &texture->size.x, &texture->size.y, &texture->channels, 0);

    if(!(texture->parameters & HE_TEXTURE_FILTER_TRILINEAR))
        texture->mipmapCount = 1;    
    
    HE_LOG("Loading texture [" + fileName + "] (" + std::to_string(texture->size.x) + "x" + std::to_string(texture->size.y) + "@" + std::to_string(texture->channels) + ")");
    
    if(!buffer) {
        HE_ERROR("Could not open texture [" + fileName + "]!");
        texture->textureId = 0;
        return;
    }
    
    if(texture->channels == 4)
        texture->format = (compress) ? HE_COLOUR_FORMAT_COMPRESSED_RGBA8 : HE_COLOUR_FORMAT_RGBA8;
    else if(texture->channels == 3)
        texture->format = (compress) ? HE_COLOUR_FORMAT_COMPRESSED_RGB8 : HE_COLOUR_FORMAT_RGB8;
    
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

void heTextureLoadFromHdrImageFile(HeTexture* texture, std::string const& fileName) {
    HE_CRASH_LOG();
    stbi_set_flip_vertically_on_load(true);
    float* buffer = stbi_loadf(fileName.c_str(), &texture->size.x, &texture->size.y, &texture->channels, 4);

    if(!(texture->parameters & HE_TEXTURE_FILTER_TRILINEAR))
        texture->mipmapCount = 1;
    
    HE_LOG("Loading hdr texture [" + fileName + "] (" + std::to_string(texture->size.x) + "x" + std::to_string(texture->size.y) + "@" + std::to_string(texture->channels) + ")");
    
    if(!buffer) {
        HE_ERROR("Could not open texture [" + fileName + "]!");
        texture->textureId = 0;
        return;
    }
    
    texture->channels = 4; // for now we only support rgba hdr textures, cant rememeber why tho
    texture->format   = HE_COLOUR_FORMAT_RGBA16;
    
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

void heTextureLoadFromCubemapFile(HeTexture* texture, std::string const& fileName, b8 const compress) {
    HE_CRASH_LOG();
    HE_LOG("Loading cubemap texture [" + fileName + "]");
    std::vector<int32_t> mipmaps;
    unsigned char* buffer = heTextureLoadFromBinaryFile(fileName, &texture->size.x, &texture->size.y, &texture->channels, &texture->format, &mipmaps, &texture->compressionFormat);

    if(!buffer) {
        HE_ERROR("Could not open texture [" + fileName + "]!");
        texture->textureId = 0;
        return;
    }

    glGenTextures(1, &texture->textureId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture->textureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    int32_t offset = 0;
    hm::vec2i mipSize = texture->size;
    for(uint8_t i = 0; i < mipmaps.size(); ++i) {
        int32_t size = mipmaps[i];
        for(uint8_t j = 0; j < 6; ++j) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, i, texture->format, mipSize.x, mipSize.y, 0, (texture->channels == 4) ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, &buffer[offset]);
        }
        
        offset += size;
        mipSize /= 2;
        mipSize.x = hm::floorPowerOfTwo(mipSize.x);
        mipSize.y = hm::floorPowerOfTwo(mipSize.y);
    }

    heTextureApplyParameters(texture);
    heGlErrorPrint();
    
    // add to memory tracker
    texture->memory = heTextureCalculateMemorySize(texture, 1);
    heMemoryTracker[HE_MEMORY_TYPE_TEXTURE] += texture->memory;
    texture->referenceCount = 1;
    texture->mipmapCount    = (uint32_t) mipmaps.size();
    texture->cubeMap        = true;
    
#ifdef HE_ENABLE_NAMES
    texture->name = fileName;
    glObjectLabel(GL_TEXTURE, texture->textureId, -1, texture->name.c_str());
#endif
};

void heTextureLoadFromHdrCubemapFile(HeTexture* texture, std::string const& fileName) {
    HE_CRASH_LOG();
    HE_LOG("Loading cubemap texture [" + fileName + "]");
    std::vector<int32_t> mipmaps;
    float* buffer = heTextureLoadFromHdrBinaryFile(fileName, &texture->size.x, &texture->size.y, &texture->channels, &texture->format, &mipmaps);

    texture->referenceCount = 1;
    texture->mipmapCount    = (uint32_t) mipmaps.size();
    texture->cubeMap        = true;

    if(!buffer) {
        HE_ERROR("Could not open texture [" + fileName + "]!");
        texture->textureId = 0;
        return;
    }

    glGenTextures(1, &texture->textureId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture->textureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, texture->mipmapCount - 1);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    int32_t offset = 0;
    hm::vec2i mipSize = texture->size;
    for(uint8_t i = 0; i < mipmaps.size(); ++i) {
        int32_t size = (mipmaps[i] / sizeof(float)) / 6; // per face
        for(uint8_t j = 0; j < 6; ++j) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, i, texture->format, mipSize.x, mipSize.y, 0, (texture->channels == 4) ? GL_RGBA : GL_RGB, GL_FLOAT, &buffer[offset]);
            offset += size;
        } 
        
        mipSize /= 2;
        mipSize.x = hm::floorPowerOfTwo(mipSize.x);
        mipSize.y = hm::floorPowerOfTwo(mipSize.y);
    }

    free(buffer);
    heTextureApplyParameters(texture);
    heGlErrorPrint("Hdr cubemap");
    
    // add to memory tracker
    texture->memory = heTextureCalculateMemorySize(texture, 1);
    heMemoryTracker[HE_MEMORY_TYPE_TEXTURE] += texture->memory;
    
#ifdef HE_ENABLE_NAMES
    texture->name = fileName;
    glObjectLabel(GL_TEXTURE, texture->textureId, -1, texture->name.c_str());
#endif
};

void heTextureLoadFromCompressedFile(HeTexture* texture, std::string const& fileName) {
    HE_CRASH_LOG();
    HE_LOG("Loading compressed texture [" + fileName + "]");
    std::vector<int32_t> mipmaps;
    unsigned char* compressedBuffer = heTextureLoadFromBinaryFile(fileName, &texture->size.x, &texture->size.y, &texture->channels, &texture->format, &mipmaps, &texture->compressionFormat);
    
    if(!compressedBuffer) {
        HE_ERROR("Could not open texture [" + fileName + "]!");
        texture->textureId = 0;
        return;
    }
    
    texture->bufferc = compressedBuffer;
    if (heIsMainThread()) {
#ifdef HE_ENABLE_NAMES
        texture->name = fileName;
#endif
        heTextureCreateFromCompressedBuffer(texture, mipmaps);
    } else {
        // no context in the current thread, add it to the thread loader
        heThreadLoaderRequestTexture(texture);
    }
};

void heTextureCreateFromBuffer(HeTexture* texture) {
    HE_CRASH_LOG();
    if(texture->size.x == 0 || texture->size.y == 0 || texture->channels == 0)
        HE_WARNING("Creating texture with invalid info (" + std::to_string(texture->size.x) + "x" + std::to_string(texture->size.y) + "@" + std::to_string(texture->channels) + ")"); 

    uint32_t unpackAlignment = 1;
    glGenTextures(1, &texture->textureId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->textureId);
    if(texture->format == HE_COLOUR_FORMAT_RGBA16) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
        glTexImage2D(GL_TEXTURE_2D, 0, texture->format, texture->size.x, texture->size.y, 0, (texture->channels == 4) ? GL_RGBA : GL_RGB, GL_FLOAT, texture->bufferf);
        free(texture->bufferf);
        texture->bufferf = nullptr;
        unpackAlignment = 8;
    } else {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, texture->format, texture->size.x, texture->size.y, 0, (texture->channels == 4) ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, texture->bufferc);
        free(texture->bufferc);
        texture->bufferc = nullptr;
        unpackAlignment = 1;
    }

    heTextureApplyParameters(texture);
    if(texture->parameters & HE_TEXTURE_FILTER_TRILINEAR)
        glGenerateMipmap((texture->cubeMap) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D); // pretty sure this will always be texture2d but whatever
    
    // memory tracker
    texture->referenceCount = 1;
    texture->memory = heTextureCalculateMemorySize(texture, unpackAlignment);
    heMemoryTracker[HE_MEMORY_TYPE_TEXTURE] += texture->memory;
#ifdef HE_ENABLE_NAMES
    glObjectLabel(GL_TEXTURE, texture->textureId, (uint32_t) texture->name.size(), texture->name.c_str());
#endif
};

void heTextureCreateFromCompressedBuffer(HeTexture* texture, std::vector<int32_t> const& mipmapSizes) {
    HE_CRASH_LOG();
    if(texture->size.x == 0 || texture->size.y == 0 || texture->channels == 0)
        HE_WARNING("Creating texture with invalid info (" + std::to_string(texture->size.x) + "x" + std::to_string(texture->size.y) + "@" + std::to_string(texture->channels) + ")"); 

    int32_t mipmapCount = (int32_t) mipmapSizes.size();
    texture->mipmapCount = mipmapCount;
            
    glGenTextures(1, &texture->textureId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->textureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    int32_t   offset     = 0;
    uint32_t  totalBytes = 0;
    hm::vec2i mipSize    = texture->size;
    for(int32_t all = 0; all < mipmapCount; ++all) {
        int32_t size = mipmapSizes[all];        
        glCompressedTexImage2D(GL_TEXTURE_2D, all, texture->compressionFormat, mipSize.x, mipSize.y, 0, size, &texture->bufferc[offset]);
        offset += size;
        mipSize /= 2;
        mipSize.x = hm::floorPowerOfTwo(mipSize.x);
        mipSize.y = hm::floorPowerOfTwo(mipSize.y);
        int32_t levelSize;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, all, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &levelSize);
        totalBytes += levelSize;
    }
    free(texture->bufferc);
    texture->bufferc = nullptr;

    heTextureApplyParameters(texture);        

    // memory tracker
    texture->memory = totalBytes;
    heMemoryTracker[HE_MEMORY_TYPE_TEXTURE] += texture->memory;
    texture->referenceCount = 1;
#ifdef HE_ENABLE_NAMES
    glObjectLabel(GL_TEXTURE, texture->textureId, (uint32_t) texture->name.size(), texture->name.c_str());
#endif
};

void heTextureBind(HeTexture const* texture, int8_t const slot) {
    HE_CRASH_LOG();
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
    HE_CRASH_LOG();
    uint32_t format = (cubeMap) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(format, texture);
};

void heImageTextureBind(HeTexture const* texture, int8_t const slot, int8_t const level, int8_t const layer, HeAccessType const access) {
    HE_CRASH_LOG();
    if(texture != nullptr)
        glBindImageTexture(slot, texture->textureId, level, (layer == -1) ? GL_TRUE : GL_FALSE, (layer == -1) ? 0 : layer, access, texture->format);
    else
        glBindImageTexture(slot, 0, level, (layer == -1) ? GL_TRUE : GL_FALSE, (layer == -1) ? 0 : layer, access, GL_RGB8);        
};

void heTextureUnbind(int8_t const slot, b8 const cubeMap) {
    HE_CRASH_LOG();
    uint32_t format = (cubeMap) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(format, 0);
};

void heTextureDestroy(HeTexture* texture) {
    HE_CRASH_LOG();
    if(--texture->referenceCount == 0) {
        heMemoryTracker[HE_MEMORY_TYPE_TEXTURE] -= texture->memory;
        glDeleteTextures(1, &texture->textureId);
        texture->textureId = 0;
        texture->size = hm::vec2i(0);
        texture->channels  = 0;
        texture->memory    = 0;
    }
};

int32_t heTextureGetCompressionFormat(HeTexture* texture) {
    int32_t format;
    glGetTextureLevelParameteriv(texture->textureId, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
    return format;
};

hm::vec2i heTextureCalculateMipmapSize(HeTexture* texture, uint32_t const level) {
    if(level == 0)
        return texture->size;

    hm::vec2i size = texture->size;
    for(uint32_t i = 0; i < level; ++i) {
        size /= 2;
        size.x = hm::floorPowerOfTwo(size.x);
        size.y = hm::floorPowerOfTwo(size.y);
    }
    
    return size;
};

uint32_t heTextureCalculateMemorySize(HeTexture* texture, uint8_t const unpackAlignment) {
    uint8_t bytesPerPixel = heColourFormatGetBytesPerPixel(texture->format);
    uint32_t row = texture->size.x * bytesPerPixel;
    row += row % 8;
    uint32_t total = row * texture->size.y;
    total = hm::ceilPowerOfTwo(total);
    if(texture->parameters & HE_TEXTURE_FILTER_TRILINEAR)
        total = (uint32_t) (total * 1.33);
    if(texture->cubeMap)
        total *= 6; // for all 6 faces
    
    return total;
};


void* heTextureGetData(HeTexture* texture, int32_t* size, uint8_t const level) {
    void* buffer = nullptr;
    if(texture->cubeMap) {
        buffer = heTextureGetCubemapHdrData(texture, size, level);
    } else {
        if(texture->compressionFormat)
            buffer = heTextureGetCompressedData(texture, size, level);
        else if(texture->format == HE_COLOUR_FORMAT_RGB16 || texture->format == HE_COLOUR_FORMAT_RGBA16) 
            buffer = heTextureGetHdrData(texture, size, level);
        else {
            uint32_t format = GL_RGB;
            if(texture->format == HE_COLOUR_FORMAT_RGBA8)
                format = GL_RGBA;
    
            hm::vec2i mipSize = heTextureCalculateMipmapSize(texture, level);
            *size = mipSize.x * mipSize.y * heColourFormatGetBytesPerPixel(texture->format);    
            buffer = (unsigned char*) malloc(*size);
            glGetTextureImage(texture->textureId, level, format, GL_UNSIGNED_BYTE, *size, buffer);
        }
    }

    return buffer;
};

void* heTextureGetCompressedData(HeTexture* texture, int32_t* size, uint8_t const level) {
    glGetTextureLevelParameteriv(texture->textureId, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, size);
    unsigned char* buffer = (unsigned char*) malloc(*size);
    glGetCompressedTextureImage(texture->textureId, level, *size, buffer);
    return buffer;
};

void* heTextureGetHdrData(HeTexture* texture, int32_t* size, uint8_t const level) {
    uint32_t format = GL_RGB;
    if(texture->format == HE_COLOUR_FORMAT_RGBA16)
        format = GL_RGBA;

    hm::vec2i mipSize = heTextureCalculateMipmapSize(texture, level);
    *size = mipSize.x * mipSize.y * heColourFormatGetBytesPerPixel(texture->format);    
    float* buffer = (float*) malloc(*size);
    glGetTextureImage(texture->textureId, level, format, GL_FLOAT, *size, buffer);
    return buffer;
};

void* heTextureGetCubemapHdrData(HeTexture* texture, int32_t* size, uint8_t const level) {
    uint32_t format = GL_RGB;
    if(texture->format == HE_COLOUR_FORMAT_RGBA16)
        format = GL_RGBA;
    
    hm::vec2i mipSize      = heTextureCalculateMipmapSize(texture, level); // in pixels
    uint32_t perpixel      = heColourFormatGetBytesPerPixel(texture->format); // in bytes
    uint32_t perface       = mipSize.x * mipSize.y * perpixel; // in bytes
    uint32_t floatsperface = perface / 4; // in floats
    *size                  = perface * 6; // in bytes
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    float* buffer = (float*) malloc(*size);
    glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X, level, format, GL_FLOAT, &buffer[floatsperface * 0]);
    glGetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, level, format, GL_FLOAT, &buffer[floatsperface * 1]);    
    glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, level, format, GL_FLOAT, &buffer[floatsperface * 2]);
    glGetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, level, format, GL_FLOAT, &buffer[floatsperface * 3]);
    glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, level, format, GL_FLOAT, &buffer[floatsperface * 4]);
    glGetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, level, format, GL_FLOAT, &buffer[floatsperface * 5]);
    return buffer;
};


void heTextureApplyParameters(HeTexture* texture) {
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
    if(texture->parameters & HE_TEXTURE_FILTER_ANISOTROPIC)
        heTextureFilterAnisotropic(texture);
    if(texture->parameters & HE_TEXTURE_FILTER_TRILINEAR)
        heTextureFilterTrilinear(texture);
};

void heTextureFilterLinear(HeTexture* texture) {
    uint32_t format = (texture->cubeMap) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
    
    glTexParameteri(format, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(format, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
};

void heTextureFilterBilinear(HeTexture* texture) {
    uint32_t format = (texture->cubeMap) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

    glTexParameteri(format, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(format, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
};

void heTextureFilterTrilinear(HeTexture* texture) {
    texture->mipmapCount = min(texture->mipmapCount, 1 + (uint32_t) (std::floor(std::log2(max(texture->size.x, texture->size.y)))));
    uint32_t format = (texture->cubeMap) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
    glTexParameteri(format, GL_TEXTURE_MAX_LEVEL, texture->mipmapCount - 1);
    glTexParameteri(format, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(format, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(format, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
};

void heTextureGenerateMipmaps(b8 const cubeMap) {
    glGenerateMipmap((cubeMap) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D);
};

void heTextureFilterAnisotropic(HeTexture* texture) {
    uint32_t format = (texture->cubeMap) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
    if(maxAnisotropicValue == -1.f)
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropicValue);
    glTexParameterf(format, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropicValue);
};

void heTextureClampEdge(HeTexture* texture) {
    uint32_t format = (texture->cubeMap) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

    glTexParameteri(format, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(format, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if(texture->cubeMap)
        glTexParameteri(format, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
};

void heTextureClampBorder(HeTexture* texture) {
    uint32_t format = (texture->cubeMap) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

    glTexParameteri(format, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(format, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    if(texture->cubeMap)
        glTexParameteri(format, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
};

void heTextureClampRepeat(HeTexture* texture) {
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
    return (z - x) * 1000;
};

uint32_t heColourFormatGetBytesPerPixel(HeColourFormat const format) {
    uint8_t bytesPerPixel = 0;
    switch(format) {
    case HE_COLOUR_FORMAT_RGB8:
        bytesPerPixel = 3;
        break;
    
    case HE_COLOUR_FORMAT_RGBA8:
        bytesPerPixel = 4;
        break;
    
    case HE_COLOUR_FORMAT_RGB16:
        bytesPerPixel = 3 * 4;
        break;
    
    case HE_COLOUR_FORMAT_RGBA16:
        bytesPerPixel = 4 * 4;
        break;
    }
    return bytesPerPixel;
};

void heGlPrintInfo() {
    std::string vendor, renderer, version;
    vendor   = std::string((const char*) glGetString(GL_VENDOR));
    renderer = std::string((const char*) glGetString(GL_RENDERER));
    version  = std::string((const char*) glGetString(GL_VERSION));
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

void heGlErrorPrint(std::string const& location) {
    uint32_t error = heGlErrorCheck();
    if(error != GL_NO_ERROR)
        HE_WARNING("Gl Error: " + he_gl_error_to_string(error) + " in " + location);
};

#else

void heGlErrorClear()         {};
void heGlErrorSaveAllErrors() {};
uint32_t heGlErrorGet()       { return 0; };
uint32_t heGlErrorCheck()     { return 0; };

#endif
