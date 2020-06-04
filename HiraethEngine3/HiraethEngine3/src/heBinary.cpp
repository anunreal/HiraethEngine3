#include "heBinary.h"
#include "heCore.h"
#include "heLoader.h"
#include "heWin32Layer.h"

b8 heBinaryBufferOpenFile(HeBinaryBuffer* buffer, std::string const& fileName, uint32_t const maxSize, HeAccessType const access) {
    buffer->maxSize  = maxSize;
    buffer->ptr      = (char*) malloc(buffer->maxSize);
    buffer->access   = access;

    size_t index = fileName.find_last_of('/');
    buffer->fullPath = fileName;
    if(index == std::string::npos)
        buffer->name = buffer->fullPath;
    else
        buffer->name = fileName.substr(index + 1, fileName.find('.'));
    b8 success = true;
    
    if(access == HE_ACCESS_READ_ONLY || access == HE_ACCESS_READ_WRITE) {
        buffer->in.open(fileName, std::ios::in | std::ios::binary);
        if(!buffer->in)
            success = false;
        else
            heBinaryBufferReadFromFile(buffer);
    }
    
    if(access == HE_ACCESS_WRITE_ONLY || access == HE_ACCESS_READ_WRITE) {
        HE_LOG("Creating folder [" + buffer->fullPath.substr(0, index) + "]");
        heWin32FolderCreate(buffer->fullPath.substr(0, index));
        buffer->out.open(buffer->fullPath, std::ios::out | std::ios::binary);
        success = buffer->out.is_open();
    }

    if(!success) {
        HE_LOG("Could not open binary buffer [" + fileName + "]");
    }
    
    return success;
};

void heBinaryBufferCloseFile(HeBinaryBuffer* buffer) {
    if(buffer->out.good()) {
        if(buffer->offset > 0)
            buffer->out.write(buffer->ptr, buffer->offset);
        buffer->out.close();
    }
    
    if(buffer->in.good())
        buffer->in.close();
    
    free(buffer->ptr);
    buffer->maxSize = 0;
    buffer->offset = 0;
    buffer->ptr = nullptr;
};

char heBinaryBufferPeek(HeBinaryBuffer* buffer) {
    if(buffer->offset < buffer->size)
        return buffer->ptr[buffer->offset];
    else
        return buffer->in.peek();
};

b8 heBinaryBufferAvailable(HeBinaryBuffer* buffer) {
    return (buffer->offset < buffer->size) || (buffer->in.peek() != EOF);
};

void heBinaryBufferReadFromFile(HeBinaryBuffer* buffer) {
    if(buffer->in.good()) {
        buffer->in.read(buffer->ptr, buffer->maxSize);
        buffer->offset = 0;
        buffer->size = (uint32_t) buffer->in.gcount();
    }
};

void heBinaryBufferAdd(HeBinaryBuffer* buffer, void const* input, uint32_t size) {
    uint32_t space = buffer->maxSize - buffer->offset;
    
    if(size < space) {
        memcpy(&buffer->ptr[buffer->offset], (char*) input, size);
        buffer->offset += size;
    } else {
        uint32_t written = 0;
        while(size > 0) {
            memcpy(&buffer->ptr[buffer->offset], &((char*) input)[written], space);
            buffer->offset += space;
            buffer->out.write(buffer->ptr, buffer->offset);
            buffer->offset = 0;
            size -= space;
            written += space;
            space = size;
            if(space > buffer->maxSize)
                space = buffer->maxSize;
        }
    }
};

void heBinaryBufferAdd(HeBinaryBuffer* buffer, char input) {
    if(buffer->offset == buffer->maxSize) {
        buffer->out.write(buffer->ptr, buffer->maxSize);
        buffer->offset = 0;
    }
    
    buffer->ptr[buffer->offset++] = input;
};

void heBinaryBufferAddString(HeBinaryBuffer* buffer, std::string const& in) {
    const char* cstr = in.c_str();
    heBinaryBufferAddInt(buffer, (int32_t) in.size()); // size of string
    heBinaryBufferAdd(buffer, cstr, (uint32_t) in.size());
};

void heBinaryBufferAddFloat(HeBinaryBuffer* buffer, float const in) {
    char bytes[sizeof(float)];
    *(float*) (bytes) = in;
    heBinaryBufferAdd(buffer, (void const*) &bytes[0], sizeof(float));
};

void heBinaryBufferAddInt(HeBinaryBuffer* buffer, int32_t const in) {
    char bytes[4];
    bytes[0] = (char) (in >> 24);
    bytes[1] = (char) (in >> 16);
    bytes[2] = (char) (in >> 8);
    bytes[3] = (char) (in);
    heBinaryBufferAdd(buffer, (void const*) &bytes[0], 4);
};

void heBinaryBufferAddFloatBuffer(HeBinaryBuffer* buffer, std::vector<float> const& floats) {
    heBinaryBufferAddInt(buffer, (int32_t) floats.size() * sizeof(float)); // size of buffer in bytes
    heBinaryBufferAdd(buffer, (void const*) floats.data(), (uint32_t) (floats.size() * sizeof(float)));
};


b8 heBinaryBufferCopy(HeBinaryBuffer* buffer, void* output, uint32_t size) {
    uint32_t space = buffer->maxSize - buffer->offset;
    if(size <= space) {
        memcpy(output, &buffer->ptr[buffer->offset], size);
        buffer->offset += size;
    } else {
        memcpy((char*)output, &buffer->ptr[buffer->offset], space); // write remaining buffer
        uint32_t copied = space;
        size -= space;
        
        while(size > 0) {
            heBinaryBufferReadFromFile(buffer);
            
            space = buffer->maxSize;
            if(size < space)
                space = size;
            
            memcpy(&((char*)output)[copied], &buffer->ptr[buffer->offset], space);
            buffer->offset = space;
            copied += space;
            size -= space;
        }
    }
    
    return true;
};

b8 heBinaryBufferGetString(HeBinaryBuffer* buffer, std::string* output) {
    int32_t size;
    heBinaryBufferGetInt(buffer, &size);
    output->resize(size);
    heBinaryBufferCopy(buffer, output->data(), size);
    return true;
};

b8 heBinaryBufferGetFloat(HeBinaryBuffer* buffer, float* output) {
    uint8_t bytes[4];
    
    heBinaryBufferCopy(buffer, bytes, 4);
    *output = *(float*) (bytes);
    return true;
};

b8 heBinaryBufferGetInt(HeBinaryBuffer* buffer, int32_t* output) {
    uint8_t bytes[4];
    
    heBinaryBufferCopy(buffer, bytes, 4);
    *output = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | (bytes[3]);
    return true;
};

b8 heBinaryBufferGetFloatBuffer(HeBinaryBuffer* buffer, std::vector<float>* output) {
    int32_t size; // in bytes
    heBinaryBufferGetInt(buffer, &size);
    output->clear();
    output->resize(size / sizeof(float));
    heBinaryBufferCopy(buffer, output->data(), size);
    return true;
};

unsigned char* heTextureLoadFromBinaryFile(std::string const& file, int32_t* width, int32_t* height, int32_t* channels, HeColourFormat* format, std::vector<int32_t>* sizes, int32_t* compressionFormat) {
    HeBinaryBuffer in;
    if(!heBinaryBufferOpenFile(&in, file, 8192, HE_ACCESS_READ_ONLY)) {
        HE_ERROR("Could not find binary texture [" + file + "]");
        return nullptr;
    }

    int32_t mipmaps = 0;
    int32_t totalSize = 0;
    int32_t formatInternal = 0;
    heBinaryBufferGetInt(&in, width);
    heBinaryBufferGetInt(&in, height);
    heBinaryBufferGetInt(&in, channels);
    heBinaryBufferGetInt(&in, &formatInternal);
    heBinaryBufferGetInt(&in, compressionFormat);
    heBinaryBufferGetInt(&in, &mipmaps);
    *format = (HeColourFormat) formatInternal;
    
    sizes->reserve(mipmaps);
    
    // load mip levels
    for(int32_t i = 0; i < mipmaps; ++i) {
        int32_t size = 0;
        heBinaryBufferGetInt(&in, &size);
        sizes->emplace_back(size);
        totalSize += size;
    }
    
    unsigned char* buffer = (unsigned char*) malloc(totalSize);
    heBinaryBufferCopy(&in, buffer, totalSize);
    heBinaryBufferCloseFile(&in);
    return buffer;
};

float* heTextureLoadFromHdrBinaryFile(std::string const& file, int32_t* width, int32_t* height, int32_t* channels, HeColourFormat* format, std::vector<int32_t>* sizes) {
    HeBinaryBuffer in;
    if(!heBinaryBufferOpenFile(&in, file, 8192, HE_ACCESS_READ_ONLY)) {
        HE_ERROR("Could not find binary texture [" + file + "]");
        return nullptr;
    }

    int32_t mipmaps = 0;
    int32_t totalSize = 0;
    int32_t formatInternal = 0;
    int32_t compressionFormat = 0;
    heBinaryBufferGetInt(&in, width);
    heBinaryBufferGetInt(&in, height);
    heBinaryBufferGetInt(&in, channels);
    heBinaryBufferGetInt(&in, &formatInternal);
    heBinaryBufferGetInt(&in, &compressionFormat); // ignored because we cant compress hdr images anyway
    heBinaryBufferGetInt(&in, &mipmaps);
    *format = (HeColourFormat) formatInternal;
    
    sizes->reserve(mipmaps);
    
    // load mip levels
    for(int32_t i = 0; i < mipmaps; ++i) {
        int32_t size = 0;
        heBinaryBufferGetInt(&in, &size);
        sizes->emplace_back(size);
        totalSize += size;
    }
    
    float* buffer = (float*) malloc(totalSize);
    heBinaryBufferCopy(&in, buffer, totalSize);
    heBinaryBufferCloseFile(&in);
    return buffer;
};
