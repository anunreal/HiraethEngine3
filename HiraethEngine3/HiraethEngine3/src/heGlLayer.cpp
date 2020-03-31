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
#include "..\stb_image.h"


// --- Shaders

std::string heShaderLoadSource(const std::string& file) {
    
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

void heShaderLoadCompute(HeShaderProgram* program, const std::string& computeShader) {
    
    std::string cs_src = heShaderLoadSource(computeShader);
    
    if(cs_src.empty())
        return;
    
    uint32_t cid = glCreateShader(GL_COMPUTE_SHADER);
    int err = glGetError();
    if (err != GL_NO_ERROR)
        HE_ERROR("Shader Creation: " + std::to_string(err));
    
    const char* cs_ptr = cs_src.c_str();
    
    glShaderSource(cid, 1, &cs_ptr, NULL);
    glCompileShader(cid);
    
    int compileStatus;
    glGetShaderiv(cid, GL_COMPILE_STATUS, &compileStatus);
    if (!compileStatus) {
        char infoLog[512];
        memset(infoLog, 0, 512);
        glGetShaderInfoLog(cid, 512, NULL, infoLog);
        HE_ERROR("While compiling shader [" + computeShader + "]:\n" + std::string(infoLog));
    }
    
    program->programId = glCreateProgram();
    glAttachShader(program->programId, cid);
    glLinkProgram(program->programId);
    
    int success = 0;
    glGetProgramiv(program->programId, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        memset(infoLog, 0, 512);
        glGetProgramInfoLog(program->programId, 512, NULL, infoLog);
        HE_ERROR("Unable to link shader:\n" + std::string(infoLog));
        program->programId = 0;
    } else
        glUseProgram(program->programId);
    
    glDeleteShader(cid);
    
};

void heShaderLoadProgram(HeShaderProgram* program, const std::string& vertexShader, const std::string& fragmentShader) {
    
    std::string vs = heShaderLoadSource(vertexShader);
    std::string fs = heShaderLoadSource(fragmentShader);
    
    if (vs.empty() || fs.empty())
        return;
    
    uint32_t vid = glCreateShader(GL_VERTEX_SHADER);
    uint32_t fid = glCreateShader(GL_FRAGMENT_SHADER);
    
    int err = glGetError();
    if (err != GL_NO_ERROR)
        HE_ERROR("Shader Creation: " + std::to_string(err));
    
    const char* vs_ptr = vs.c_str();
    const char* fs_ptr = fs.c_str();
    
    int compileStatus;
    
    glShaderSource(vid, 1, &vs_ptr, NULL);
    glCompileShader(vid);
    
    glGetShaderiv(vid, GL_COMPILE_STATUS, &compileStatus);
    if (!compileStatus) {
        char infoLog[512];
        memset(infoLog, 0, 512);
        glGetShaderInfoLog(vid, 512, NULL, infoLog);
        HE_ERROR("While compiling shader [" + vertexShader + "]:\n" + std::string(infoLog));
    }
    
    glShaderSource(fid, 1, &fs_ptr, NULL);
    glCompileShader(fid);
    
    glGetShaderiv(fid, GL_COMPILE_STATUS, &compileStatus);
    if (!compileStatus) {
        char infoLog[512];
        memset(infoLog, 0, 512);
        glGetShaderInfoLog(fid, 512, NULL, infoLog);
        HE_ERROR("While compiling shader [" + fragmentShader + "]:\n" + std::string(infoLog));
    }
    
    program->programId = glCreateProgram();
    glAttachShader(program->programId, vid);
    glAttachShader(program->programId, fid);
    glLinkProgram(program->programId);
    
    int success = 0;
    glGetProgramiv(program->programId, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        memset(infoLog, 0, 512);
        glGetProgramInfoLog(program->programId, 512, NULL, infoLog);
        HE_ERROR("Unable to link shader:\n" + std::string(infoLog));
    }
    
    glUseProgram(0);
    glDeleteShader(vid);
    glDeleteShader(fid);
    
};

void heShaderCreateProgram(HeShaderProgram* program, const std::string& vertexShader, const std::string& fragmentShader) {
    
    program->files.emplace_back(vertexShader);
    program->files.emplace_back(fragmentShader);
    heShaderLoadProgram(program, vertexShader, fragmentShader);
    
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

void heShaderRunCompute(HeShaderProgram* program, const uint32_t groupsX, const uint32_t groupsY, const uint32_t groupsZ) {
    
    heShaderBind(program);
    glDispatchCompute(groupsX, groupsY, groupsZ);
    
};

void heShaderReload(HeShaderProgram* program) {
    
    // store current uniform values
    struct Uniform {
        union {
            float fdata[16];
            int   idata[16];
        };
        b8 isInt = false;
        HeUniformDataType type = HE_UNIFORM_DATA_TYPE_NONE;
    };
    
    int uniformCount;
    glGetProgramInterfaceiv(program->programId, GL_UNIFORM, GL_ACTIVE_RESOURCES, &uniformCount);
    std::map<std::string, Uniform> uniforms; 
    
    std::vector<GLchar> nameData(256);
    std::vector<GLenum> properties;
    properties.push_back(GL_NAME_LENGTH);
    properties.push_back(GL_TYPE);
    properties.push_back(GL_LOCATION);
    std::vector<GLint> values(properties.size());
    
    for(int i = 0; i < uniformCount; ++i) {
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
    // for now we assume that the program is a simple vertex / fragment shader
    heShaderLoadProgram(program, program->files[0], program->files[1]); 
    heShaderBind(program);
    
    // upload data to it
    for(const auto& all : uniforms)
        heShaderLoadUniform(program, all.first, all.second.isInt ? (const void*) &all.second.idata[0] : (const void*) &all.second.fdata[0], all.second.type);
    
    HE_DEBUG("Reloaded program (" + program->files[0] + ", " + program->files[1] + ")");
    
};

void heShaderCheckReload(HeShaderProgram* program) {
    
#ifdef HE_ENABLE_HOTSWAP_SHADER
    b8 needsReloading = false;
    
    for(const std::string& files : program->files) {
        if(heFileModified(files)) {
            needsReloading = true;
            break;
        }
    }
    
    if(needsReloading)
        heShaderReload(program);
#endif
    
};


int heShaderGetUniformLocation(HeShaderProgram* program, const std::string& uniform) {
    
    int location;
    if (program->uniforms.find(uniform) != program->uniforms.end()) {
        location = program->uniforms[uniform];
    } else {
        location = glGetUniformLocation(program->programId, uniform.c_str());
        program->uniforms[uniform] = location;
    }
    return location;
    
};

int heShaderGetSamplerLocation(HeShaderProgram* program, const std::string& sampler, const uint8_t requestedSlot) {
    
    int location;
    if (program->samplers.find(sampler) != program->samplers.end()) {
        location = program->samplers[sampler];
    } else {
        unsigned int uloc = glGetUniformLocation(program->programId, sampler.c_str());
        glProgramUniform1i(program->programId, uloc, requestedSlot);
        program->samplers[sampler] = requestedSlot;
        location = requestedSlot;
    }
    
    return location;
    
};

void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const void* data, const HeUniformDataType type) {
    
    int location = heShaderGetUniformLocation(program, uniformName);
    
    switch(type) {
        case HE_UNIFORM_DATA_TYPE_INT:
        case HE_UNIFORM_DATA_TYPE_BOOL:
        heShaderLoadUniform(program, uniformName, ((int*) data)[0]);
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

void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const int value) {
    
    int location = heShaderGetUniformLocation(program, uniformName);
    glUniform1i(location, value);
    
};

void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const float value) {
    
    int location = heShaderGetUniformLocation(program, uniformName);
    glUniform1f(location, value);
    
};

void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const double value) {
    
    int location = heShaderGetUniformLocation(program, uniformName);
    glUniform1d(location, value);
    
};

void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const uint32_t value) {
    
    int location = heShaderGetUniformLocation(program, uniformName);
    glUniform1i(location, value);
    
};

void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const hm::mat4f& value) {
    
    int location = heShaderGetUniformLocation(program, uniformName);
    glUniformMatrix4fv(location, 1, false, &value[0][0]);
    
};

void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const hm::vec2f& value) {
    
    int location = heShaderGetUniformLocation(program, uniformName);
    glUniform2f(location, value.x, value.y);
    
};

void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const hm::vec3f& value) {
    
    int location = heShaderGetUniformLocation(program, uniformName);
    glUniform3f(location, value.x, value.y, value.z);
    
};

void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const hm::vec4f& value) {
    
    int location = heShaderGetUniformLocation(program, uniformName);
    glUniform4f(location, value.x, value.y, value.z, value.w);
    
};

void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const hm::colour& value) {
    
    int location = heShaderGetUniformLocation(program, uniformName);
    glUniform4f(location, getR(&value), getG(&value), getB(&value), getA(&value));
    
};

void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const HeShaderData* data) {
    
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

void heVboCreate(HeVbo* vbo, const std::vector<float>& data) {
    
    glGenBuffers(1, &vbo->vboId);
    glBindBuffer(GL_ARRAY_BUFFER, vbo->vboId);
    glBufferData(GL_ARRAY_BUFFER, (unsigned int)data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    vbo->verticesCount = (unsigned int)data.size() / vbo->dimensions;
    
};

void heVboDestroy(HeVbo* vbo) {
    
    glDeleteBuffers(1, &vbo->vboId);
    vbo->vboId = 0;
    
};

void heVaoCreate(HeVao* vao) {
    
    glGenVertexArrays(1, &vao->vaoId);
    
};

void heVaoAddVboData(HeVbo* vbo, const int8_t attributeIndex) {
    
    heVboCreate(vbo, vbo->data);
    glVertexAttribPointer(attributeIndex, vbo->dimensions, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    vbo->data.clear();
    
};

void heVaoAddVbo(HeVao* vao, HeVbo* vbo) {
    
    glVertexAttribPointer((uint32_t) vao->vbos.size(), vbo->dimensions, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    vao->vbos.emplace_back(*vbo);
    if (vao->vbos.size() == 1)
        vao->verticesCount = vbo->verticesCount;
    
};

void heVaoAddData(HeVao* vao, const std::vector<float>& data, const uint8_t dimensions) {
    
    if(heIsMainThread()) {
        HeVbo vbo;
        vbo.dimensions = dimensions;
        heVboCreate(&vbo, data);
        heVaoAddVbo(vao, &vbo);
    } else {
        HeVbo* vbo = &vao->vbos.emplace_back();
        vbo->data = data;
        vbo->dimensions = dimensions;
    }
    
};

void heVaoBind(const HeVao* vao) {
    
    glBindVertexArray(vao->vaoId);
    for (uint32_t i = 0; i < (uint32_t)vao->vbos.size(); ++i)
        glEnableVertexAttribArray(i);
    
};

void heVaoUnbind(const HeVao* vao) {
    
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

void heVaoRender(const HeVao* vao) {
    
    glDrawArrays(GL_TRIANGLES, 0, vao->verticesCount);
    
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

void heFboUnbind(const hm::vec2i& windowSize) {
    
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

void heFboResize(HeFbo* fbo, const hm::vec2i& newSize) {
    
    heFboDestroy(fbo);
    fbo->size = newSize;
    
    // the amount of extra textures / buffers attached to this fbo (minus the default one)
    int colourTextures = (int)fbo->colourTextures.size() - 1;
    int colourBuffers = (int)fbo->colourBuffers.size() - 1;
    
    fbo->colourTextures.clear();
    fbo->colourBuffers.clear();
    
    heFboCreate(fbo);
    for (int i = 0; i < colourTextures; ++i)
        heFboCreateColourAttachment(fbo);
    
    for (int i = 0; i < colourBuffers; ++i)
        heFboCreateMultisampledColourAttachment(fbo);
    
};

void heFboRender(HeFbo* sourceFbo, HeFbo* targetFbo) {
    
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, targetFbo->fboId);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, sourceFbo->fboId);
    glDrawBuffer(GL_BACK);
    glBlitFramebuffer(0, 0, sourceFbo->size.x, sourceFbo->size.y, 0, 0, targetFbo->size.x, targetFbo->size.y, 
                      GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    heFboUnbind();
    
};

void heFboRender(HeFbo* sourceFbo, const hm::vec2i& windowSize) {
    
    HeFbo fake;
    fake.fboId = 0;
    fake.size = windowSize;
    heFboRender(sourceFbo, &fake);
    
};


// --- Textures

void heTextureCreateEmpty(HeTexture* texture) {
    
    heTextureCreateFromBuffer(nullptr, &texture->textureId, texture->width, texture->height, texture->channels, texture->format);
    
};

void heTextureLoadFromFile(HeTexture* texture, const std::string& fileName) {
    
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

void heTextureCreateFromBuffer(unsigned char* buffer, uint32_t* id, const int16_t width, const int16_t height, const int8_t channels, const HeColourFormat format) 
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

void heTextureBind(const HeTexture* texture, const int8_t slot) {
    
    if(texture != nullptr) {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, texture->textureId);
    } else {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
};

void heTextureBind(const uint32_t texture, const int8_t slot) {
    
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, texture);
    
};

void heImageTextureBind(const HeTexture* texture, const int8_t slot, const int8_t layer, const HeAccessType access) {
    
    if(texture != nullptr)
        glBindImageTexture(slot, texture->textureId, 0, (layer == -1) ? GL_TRUE : GL_FALSE, layer, access, texture->format);
    
};

void heTextureUnbind(const int8_t slot) {
    
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

void heFrameClear(const hm::colour& colour, const int8_t type) {
    
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

void heBlendMode(const int8_t mode) {
    
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

void heBufferBlendMode(const int8_t attachmentIndex, const int8_t mode) {
    
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

void heEnableDepth(const b8 depth) {
    
    if (depth)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
    
};

void heViewport(const hm::vec2i& lowerleft, const hm::vec2i& size) {
    
    glViewport(lowerleft.x, lowerleft.y, size.x, size.y);
    
};