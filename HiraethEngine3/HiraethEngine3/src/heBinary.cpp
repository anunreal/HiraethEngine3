#include "heBinary.h"
#include "heCore.h"
#include "heLoader.h"

#pragma warning(push, 0)
#include "..\stb_image.h"
#pragma warning(pop)

b8 heBinaryBufferOpenFile(HeBinaryBuffer* buffer, std::string const& fileName, uint32_t const maxSize, HeAccessType const access) {
    buffer->maxSize = maxSize;
    buffer->ptr     = (char*) malloc(buffer->maxSize);
    buffer->access  = access;
    b8 success = true;
    
    if(access == HE_ACCESS_READ_ONLY || access == HE_ACCESS_READ_WRITE) {
        buffer->in.open(fileName, std::ios::in | std::ios::binary);
        if(!buffer->in)
            success = false;
        else
            heBinaryBufferReadFromFile(buffer);
    }
    
    if(access == HE_ACCESS_WRITE_ONLY || access == HE_ACCESS_READ_WRITE) {
        buffer->out.open(fileName, std::ios::out | std::ios::binary);
        success = buffer->out.is_open();
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


void heBinaryConvertD3InstanceFile(std::string const& inFile, std::string const& outFile) {
    std::ifstream in(inFile);
    
    HeBinaryBuffer buffer;
    heBinaryBufferOpenFile(&buffer, outFile, 4096, HE_ACCESS_WRITE_ONLY);
    
    if(!in) {
        HE_ERROR("Could not open asset file [" + inFile + "] for binary converting");
        return;
    }
    
    // parse material info
    std::string line;
    std::getline(in, line);
    while(!line.empty()) {
        heBinaryBufferAddString(&buffer, line);
        std::getline(in, line);
    }
    
    heBinaryBufferAdd(&buffer, '\n');
    float result = 0.f;
    
    // parse vertices
    {
        std::vector<float> vertices;
        while(heAsciiStreamParseFloatFixedWidth(in, &result))
            vertices.emplace_back(result);
        heBinaryBufferAddFloatBuffer(&buffer, vertices);
    }
    
    // parse uvs
    {
        std::vector<float> uvs;
        while(heAsciiStreamParseFloatFixedWidth(in, &result))
            uvs.emplace_back(result);
        heBinaryBufferAddFloatBuffer(&buffer, uvs);
    }
    
    // parse normals
    {
        std::vector<float> normals;
        while(heAsciiStreamParseFloatFixedWidth(in, &result))
            normals.emplace_back(result);
        heBinaryBufferAddFloatBuffer(&buffer, normals);
    }
    
    // parse tangents
    {
        std::vector<float> tangents;
        while(heAsciiStreamParseFloatFixedWidth(in, &result))
            tangents.emplace_back(result);
        heBinaryBufferAddFloatBuffer(&buffer, tangents);
    }
    
    // parse physics
    if(in.peek() != '\n') {
        // type
        char typeChar;
        in.get(typeChar);
        uint32_t type = uint32_t(typeChar - '0');
        heBinaryBufferAddInt(&buffer, type);
        float info[3];
        heAsciiStreamParseFloatFixedWidth(in, &info[0]);
        heAsciiStreamParseFloatFixedWidth(in, &info[1]);
        heAsciiStreamParseFloatFixedWidth(in, &info[2]);
        heBinaryBufferAddFloat(&buffer, info[0]);
        heBinaryBufferAddFloat(&buffer, info[1]);
        heBinaryBufferAddFloat(&buffer, info[2]);
        
        std::vector<float> data;
        switch(type) {
            case HE_PHYSICS_SHAPE_CONCAVE_MESH:
            case HE_PHYSICS_SHAPE_CONVEX_MESH: {
                hm::vec3f vec;
                while(heAsciiStreamParseFloats(in, 3, &vec)) {
                    data.emplace_back(vec.x);
                    data.emplace_back(vec.y);
                    data.emplace_back(vec.z);
                }
                break;
            }
            
            case HE_PHYSICS_SHAPE_BOX: {
                hm::vec3f box;
                heAsciiStreamParseFloats(in, 3, &box);
                data.emplace_back(box.x);
                data.emplace_back(box.y);
                data.emplace_back(box.z);
                break;
            }
            
            case HE_PHYSICS_SHAPE_SPHERE:
            heAsciiStreamParseFloatFixedWidth(in, &data.emplace_back());
            break;
            
            case HE_PHYSICS_SHAPE_CAPSULE: {
                hm::vec2f capsule;
                heAsciiStreamParseFloats(in, 2, &capsule);
                data.emplace_back(capsule.x);
                data.emplace_back(capsule.y);
                break;
            }
        }
        
        heBinaryBufferAddFloatBuffer(&buffer, data);
    }
    
    in.close();
    heBinaryBufferCloseFile(&buffer);
};

void heBinaryConvertTexture(std::string const& inFile, std::string const& outFile) {
    int32_t width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* buffer = stbi_load(inFile.c_str(), &width, &height, &channels, 0);
    uint32_t count = width * height * channels;
    
    HeBinaryBuffer out;
    heBinaryBufferOpenFile(&out, outFile, 4096, HE_ACCESS_WRITE_ONLY);
    heBinaryBufferAddInt(&out, width);
    heBinaryBufferAddInt(&out, height);
    heBinaryBufferAddInt(&out, channels);
    heBinaryBufferAdd(&out, (void const*) (&buffer[0]), count);
    heBinaryBufferCloseFile(&out);
    stbi_image_free(buffer);
};

unsigned char* heBinaryLoadTexture(std::string const& file, int32_t* width, int32_t* height, int32_t* channels) {
    HeBinaryBuffer in;
    if(!heBinaryBufferOpenFile(&in, file, 8192, HE_ACCESS_READ_ONLY)) {
        HE_ERROR("Could not find binary texture [" + file + "]");
        return nullptr;
    }
    
    heBinaryBufferGetInt(&in, width);
    heBinaryBufferGetInt(&in, height);
    heBinaryBufferGetInt(&in, channels);
    uint32_t size = (*width) * (*height) * (*channels);
    unsigned char* buffer = (unsigned char*) malloc(size);
    heBinaryBufferCopy(&in, buffer, size);
    heBinaryBufferCloseFile(&in);
    return buffer;
};
