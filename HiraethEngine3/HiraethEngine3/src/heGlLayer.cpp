#include "heGlLayer.h"
#include "heAssets.h"
#include "GLEW/glew.h"
#include "GLEW/wglew.h"
#include "heWin32Layer.h"
#include <fstream>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


// --- Shaders

std::string heLoadShaderSource(const std::string& file) {
    
    std::ifstream stream(file);
    if (!stream) {
        std::cout << "Error: Could not find shader file [" << file << "]" << std::endl;
        return "";
    }
    
    std::string source;
    std::string line;
    
    while (std::getline(stream, line))
        source += line + "\n";
    
    stream.close();
    return source;
    
};

void heLoadComputeShader(HeShaderProgram* program, const std::string& computeShader) {
    
    unsigned int cid = glCreateShader(GL_COMPUTE_SHADER);
    int err = glGetError();
    if (err != GL_NO_ERROR)
        std::cout << "Error: Shader Creation: " << err << std::endl;
    
    std::string cs_src = heLoadShaderSource(computeShader);
    const char* cs_ptr = cs_src.c_str();
    
    glShaderSource(cid, 1, &cs_ptr, NULL);
    glCompileShader(cid);
    
    int compileStatus;
    glGetShaderiv(cid, GL_COMPILE_STATUS, &compileStatus);
    if (!compileStatus) {
        char infoLog[512];
        memset(infoLog, 0, 512);
        glGetShaderInfoLog(cid, 512, NULL, infoLog);
        std::cout << "Error: While compiling shader [" + computeShader + "]" << std::endl;
        std::cout << std::string(infoLog) << std::endl;
    }
    
    unsigned int programId = glCreateProgram();
    glAttachShader(programId, cid);
    glLinkProgram(programId);
    
    int success = 0;
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        memset(infoLog, 0, 512);
        glGetProgramInfoLog(programId, 512, NULL, infoLog);
        std::cout << "Error: Unable to link shader!" << std::endl;
        std::cout << infoLog << std::endl;
    }
    
    glUseProgram(programId);
    glDeleteShader(cid);
    program->programId = programId;
    
};

void heLoadShader(HeShaderProgram* program, const std::string& vertexShader, const std::string& fragmentShader) {
    
    std::string vs = heLoadShaderSource(vertexShader);
    std::string fs = heLoadShaderSource(fragmentShader);
    
    if (vs.empty() || fs.empty())
        return;
    
    unsigned int vid = glCreateShader(GL_VERTEX_SHADER);
    unsigned int fid = glCreateShader(GL_FRAGMENT_SHADER);
    
    int err = glGetError();
    if (err != GL_NO_ERROR)
        std::cout << "Error: Shader Creation: " << err << std::endl;
    
    
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
        std::cout << "Error: While compiling shader [" + vertexShader + "]" << std::endl;
        std::cout << std::string(infoLog) << std::endl;
    }
    
    glShaderSource(fid, 1, &fs_ptr, NULL);
    glCompileShader(fid);
    
    glGetShaderiv(fid, GL_COMPILE_STATUS, &compileStatus);
    if (!compileStatus) {
        char infoLog[512];
        memset(infoLog, 0, 512);
        glGetShaderInfoLog(fid, 512, NULL, infoLog);
        std::cout << "Error: While compiling shader [" + fragmentShader + "]" << std::endl;
        std::cout << std::string(infoLog) << std::endl;
    }
    
    unsigned int programId = glCreateProgram();
    glAttachShader(programId, vid);
    glAttachShader(programId, fid);
    glLinkProgram(programId);
    
    int success = 0;
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        memset(infoLog, 0, 512);
        glGetProgramInfoLog(programId, 512, NULL, infoLog);
        std::cout << "Error: Unable to link shader!" << std::endl;
        std::cout << infoLog << std::endl;
    }
    
    glUseProgram(0);
    glDeleteShader(vid);
    glDeleteShader(fid);
    
    program->programId = programId;
    
};

void heBindShader(const HeShaderProgram* program) {
    
    glUseProgram(program->programId);
    
};

void heUnbindShader() {
    
    glUseProgram(0);
    
};

void heDestroyShader(HeShaderProgram* program) {
    
    glDeleteProgram(program->programId);
    program->programId = 0;
    
};

void heRunComputeShader(const HeShaderProgram* program, const unsigned int groupsX, const unsigned int groupsY, const unsigned int groupsZ) {
    
    heBindShader(program);
    glDispatchCompute(groupsX, groupsY, groupsZ);
    
};

void heReloadShader(HeShaderProgram* program) {
    
};


int heGetShaderUniformLocation(HeShaderProgram* program, const std::string& uniform) {
    
    int location;
    if (program->uniforms.find(uniform) != program->uniforms.end()) {
        location = program->uniforms[uniform];
    } else {
        location = glGetUniformLocation(program->programId, uniform.c_str());
        program->uniforms[uniform] = location;
    }
    return location;
    
};

int heGetShaderSamplerLocation(HeShaderProgram* program, const std::string& sampler, const unsigned int requestedSlot) {
    
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


void heLoadShaderUniform(HeShaderProgram* program, const std::string& uniformName, const int value) {
    
    int location = heGetShaderUniformLocation(program, uniformName);
    glUniform1i(location, value);
    
};

void heLoadShaderUniform(HeShaderProgram* program, const std::string& uniformName, const float value) {
    
    int location = heGetShaderUniformLocation(program, uniformName);
    glUniform1f(location, value);
    
};

void heLoadShaderUniform(HeShaderProgram* program, const std::string& uniformName, const double value) {
    
    int location = heGetShaderUniformLocation(program, uniformName);
    glUniform1d(location, value);
    
};

void heLoadShaderUniform(HeShaderProgram* program, const std::string& uniformName, const uint32_t value) {
    
    int location = heGetShaderUniformLocation(program, uniformName);
    glUniform1i(location, value);
    
};

void heLoadShaderUniform(HeShaderProgram* program, const std::string& uniformName, const hm::mat4f& value) {
    
    int location = heGetShaderUniformLocation(program, uniformName);
    glUniformMatrix4fv(location, 1, false, &value[0][0]);
    
};

void heLoadShaderUniform(HeShaderProgram* program, const std::string& uniformName, const hm::vec2f& value) {
    
    int location = heGetShaderUniformLocation(program, uniformName);
    glUniform2f(location, value.x, value.y);
    
};

void heLoadShaderUniform(HeShaderProgram* program, const std::string& uniformName, const hm::vec3f& value) {
    
    int location = heGetShaderUniformLocation(program, uniformName);
    glUniform3f(location, value.x, value.y, value.z);
    
};

void heLoadShaderUniform(HeShaderProgram* program, const std::string& uniformName, const hm::vec4f& value) {
    
    int location = heGetShaderUniformLocation(program, uniformName);
    glUniform4f(location, value.x, value.y, value.z, value.w);
    
};

void heLoadShaderUniform(HeShaderProgram* program, const std::string& uniformName, const hm::colour& value) {
    
    int location = heGetShaderUniformLocation(program, uniformName);
    glUniform4f(location, value.getR(), value.getG(), value.getB(), value.getA());
    
};

void heLoadShaderUniform(HeShaderProgram* program, const std::string& uniformName, const HeShaderData* data) {
    
    switch (data->type) {
        case HE_SHADER_DATA_TYPE_INT:
        heLoadShaderUniform(program, uniformName, data->_int);
        break;
        
        case HE_SHADER_DATA_TYPE_FLOAT:
        heLoadShaderUniform(program, uniformName, data->_float);
        break;
        
        case HE_SHADER_DATA_TYPE_VEC2:
        heLoadShaderUniform(program, uniformName, data->_vec2);
        break;
        
        case HE_SHADER_DATA_TYPE_COLOUR:
        heLoadShaderUniform(program, uniformName, data->_colour);
        break;
    };
    
};



// --- Buffers

void heCreateVbo(HeVbo* vbo, const std::vector<float>& data) {
    
    glGenBuffers(1, &vbo->vboId);
    glBindBuffer(GL_ARRAY_BUFFER, vbo->vboId);
    glBufferData(GL_ARRAY_BUFFER, (unsigned int)data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    vbo->verticesCount = (unsigned int)data.size() / vbo->dimensions;
    
};

void heDestroyVbo(HeVbo* vbo) {
    
    glDeleteBuffers(1, &vbo->vboId);
    vbo->vboId = 0;
    
};

void heCreateVao(HeVao* vao) {
    
    glGenVertexArrays(1, &vao->vaoId);
    
};

void heAddVbo(HeVao* vao, HeVbo* vbo) {
    
    glVertexAttribPointer((unsigned int)vao->vbos.size(), vbo->dimensions, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    vao->vbos.emplace_back(*vbo);
    if (vao->vbos.size() == 1)
        vao->verticesCount = vbo->verticesCount;
    
};

void heAddVaoData(HeVao* vao, const std::vector<float>& data, const unsigned int dimensions) {
    
    HeVbo vbo;
    vbo.dimensions = dimensions;
    heCreateVbo(&vbo, data);
    heAddVbo(vao, &vbo);
    
};

void heBindVao(const HeVao* vao) {
    
    glBindVertexArray(vao->vaoId);
    for (unsigned int i = 0; i < (unsigned int)vao->vbos.size(); ++i)
        glEnableVertexAttribArray(i);
    
};

void heUnbindVao(const HeVao* vao) {
    
    for (unsigned int i = 0; i < (unsigned int)vao->vbos.size(); ++i)
        glDisableVertexAttribArray(i);
    glBindVertexArray(0);
    
};

void heDestroyVao(HeVao* vao) {
    
    for (HeVbo& vbos : vao->vbos)
        heDestroyVbo(&vbos);
    
    glDeleteVertexArrays(1, &vao->vaoId);
    vao->vaoId = 0;
    
};

void heRenderVao(const HeVao* vao) {
    
    glDrawArrays(GL_TRIANGLES, 0, vao->verticesCount);
    
};



void heCreateColourAttachment(HeFbo* fbo) {
    
    unsigned int id;
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
    
}

void heCreateMultisampledColourAttachment(HeFbo* fbo) {
    
    unsigned int id;
    glGenRenderbuffers(1, &id);
    glBindRenderbuffer(GL_RENDERBUFFER, id);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, fbo->samples, (fbo->flags & HE_FBO_FLAG_HDR) ? GL_RGBA16F : GL_RGBA8,
                                     fbo->size.x, fbo->size.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (int)fbo->colourBuffers.size(), GL_RENDERBUFFER, id);
    fbo->colourBuffers.emplace_back(id);
    
}

void heCreateDepthBufferAttachment(HeFbo* fbo) {
    
    glGenRenderbuffers(1, &fbo->depthAttachments[1]);
    glBindRenderbuffer(GL_RENDERBUFFER, fbo->depthAttachments[1]);
    if (fbo->samples == 1)
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, fbo->size.x, fbo->size.y);
    else
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, fbo->samples, GL_DEPTH_COMPONENT24, fbo->size.x, fbo->size.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fbo->depthAttachments[1]);
    
}

void heCreateDepthTextureAttachment(HeFbo* fbo) {
    
    glGenTextures(1, &fbo->depthAttachments[0]);
    glBindTexture(GL_TEXTURE_2D, fbo->depthAttachments[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, fbo->size.x, fbo->size.y, 0, GL_DEPTH_COMPONENT,
                 GL_UNSIGNED_INT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, fbo->depthAttachments[0], 0);
    
}

void heCreateFbo(HeFbo* fbo) {
    
    glGenFramebuffers(1, &fbo->fboId);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo->fboId);
    
    if (fbo->samples == 1)
        heCreateColourAttachment(fbo);
    else
        heCreateMultisampledColourAttachment(fbo);
    
    if (fbo->flags & HE_FBO_FLAG_DEPTH_RENDER_BUFFER)
        heCreateDepthBufferAttachment(fbo);
    if (fbo->flags & HE_FBO_FLAG_DEPTH_TEXTURE)
        heCreateDepthTextureAttachment(fbo);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Error: Fbo not set up correctly (" << glGetError() << ")" << std::endl;
    
}

void heBindFbo(HeFbo* fbo) {
    
    glBindFramebuffer(GL_FRAMEBUFFER, fbo->fboId);
    glViewport(0, 0, fbo->size.x, fbo->size.y);
    
    if (fbo->samples > 1)
        glEnable(GL_MULTISAMPLE);
    
    // enable draw buffers
    int count = std::max<int>((int)fbo->colourBuffers.size(), (int)fbo->colourTextures.size());
    std::vector<unsigned int> drawBuffers;
    for (int i = 0; i < count; ++i)
        drawBuffers.emplace_back(GL_COLOR_ATTACHMENT0 + i);
    
    glDrawBuffers(count, drawBuffers.data());
    
}

void heUnbindFbo(const hm::vec2i& windowSize) {
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowSize.x, windowSize.y);
    
}

void heUnbindFbo() {
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
}

void heDestroyFbo(HeFbo* fbo) {
    
    glDeleteFramebuffers(1, &fbo->fboId);
    glDeleteTextures(1, &fbo->depthAttachments[0]); // colour texture
    glDeleteRenderbuffers(1, &fbo->depthAttachments[1]); // multisampled colour buffer
    glDeleteTextures((int)fbo->colourTextures.size(), fbo->colourTextures.data());
    glDeleteRenderbuffers((int)fbo->colourBuffers.size(), fbo->colourBuffers.data());
    fbo->fboId = 0;
    
}

void heResizeFbo(HeFbo* fbo, const hm::vec2i& newSize) {
    
    heDestroyFbo(fbo);
    fbo->size = newSize;
    
    // the amount of extra textures / buffers attached to this fbo (minus the default one)
    int colourTextures = (int)fbo->colourTextures.size() - 1;
    int colourBuffers = (int)fbo->colourBuffers.size() - 1;
    
    fbo->colourTextures.clear();
    fbo->colourBuffers.clear();
    
    heCreateFbo(fbo);
    for (int i = 0; i < colourTextures; ++i)
        heCreateColourAttachment(fbo);
    
    for (int i = 0; i < colourBuffers; ++i)
        heCreateMultisampledColourAttachment(fbo);
    
    
}



// --- Textures

void heLoadTexture(HeTexture* texture, const std::string& fileName) {
    
    stbi_set_flip_vertically_on_load(true);
    unsigned char* buffer = stbi_load(fileName.c_str(), &texture->width, &texture->height, &texture->channels, 4);
    if (buffer == nullptr) {
        std::cout << "Error: Could not open texture [" << fileName << "]!" << std::endl;
        texture->textureId = 0;
        return;
    }
    
    texture->format = HE_COLOUR_FORMAT_RGBA8;
    if (wglGetCurrentContext() != NULL)
        // we are in some context thread, load it right now
        heCreateGlTextureFromBuffer(buffer, &texture->textureId, texture->width, texture->height, texture->channels, HE_COLOUR_FORMAT_RGBA8);
    else
        // no context in the current thread, add it to the thread loader
        heRequestTexture(texture, buffer);
    //heThreadLoader.textures[texture] = buffer;
    
};

void heCreateTexture(HeTexture* texture) {
    
    heCreateGlTextureFromBuffer(nullptr, &texture->textureId, texture->width, texture->height, texture->channels, texture->format);
    
};

void heCreateGlTextureFromBuffer(unsigned char* buffer, unsigned int* id, const int width, const int height, const int channels, 
                                 const HeColourFormat format) 
{
    
    glGenTextures(1, id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, *id);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(buffer);
    
}

void heLoadTexture(HeTexture* texture, FILE* stream) {
    
    stbi_set_flip_vertically_on_load(true);
    unsigned char* buffer = stbi_load_from_file(stream, &texture->width, &texture->height, &texture->channels, 4);
    if (buffer == nullptr) {
        std::cout << "Error: Could not open texture from FILE!" << std::endl;
        texture->textureId = 0;
        return;
    }
    
    /*
    glGenTextures(1, &texture->textureId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->width, texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    */
    texture->format = HE_COLOUR_FORMAT_RGBA8;
    heCreateGlTextureFromBuffer(buffer, &texture->textureId, texture->width, texture->height, texture->channels, HE_COLOUR_FORMAT_RGBA8);
    
    stbi_image_free(buffer);
    
}

void heBindTexture(const HeTexture* texture, const int slot) {
    
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, texture->textureId);
    
};

void heBindTexture(const unsigned int texture, const int slot) {
    
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, texture);
    
}

void heBindImageTexture(const HeTexture* texture, const int slot, const int layer, const HeAccessType access) {
    
    glBindImageTexture(slot, texture->textureId, 0, (layer == -1) ? GL_TRUE : GL_FALSE, layer, access, texture->format);
    
};

void heUnbindTexture(const int slot) {
    
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, 0);
    
};

void heDestroyTexture(HeTexture* texture) {
    
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

void heClearFrame(const hm::colour& colour, const int type) {
    
    glClearColor(colour.getR(), colour.getG(), colour.getB(), colour.getA());
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
    
}

void heBlendMode(const int mode) {
    
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
    
}

void heBufferBlendMode(const int attachmentIndex, const int mode) {
    
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
    
}

void heEnableDepth(const bool depth) {
    
    if (depth)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
    
}
