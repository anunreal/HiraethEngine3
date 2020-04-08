#include "heGlLayer.h"
#include "heAssets.h"
#include "GLEW/glew.h"
#include "GLEW/wglew.h"
#include "heWindow.h"
#include "heCore.h"
#include "heWin32Layer.h"
#include <fstream>
#include <algorithm>

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

uint32_t heShaderLoadFromFile(std::string const& file, uint32_t type) {
    
    std::string source = heShaderLoadSource(file);
    if(source.empty())
        return 0;
    
    const char* src_ptr = source.c_str();
    uint32_t id = glCreateShader(type);
    glShaderSource(id, 1, &src_ptr, NULL);
    glCompileShader(id);
    
    int32_t compileStatus;
    glGetShaderiv(id, GL_COMPILE_STATUS, &compileStatus);
    if(compileStatus == 0) {
        char infoLog[512];
        memset(infoLog, 0, 512);
        glGetShaderInfoLog(id, 512, NULL, infoLog);
        HE_ERROR("While compiling shader [" + file + "]:\n" + std::string(infoLog));
        return 0;
    } else
        return id;
    
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
    
};

void heShaderCreateProgram(HeShaderProgram* program, std::string const& vertexShader, std::string const& fragmentShader) {
    
    program->files.emplace_back(vertexShader);
    program->files.emplace_back(fragmentShader);
    heShaderLoadProgram(program, vertexShader, fragmentShader);
    
};

void heShaderCreateProgram(HeShaderProgram* program, std::string const& vertexShader, std::string const& geometryShader, std::string const& fragmentShader) {
    
    program->files.emplace_back(vertexShader);
    program->files.emplace_back(geometryShader);
    program->files.emplace_back(fragmentShader);
    heShaderLoadProgram(program, vertexShader, geometryShader, fragmentShader);
    
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
    
};

void heShaderReload(HeShaderProgram* program) {
    
    // store current uniform values
    struct Uniform {
        union {
            float   fdata[16];
            int32_t idata[16];
        };
        b8 isInt = false;
        HeUniformDataType type = HE_UNIFORM_DATA_TYPE_NONE;
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
        u.type = (HeUniformDataType) values[1];
        
        if(u.type == HE_UNIFORM_DATA_TYPE_INT || u.type == HE_UNIFORM_DATA_TYPE_BOOL) {
            glGetUniformiv(program->programId, values[2], &u.idata[0]);
            u.isInt = true;
        } else
            glGetUniformfv(program->programId, values[2], &u.fdata[0]);
        
        uniforms[name] = u;
    }
    
    
    // reload shader
    heShaderDestroy(program);
    if(program->files.size() == 2)
        // vertex -> fragment
        heShaderLoadProgram(program, program->files[0], program->files[1]);
    else
        // vertex -> geometry -> fragment
    
        heShaderLoadProgram(program, program->files[0], program->files[1], program->files[2]);
    
    heShaderBind(program);
    
    // upload data to it
    for(auto const& all : uniforms)
        heShaderLoadUniform(program, all.first, all.second.isInt ? (const void*) &all.second.idata[0] : (const void*) &all.second.fdata[0], all.second.type);
    
    std::string names;
    for(std::string const& all : program->files)
        names += all + ", ";
    HE_DEBUG("Reloaded program (" + names.substr(0, names.size() - 3) + ")");
    
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
    }
    return location;
    
};

int heShaderGetSamplerLocation(HeShaderProgram* program, std::string const& sampler, uint8_t const requestedSlot) {
    
    int32_t location;
    if (program->samplers.find(sampler) != program->samplers.end()) {
        location = program->samplers[sampler];
    } else {
        uint32_t uloc = glGetUniformLocation(program->programId, sampler.c_str());
        glProgramUniform1i(program->programId, uloc, requestedSlot);
        program->samplers[sampler] = requestedSlot;
        location = requestedSlot;
    }
    
    return location;
    
};

void heShaderLoadUniform(HeShaderProgram* program, std::string const& uniformName, void const* data, HeUniformDataType const type) {
    
    int32_t location = heShaderGetUniformLocation(program, uniformName);
    
    switch(type) {
        case HE_UNIFORM_DATA_TYPE_INT:
        case HE_UNIFORM_DATA_TYPE_BOOL:
        heShaderLoadUniform(program, uniformName, ((int32_t*) data)[0]);
        break;
        
        case HE_UNIFORM_DATA_TYPE_FLOAT:
        heShaderLoadUniform(program, uniformName, ((float*) data)[0]);
        break;
        
        case HE_UNIFORM_DATA_TYPE_VEC2:
        heShaderLoadUniform(program, uniformName, hm::vec2f(((float*) data)[0], ((float*) data)[1]));
        break;
        
        case HE_UNIFORM_DATA_TYPE_VEC3:
        heShaderLoadUniform(program, uniformName, hm::vec3f(((float*) data)[0], ((float*) data)[1], ((float*) data)[2]));
        break;
        
        case HE_UNIFORM_DATA_TYPE_VEC4:
        heShaderLoadUniform(program, uniformName, hm::vec4f(((float*) data)[0], ((float*) data)[1], ((float*) data)[2], ((float*) data)[3]));
        break;
        
        case HE_UNIFORM_DATA_TYPE_MAT4:
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
        case HE_UNIFORM_DATA_TYPE_INT:
        heShaderLoadUniform(program, uniformName, data->_int);
        break;
        
        case HE_UNIFORM_DATA_TYPE_FLOAT:
        heShaderLoadUniform(program, uniformName, data->_float);
        break;
        
        case HE_UNIFORM_DATA_TYPE_VEC2:
        heShaderLoadUniform(program, uniformName, data->_vec2);
        break;
        
        case HE_UNIFORM_DATA_TYPE_COLOUR:
        heShaderLoadUniform(program, uniformName, data->_colour);
        break;
    };
    
};


// --- Buffers

void heVboCreate(HeVbo* vbo, std::vector<float> const& data, uint8_t const dimensions, HeVboUsage const usage) {
    
    glGenBuffers(1, &vbo->vboId);
    glBindBuffer(GL_ARRAY_BUFFER, vbo->vboId);
    glBufferData(GL_ARRAY_BUFFER, (uint32_t)data.size() * sizeof(float), data.data(), usage);
    vbo->usage         = usage;
    vbo->dimensions    = dimensions;
    vbo->verticesCount = (uint32_t)data.size() / vbo->dimensions;
    vbo->type          = HE_UNIFORM_DATA_TYPE_FLOAT;
    
};

void heVboCreateInt(HeVbo* vbo, std::vector<int32_t> const& data, uint8_t const dimensions, HeUniformDataType const type, HeVboUsage const usage) {
    
    glGenBuffers(1, &vbo->vboId);
    glBindBuffer(GL_ARRAY_BUFFER, vbo->vboId);
    glBufferData(GL_ARRAY_BUFFER, (uint32_t)data.size() * sizeof(int32_t), data.data(), usage);
    vbo->usage         = usage;
    vbo->dimensions    = dimensions;
    vbo->verticesCount = (uint32_t)data.size() / vbo->dimensions;
    vbo->type          = type;
    
};

void heVboCreateInt(HeVbo* vbo, std::vector<int32_t> const& data, uint8_t const dimensions, HeVboUsage const usage) {
    
    glGenBuffers(1, &vbo->vboId);
    glBindBuffer(GL_ARRAY_BUFFER, vbo->vboId);
    glBufferData(GL_ARRAY_BUFFER, (uint32_t)data.size() * sizeof(int32_t), data.data(), usage);
    vbo->usage         = usage;
    vbo->dimensions    = dimensions;
    vbo->verticesCount = (uint32_t)data.size() / vbo->dimensions;
    vbo->type          = HE_UNIFORM_DATA_TYPE_INT;
    
};

void heVboCreateUint(HeVbo* vbo, std::vector<uint32_t> const& data, uint8_t const dimensions, HeVboUsage const usage) {
    
    glGenBuffers(1, &vbo->vboId);
    glBindBuffer(GL_ARRAY_BUFFER, vbo->vboId);
    glBufferData(GL_ARRAY_BUFFER, (uint32_t)data.size() * sizeof(int32_t), data.data(), usage);
    vbo->usage         = usage;
    vbo->dimensions    = dimensions;
    vbo->verticesCount = (uint32_t)data.size() / vbo->dimensions;
    vbo->type          = HE_UNIFORM_DATA_TYPE_UINT;
    
};

void heVboAllocate(HeVbo* vbo, uint32_t const size, const uint8_t dimensions, HeVboUsage const usage, HeUniformDataType const type) {
    
    glGenBuffers(1, &vbo->vboId);
    glBindBuffer(GL_ARRAY_BUFFER, vbo->vboId);
    
    if(size > 0)
        glBufferData(GL_ARRAY_BUFFER, size * (type == HE_UNIFORM_DATA_TYPE_FLOAT ? sizeof(float) : sizeof(int32_t)), nullptr, usage);
    
    vbo->usage         = usage;
    vbo->dimensions    = dimensions;
    vbo->verticesCount = 0;
    vbo->type          = type;
    
};

void heVboDestroy(HeVbo* vbo) {
    
    glDeleteBuffers(1, &vbo->vboId);
    vbo->vboId = 0;
    
};

void heVaoCreate(HeVao* vao, HeVaoType const type) {
    
    glGenVertexArrays(1, &vao->vaoId);
    vao->type = type;
    
};

void heVaoAddVboData(HeVbo* vbo, int8_t const attributeIndex) {
    
    if(vbo->type == HE_UNIFORM_DATA_TYPE_FLOAT) {
        heVboCreate(vbo, vbo->dataf, vbo->dimensions, vbo->usage);
        glVertexAttribPointer(attributeIndex, vbo->dimensions, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
        vbo->dataf.clear();
    } else if(vbo->type == HE_UNIFORM_DATA_TYPE_INT) {
        heVboCreateInt(vbo, vbo->datai, vbo->dimensions, vbo->usage);
        glVertexAttribIPointer(attributeIndex, vbo->dimensions, vbo->type, 0, (GLvoid*)0);
        vbo->datai.clear();
    } else if(vbo->type == HE_UNIFORM_DATA_TYPE_UINT) {
        heVboCreateUint(vbo, vbo->dataui, vbo->dimensions, vbo->usage);
        glVertexAttribIPointer(attributeIndex, vbo->dimensions, vbo->type, 0, (GLvoid*)0);
        vbo->dataui.clear();
    }
    
};

void heVaoAddVbo(HeVao* vao, HeVbo* vbo) {
    
    if(vbo->type == HE_UNIFORM_DATA_TYPE_FLOAT)
        glVertexAttribPointer((uint32_t) vao->vbos.size(), vbo->dimensions, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    else
        glVertexAttribIPointer((uint32_t) vao->vbos.size(), vbo->dimensions, vbo->type, 0, (GLvoid*)0);
    
    vao->vbos.emplace_back(*vbo);
    if (vao->vbos.size() == 1)
        vao->verticesCount = vbo->verticesCount;
    
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
        vbo->type       = HE_UNIFORM_DATA_TYPE_FLOAT;
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
        vbo->type       = HE_UNIFORM_DATA_TYPE_INT;
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
        vbo->type       = HE_UNIFORM_DATA_TYPE_UINT;
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
    
};

void heVaoRender(HeVao const* vao) {
    
    glDrawArrays(vao->type, 0, vao->verticesCount);
    
};



void heFboCreateColourAttachment(HeFbo* fbo) {
    
    uint32_t id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, (fbo->flags & HE_FBO_FLAG_HDR) ? GL_RGBA16F : GL_RGBA8, fbo->size.x, fbo->size.y,
                 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (int)fbo->colourTextures.size(), GL_TEXTURE_2D, id, 0);
    fbo->colourTextures.emplace_back(id);
    
};

void heFboCreateMultisampledColourAttachment(HeFbo* fbo) {
    
    uint32_t id;
    glGenRenderbuffers(1, &id);
    glBindRenderbuffer(GL_RENDERBUFFER, id);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, fbo->samples, (fbo->flags & HE_FBO_FLAG_HDR) ? GL_RGBA16F : GL_RGBA8,
                                     fbo->size.x, fbo->size.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (int)fbo->colourBuffers.size(), GL_RENDERBUFFER, id);
    fbo->colourBuffers.emplace_back(id);
    
};

void heFboCreateDepthBufferAttachment(HeFbo* fbo) {
    
    glGenRenderbuffers(1, &fbo->depthAttachments[1]);
    glBindRenderbuffer(GL_RENDERBUFFER, fbo->depthAttachments[1]);
    if (fbo->samples == 1)
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, fbo->size.x, fbo->size.y);
    else
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, fbo->samples, GL_DEPTH_COMPONENT24, fbo->size.x, fbo->size.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fbo->depthAttachments[1]);
    
};

void heFboCreateDepthTextureAttachment(HeFbo* fbo) {
    
    glGenTextures(1, &fbo->depthAttachments[0]);
    glBindTexture(GL_TEXTURE_2D, fbo->depthAttachments[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, fbo->size.x, fbo->size.y, 0, GL_DEPTH_COMPONENT,
                 GL_UNSIGNED_INT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, fbo->depthAttachments[0], 0);
    
};

void heFboCreate(HeFbo* fbo) {
    
    glGenFramebuffers(1, &fbo->fboId);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo->fboId);
    
    if (fbo->samples == 1)
        heFboCreateColourAttachment(fbo);
    else
        heFboCreateMultisampledColourAttachment(fbo);
    
    if (fbo->flags & HE_FBO_FLAG_DEPTH_RENDER_BUFFER)
        heFboCreateDepthBufferAttachment(fbo);
    if (fbo->flags & HE_FBO_FLAG_DEPTH_TEXTURE)
        heFboCreateDepthTextureAttachment(fbo);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        HE_ERROR("Fbo not set up correctly (" + std::to_string(glGetError()) + ")");
    
    heFboUnbind();
    
};

void heFboBind(HeFbo* fbo) {
    
    glBindFramebuffer(GL_FRAMEBUFFER, fbo->fboId);
    glViewport(0, 0, fbo->size.x, fbo->size.y);
    
    if (fbo->samples > 1)
        glEnable(GL_MULTISAMPLE);
    
    // enable draw buffers
    int count = std::max<int>((int)fbo->colourBuffers.size(), (int)fbo->colourTextures.size());
    std::vector<uint32_t> drawBuffers;
    for (int i = 0; i < count; ++i)
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
    
    glDeleteFramebuffers(1, &fbo->fboId);
    glDeleteTextures(1, &fbo->depthAttachments[0]); // colour texture
    glDeleteRenderbuffers(1, &fbo->depthAttachments[1]); // multisampled colour buffer
    glDeleteTextures((int)fbo->colourTextures.size(), fbo->colourTextures.data());
    glDeleteRenderbuffers((int)fbo->colourBuffers.size(), fbo->colourBuffers.data());
    fbo->fboId = 0;
    
};

void heFboResize(HeFbo* fbo, hm::vec2i const& newSize) {
    
    heFboDestroy(fbo);
    fbo->size = newSize;
    
    // the amount of extra textures / buffers attached to this fbo (minus the default one)
    int32_t colourTextures = (int32_t)fbo->colourTextures.size() - 1;
    int32_t colourBuffers = (int32_t)fbo->colourBuffers.size() - 1;
    
    fbo->colourTextures.clear();
    fbo->colourBuffers.clear();
    
    heFboCreate(fbo);
    for (int16_t i = 0; i < colourTextures; ++i)
        heFboCreateColourAttachment(fbo);
    
    for (int16_t i = 0; i < colourBuffers; ++i)
        heFboCreateMultisampledColourAttachment(fbo);
    
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


// --- Textures

void heTextureCreateEmpty(HeTexture* texture) {
    
    heTextureCreateFromBuffer(nullptr, &texture->textureId, texture->width, texture->height, texture->channels, texture->format);
    
};

void heTextureLoadFromFile(HeTexture* texture, std::string const& fileName) {
    
    stbi_set_flip_vertically_on_load(true);
    unsigned char* buffer = stbi_load(fileName.c_str(), &texture->width, &texture->height, &texture->channels, 4);
    if (buffer == nullptr) {
        HE_ERROR("Could not open texture [" + fileName + "]!");
        texture->textureId = 0;
        return;
    }
    
    texture->format = HE_COLOUR_FORMAT_RGBA8;
    if (heIsMainThread())
        // we are in some context thread, load it right now
        heTextureCreateFromBuffer(buffer, &texture->textureId, texture->width, texture->height, texture->channels, HE_COLOUR_FORMAT_RGBA8);
    else
        // no context in the current thread, add it to the thread loader
        heThreadLoaderRequestTexture(texture, buffer);
    
};

void heTextureCreateFromBuffer(unsigned char* buffer, uint32_t* id, int16_t const width, int16_t const height, int8_t const channels, HeColourFormat const format)
{
    
    glGenTextures(1, id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, *id);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    heTextureFilterTrilinear();
    heTextureFilterAnisotropic();
    heTextureClampRepeat();
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(buffer);
    
};

void heTextureLoadFromFile(HeTexture* texture, FILE* stream) {
    
    stbi_set_flip_vertically_on_load(true);
    unsigned char* buffer = stbi_load_from_file(stream, &texture->width, &texture->height, &texture->channels, 4);
    if (buffer == nullptr) {
        HE_ERROR("Could not open texture from FILE!");
        texture->textureId = 0;
        return;
    }
    
    texture->format = HE_COLOUR_FORMAT_RGBA8;
    heTextureCreateFromBuffer(buffer, &texture->textureId, texture->width, texture->height, texture->channels, HE_COLOUR_FORMAT_RGBA8);
    
    stbi_image_free(buffer);
    
};

void heTextureBind(HeTexture const* texture, int8_t const slot) {
    
    if(texture != nullptr) {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, texture->textureId);
    } else {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
};

void heTextureBind(uint32_t const texture, int8_t const slot) {
    
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, texture);
    
};

void heImageTextureBind(HeTexture const* texture, int8_t const slot, int8_t const layer, HeAccessType const access) {
    
    if(texture != nullptr)
        glBindImageTexture(slot, texture->textureId, 0, (layer == -1) ? GL_TRUE : GL_FALSE, layer, access, texture->format);
    
};

void heTextureUnbind(int8_t const slot) {
    
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, 0);
    
};

void heTextureDestroy(HeTexture* texture) {
    
    if (--texture->referenceCount == 0) {
        glDeleteTextures(1, &texture->textureId);
        texture->textureId = 0;
        texture->width = 0;
        texture->height = 0;
        texture->channels = 0;
    }
    
};

void heTextureFilterNearest() {
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    
};

void heTextureFilterBilinear() {
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
};

void heTextureFilterTrilinear() {
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    
};

void heTextureFilterAnisotropic() {
    
    float value;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &value);
    const float amount = std::fmin(4.0f, value);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, amount);
    
};

void heTextureClampEdge() {
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
};

void heTextureClampBorder() {
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    
};

void heTextureClampRepeat() {
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
};


// --- Utils

void heFrameClear(hm::colour const& colour, int8_t const type) {
    
    glClearColor(getR(&colour), getG(&colour), getB(&colour), getA(&colour));
    switch (type) {
        case 0:
        glClear(GL_COLOR_BUFFER_BIT);
        break;
        
        case 1:
        glClear(GL_DEPTH_BUFFER_BIT);
        break;
        
        case 2:
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        break;
    }
    
};

void heBlendMode(int8_t const mode) {
    
    if (mode == -1) {
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
    
    if (mode == -1)
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

void heEnableDepth(b8 const depth) {
    
    if (depth)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
    
};

void heViewport(hm::vec2i const& lowerleft, hm::vec2i const& size) {
    
    glViewport(lowerleft.x, lowerleft.y, size.x, size.y);
    
};


#ifdef HE_ENABLE_ERROR_CHECKING

std::vector<uint32_t> errors;

void heGlErrorClear() {
    
    while(glGetError() != GL_NO_ERROR) {}
    
};

void heGlErrorSaveAll() {
    
    errors.clear();
    uint32_t error;
    while((error = glGetError()) != GL_NO_ERROR)
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

void heGlErrorClear()         {};
void heGlErrorSaveAllErrors() {};
uint32_t heGlErrorGet()       { return 0; };
uint32_t heGlErrorCheck()     { return 0; };

#endif
