#include "heConverter.h"
#include "heBinary.h"
#include "heAssets.h"
#include "heCore.h"

#pragma warning(push, 0)
#include "..\stb_image.h"
#pragma warning(pop)

void heBinaryConvertD3InstanceFile(std::string const& inFile, std::string const& outFile) {
    HeTextFile in;
    in.skipEmptyLines = false;
    heTextFileOpen(&in, inFile, 0, false);
    
    HeBinaryBuffer buffer;
    heBinaryBufferOpenFile(&buffer, outFile, 4096, HE_ACCESS_WRITE_ONLY);
    
    if(!in.open) {
        HE_ERROR("Could not open asset file [" + inFile + "] for binary converting");
        return;
    }

    if(!buffer.out) {
        HE_ERROR("Could not open asset file [" + inFile + "] for binary converting");
        return;
    }
    
    // parse material info
    std::string line;
    heTextFileGetLine(&in, &line);
    while(!line.empty()) {
        heBinaryBufferAddString(&buffer, line);
        heTextFileGetLine(&in, &line);
    }
    
    heBinaryBufferAdd(&buffer, '\n');
    float result = 0.f;
    
    // parse vertices
    {
        std::vector<float> vertices;
        while(heTextFileGetFloat(&in, &result))
            vertices.emplace_back(result);
        heBinaryBufferAddFloatBuffer(&buffer, vertices);
    }
    
    // parse uvs
    {
        std::vector<float> uvs;
        while(heTextFileGetFloat(&in, &result))
            uvs.emplace_back(result);
        heBinaryBufferAddFloatBuffer(&buffer, uvs);
    }
    
    // parse normals
    {
        std::vector<float> normals;
        while(heTextFileGetFloat(&in, &result))
            normals.emplace_back(result);
        heBinaryBufferAddFloatBuffer(&buffer, normals);
    }
    
    // parse tangents
    {
        std::vector<float> tangents;
        while(heTextFileGetFloat(&in, &result))
            tangents.emplace_back(result);
        heBinaryBufferAddFloatBuffer(&buffer, tangents);
    }
    
    // parse physics
    if(heTextFilePeek(&in) != '\n') {
        // type
        char typeChar;
        heTextFileGetChar(&in, &typeChar);
        uint32_t type = uint32_t(typeChar - '0');
        heBinaryBufferAddInt(&buffer, type);
        float info[3];
        heTextFileGetFloat(&in, &info[0]);
        heTextFileGetFloat(&in, &info[1]);
        heTextFileGetFloat(&in, &info[2]);
        heBinaryBufferAddFloat(&buffer, info[0]);
        heBinaryBufferAddFloat(&buffer, info[1]);
        heBinaryBufferAddFloat(&buffer, info[2]);
        
        std::vector<float> data;
        switch(type) {
        case HE_PHYSICS_SHAPE_CONCAVE_MESH:
        case HE_PHYSICS_SHAPE_CONVEX_MESH: {
            hm::vec3f vec;
            while(heTextFileGetFloats(&in, 3, &vec)) {
                data.emplace_back(vec.x);
                data.emplace_back(vec.y);
                data.emplace_back(vec.z);
            }
            break;
        }
            
        case HE_PHYSICS_SHAPE_BOX: {
            hm::vec3f box;
            heTextFileGetFloats(&in, 3, &box);
            data.emplace_back(box.x);
            data.emplace_back(box.y);
            data.emplace_back(box.z);
            break;
        }
            
        case HE_PHYSICS_SHAPE_SPHERE:
            heTextFileGetFloat(&in, &data.emplace_back());
            break;
            
        case HE_PHYSICS_SHAPE_CAPSULE: {
            hm::vec2f capsule;
            heTextFileGetFloats(&in, 2, &capsule);
            data.emplace_back(capsule.x);
        data.emplace_back(capsule.y);
        break;
        }
        }
        
        heBinaryBufferAddFloatBuffer(&buffer, data);
    }
    
    heTextFileClose(&in);
    heBinaryBufferCloseFile(&buffer);
};

void heTextureCompress(std::string const& inFile, std::string const& outFile) {
    HeTexture texture;
    texture.parameters = HE_TEXTURE_FILTER_TRILINEAR | HE_TEXTURE_CLAMP_REPEAT;
    heTextureLoadFromImageFile(&texture, inFile, true);
    int32_t format = heTextureGetCompressionFormat(&texture);
    int32_t mipmaps = texture.mipmapCount;
    if(mipmaps > 5)
        mipmaps = 5;
    
    HeBinaryBuffer out;
    if(!heBinaryBufferOpenFile(&out, outFile, 4096, HE_ACCESS_WRITE_ONLY))
        return;

    heBinaryBufferAddInt(&out, texture.size.x);
    heBinaryBufferAddInt(&out, texture.size.y);
    heBinaryBufferAddInt(&out, texture.channels);
    heBinaryBufferAddInt(&out, texture.format); // colour format
    heBinaryBufferAddInt(&out, format); // compression format
    heBinaryBufferAddInt(&out, mipmaps);

    std::vector<unsigned char*> buffers;
    std::vector<int32_t> bufferSizes;
    buffers.reserve(mipmaps);
    
    for(int32_t i = 0; i < mipmaps; ++i) {
        int32_t size = 0;
        unsigned char* buffer = (unsigned char*) heTextureGetCompressedData(&texture, &size, i);
        heBinaryBufferAddInt(&out, size);
        buffers.emplace_back(buffer);
        bufferSizes.emplace_back(size);
    }

    for(int32_t i = 0; i < mipmaps; ++i) {
        heBinaryBufferAdd(&out, (void const*) (&buffers[i][0]), bufferSizes[i]);
        free(buffers[i]);
    }
    
    heBinaryBufferCloseFile(&out);
    heTextureDestroy(&texture);
};

void heTextureExport(HeTexture* texture, std::string const& outFile) {
    b8 isHdr = texture->format == HE_COLOUR_FORMAT_RGB16 || texture->format == HE_COLOUR_FORMAT_RGBA16;
    heTextureBind(texture, 0);
    
    int32_t format = 0;
    if(texture->compressionFormat)
        format = heTextureGetCompressionFormat(texture);

    int32_t mipmaps = texture->mipmapCount;
    if(mipmaps > 5)
        mipmaps = 5;
    
    HeBinaryBuffer out;
    if(!heBinaryBufferOpenFile(&out, outFile, 4096, HE_ACCESS_WRITE_ONLY))
        return;

    heBinaryBufferAddInt(&out, texture->size.x);
    heBinaryBufferAddInt(&out, texture->size.y);
    heBinaryBufferAddInt(&out, texture->channels);
    heBinaryBufferAddInt(&out, texture->format);
    heBinaryBufferAddInt(&out, format);
    heBinaryBufferAddInt(&out, mipmaps);

    std::vector<void*> buffers;
    std::vector<int32_t> bufferSizes;
    buffers.reserve(mipmaps);
    
    for(int32_t i = 0; i < mipmaps; ++i) {
        int32_t size = 0;
        void* buffer = heTextureGetData(texture, &size, i);
        heBinaryBufferAddInt(&out, size);
        buffers.emplace_back(buffer);
        bufferSizes.emplace_back(size);
    }

    for(int32_t i = 0; i < mipmaps; ++i) {
        heBinaryBufferAdd(&out, buffers[i], bufferSizes[i]);            
        free(buffers[i]);
    }
    
    heBinaryBufferCloseFile(&out);
};
