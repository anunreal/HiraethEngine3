#include "world.h"
#include "heWindow.h"
#include "generator.h"
#include "heWin32Layer.h"
#include "heCore.h"

Block* getBlockAt(Chunk* chunk, BlockPosition const& position) {
    return &chunk->layers[position.y].blocks[position.x * CHUNK_SIZE + position.z];
};

b8 blockHasNeighbour(World* world, Chunk* chunk, BlockPosition const& position, char const direction) {
    b8 hasNeighbour = false;

    switch(direction) {
    case 'T': {
        if(position.y == CHUNK_HEIGHT - 1)
            hasNeighbour = false;
        else
            hasNeighbour = getBlockAt(chunk, position + hm::vec3i(0, 1, 0))->type != BLOCK_TYPE_AIR;
        break;
    }

    case 'D': {
        if(position.y == 0)
            hasNeighbour = false;
        else
            hasNeighbour = getBlockAt(chunk, position - hm::vec3i(0, 1, 0))->type != BLOCK_TYPE_AIR;
        break;
    }

    case 'R': {
        if(position.x == CHUNK_SIZE - 1)
            hasNeighbour = false;
        else
            hasNeighbour = getBlockAt(chunk, position + hm::vec3i(1, 0, 0))->type != BLOCK_TYPE_AIR;
        break;
    }

    case 'L': {
        if(position.x == 0)
            hasNeighbour = false;
        else
            hasNeighbour = getBlockAt(chunk, position - hm::vec3i(1, 0, 0))->type != BLOCK_TYPE_AIR;
        break;
    }

    case 'F': {
        if(position.z == 0)
            hasNeighbour = false;
        else
            hasNeighbour = getBlockAt(chunk, position - hm::vec3i(0, 0, 1))->type != BLOCK_TYPE_AIR;
        break;
    }

    case 'B': {
        if(position.z == CHUNK_SIZE - 1)
            hasNeighbour = false;
        else
            hasNeighbour = getBlockAt(chunk, position + hm::vec3i(0, 0, 1))->type != BLOCK_TYPE_AIR;
        break;
    }
    }

    return hasNeighbour;
};

hm::vec4f getUvCoordsForFace(uint32_t const faceIndex) {
    uint32_t row = faceIndex / (256 / 16);
    return hm::vec4f((16.f * faceIndex) / 256.f, (16.f * row) / 256.f, 16.f / 256.f, 16.f / 256.f);
};

uint32_t getFaceForType(BlockType const type, char const direction) {
    uint32_t face = 0;

    if(type == BLOCK_TYPE_GRASS) {
        switch(direction) {
        case 'L':
        case 'R':
        case 'F':
        case 'B':
            face = 1;
            break;

        case 'T':
            face = 0;
            break;

        case 'D':
            face = 2;
            break;
        }
    } else if(type == BLOCK_TYPE_DIRT)
        face = 2;
    else if(type == BLOCK_TYPE_STONE)
        face = 3;

    return face;
};

void buildChunk(World* world, Chunk* chunk) {
    std::vector<float> vertices;
    std::vector<float> uvs;

    // blocks a 6 faces a 6 positions a 3 floats
    vertices.reserve(chunk->blockCount * 6 * 6 * 3);
    uvs.reserve(chunk->blockCount * 6 * 6 * 2);
    
    for(uint32_t z = 0; z < CHUNK_SIZE; ++z) {
        for(uint32_t x = 0; x < CHUNK_SIZE; ++x) {
            for(uint32_t y = 0; y < CHUNK_HEIGHT; ++y) {
                hm::vec3i position(x, y, z);
                Block* b = getBlockAt(chunk, position);
                if(b->type != BLOCK_TYPE_AIR) {
                    if(!blockHasNeighbour(world, chunk, position, 'B')) { // back face (+z)
                        hm::vec4f faceUvs = getUvCoordsForFace(getFaceForType(b->type, 'B'));

                        vertices.emplace_back(0.f + position.x);
                        vertices.emplace_back(0.f + position.y);
                        vertices.emplace_back(1.f + position.z);

                        vertices.emplace_back(1.f + position.x);
                        vertices.emplace_back(1.f + position.y);
                        vertices.emplace_back(1.f + position.z);

                        vertices.emplace_back(0.f + position.x);
                        vertices.emplace_back(1.f + position.y);
                        vertices.emplace_back(1.f + position.z);

                        vertices.emplace_back(0.f + position.x);
                        vertices.emplace_back(0.f + position.y);
                        vertices.emplace_back(1.f + position.z);

                        vertices.emplace_back(1.f + position.x);
                        vertices.emplace_back(0.f + position.y);
                        vertices.emplace_back(1.f + position.z);

                        vertices.emplace_back(1.f + position.x);
                        vertices.emplace_back(1.f + position.y);
                        vertices.emplace_back(1.f + position.z);

                        uvs.emplace_back(faceUvs.x + faceUvs.z);                        
                        uvs.emplace_back(1.f - (faceUvs.y + faceUvs.w));
                    
                        uvs.emplace_back(faceUvs.x);                        
                        uvs.emplace_back(1.f - faceUvs.y);

                        uvs.emplace_back(faceUvs.x + faceUvs.z);                        
                        uvs.emplace_back(1.f - faceUvs.y);
                    
                        uvs.emplace_back(faceUvs.x + faceUvs.z);                        
                        uvs.emplace_back(1.f - (faceUvs.y + faceUvs.w));
                    
                        uvs.emplace_back(faceUvs.x);
                        uvs.emplace_back(1.f - (faceUvs.y + faceUvs.w));
                    
                        uvs.emplace_back(faceUvs.x);                        
                        uvs.emplace_back(1.f - faceUvs.y);
                    }
                    
                    if(!blockHasNeighbour(world, chunk, position, 'F')){ // front face (-z)
                        hm::vec4f faceUvs = getUvCoordsForFace(getFaceForType(b->type, 'F'));

                        vertices.emplace_back(0.f + position.x);
                        vertices.emplace_back(0.f + position.y);
                        vertices.emplace_back(0.f + position.z);

                        vertices.emplace_back(0.f + position.x);
                        vertices.emplace_back(1.f + position.y);
                        vertices.emplace_back(0.f + position.z);

                        vertices.emplace_back(1.f + position.x);
                        vertices.emplace_back(1.f + position.y);
                        vertices.emplace_back(0.f + position.z);

                        vertices.emplace_back(1.f + position.x);
                        vertices.emplace_back(0.f + position.y);
                        vertices.emplace_back(0.f + position.z);

                        vertices.emplace_back(0.f + position.x);
                        vertices.emplace_back(0.f + position.y);
                        vertices.emplace_back(0.f + position.z);

                        vertices.emplace_back(1.f + position.x);
                        vertices.emplace_back(1.f + position.y);
                        vertices.emplace_back(0.f + position.z);

                        uvs.emplace_back(faceUvs.x);
                        uvs.emplace_back(1.f - (faceUvs.y + faceUvs.w));                        
                        
                        uvs.emplace_back(faceUvs.x);
                        uvs.emplace_back(1.f - faceUvs.y);
                    
                        uvs.emplace_back(faceUvs.x + faceUvs.z);
                        uvs.emplace_back(1.f - faceUvs.y);
                    
                        uvs.emplace_back(faceUvs.x + faceUvs.z);
                        uvs.emplace_back(1.f - (faceUvs.y + faceUvs.w));                        
                    
                        uvs.emplace_back(faceUvs.x);
                        uvs.emplace_back(1.f - (faceUvs.y + faceUvs.w));                        
                                            
                        uvs.emplace_back(faceUvs.x + faceUvs.z);
                        uvs.emplace_back(1.f - faceUvs.y);
                    }

                    if(!blockHasNeighbour(world, chunk, position, 'T')){ // top face (+y)
                        hm::vec4f faceUvs = getUvCoordsForFace(getFaceForType(b->type, 'T'));

                        vertices.emplace_back(0.f + position.x);
                        vertices.emplace_back(1.f + position.y);
                        vertices.emplace_back(0.f + position.z);

                        vertices.emplace_back(0.f + position.x);
                        vertices.emplace_back(1.f + position.y);
                        vertices.emplace_back(1.f + position.z);

                        vertices.emplace_back(1.f + position.x);
                        vertices.emplace_back(1.f + position.y);
                        vertices.emplace_back(1.f + position.z);

                        vertices.emplace_back(1.f + position.x);
                        vertices.emplace_back(1.f + position.y);
                        vertices.emplace_back(0.f + position.z);

                        vertices.emplace_back(0.f + position.x);
                        vertices.emplace_back(1.f + position.y);
                        vertices.emplace_back(0.f + position.z);

                        vertices.emplace_back(1.f + position.x);
                        vertices.emplace_back(1.f + position.y);
                        vertices.emplace_back(1.f + position.z);

                        uvs.emplace_back(faceUvs.x);
                        uvs.emplace_back(1.f - (faceUvs.y + faceUvs.w));                        
                        
                        uvs.emplace_back(faceUvs.x);
                        uvs.emplace_back(1.f - faceUvs.y);
                    
                        uvs.emplace_back(faceUvs.x + faceUvs.z);
                        uvs.emplace_back(1.f - faceUvs.y);
                    
                        uvs.emplace_back(faceUvs.x + faceUvs.z);
                        uvs.emplace_back(1.f - (faceUvs.y + faceUvs.w));                        
                    
                        uvs.emplace_back(faceUvs.x);
                        uvs.emplace_back(1.f - (faceUvs.y + faceUvs.w));                        
                                            
                        uvs.emplace_back(faceUvs.x + faceUvs.z);
                        uvs.emplace_back(1.f - faceUvs.y);
                    }

                    if(!blockHasNeighbour(world, chunk, position, 'D')){ // down face (-y)
                        hm::vec4f faceUvs = getUvCoordsForFace(getFaceForType(b->type, 'D'));
                        
                        vertices.emplace_back(0.f + position.x);
                        vertices.emplace_back(0.f + position.y);
                        vertices.emplace_back(1.f + position.z);

                        vertices.emplace_back(0.f + position.x);
                        vertices.emplace_back(0.f + position.y);
                        vertices.emplace_back(0.f + position.z);

                        vertices.emplace_back(1.f + position.x);
                        vertices.emplace_back(0.f + position.y);
                        vertices.emplace_back(1.f + position.z);

                        vertices.emplace_back(0.f + position.x);
                        vertices.emplace_back(0.f + position.y);
                        vertices.emplace_back(0.f + position.z);

                        vertices.emplace_back(1.f + position.x);
                        vertices.emplace_back(0.f + position.y);
                        vertices.emplace_back(0.f + position.z);

                        vertices.emplace_back(1.f + position.x);
                        vertices.emplace_back(0.f + position.y);
                        vertices.emplace_back(1.f + position.z);

                        uvs.emplace_back(faceUvs.x);
                        uvs.emplace_back(1.f - faceUvs.y);

                        uvs.emplace_back(faceUvs.x);
                        uvs.emplace_back(1.f - (faceUvs.y + faceUvs.w));
                    
                        uvs.emplace_back(faceUvs.x + faceUvs.z);                        
                        uvs.emplace_back(1.f - faceUvs.y);
                        
                        uvs.emplace_back(faceUvs.x);
                        uvs.emplace_back(1.f - (faceUvs.y + faceUvs.w));
                    
                        uvs.emplace_back(faceUvs.x + faceUvs.z);
                        uvs.emplace_back(1.f - (faceUvs.y + faceUvs.w));
                    
                        uvs.emplace_back(faceUvs.x + faceUvs.z);
                        uvs.emplace_back(1.f - faceUvs.y);
                    }

                    if(!blockHasNeighbour(world, chunk, position, 'R')){ // left face (+x)
                        hm::vec4f faceUvs = getUvCoordsForFace(getFaceForType(b->type, 'R'));

                        vertices.emplace_back(1.f + position.x);
                        vertices.emplace_back(0.f + position.y);
                        vertices.emplace_back(0.f + position.z);

                        vertices.emplace_back(1.f + position.x);
                        vertices.emplace_back(1.f + position.y);
                        vertices.emplace_back(1.f + position.z);

                        vertices.emplace_back(1.f + position.x);
                        vertices.emplace_back(0.f + position.y);
                        vertices.emplace_back(1.f + position.z);
                        
                        vertices.emplace_back(1.f + position.x);
                        vertices.emplace_back(0.f + position.y);
                        vertices.emplace_back(0.f + position.z);

                        vertices.emplace_back(1.f + position.x);
                        vertices.emplace_back(1.f + position.y);
                        vertices.emplace_back(0.f + position.z);

                        vertices.emplace_back(1.f + position.x);
                        vertices.emplace_back(1.f + position.y);
                        vertices.emplace_back(1.f + position.z);
                        
                        uvs.emplace_back(faceUvs.x);
                        uvs.emplace_back(1.f - (faceUvs.y + faceUvs.w));

                        uvs.emplace_back(faceUvs.x + faceUvs.z);                        
                        uvs.emplace_back(1.f - faceUvs.y);
                    
                        uvs.emplace_back(faceUvs.x + faceUvs.z);                        
                        uvs.emplace_back(1.f - (faceUvs.y + faceUvs.w));

                        uvs.emplace_back(faceUvs.x);
                        uvs.emplace_back(1.f - (faceUvs.y + faceUvs.w));
                    
                        uvs.emplace_back(faceUvs.x);
                        uvs.emplace_back(1.f - faceUvs.y);
                    
                        uvs.emplace_back(faceUvs.x + faceUvs.z);                        
                        uvs.emplace_back(1.f - faceUvs.y);
                    }

                    if(!blockHasNeighbour(world, chunk, position, 'L')){ // right face (-x)
                        hm::vec4f faceUvs = getUvCoordsForFace(getFaceForType(b->type, 'L'));

                        vertices.emplace_back(0.f + position.x);
                        vertices.emplace_back(0.f + position.y);
                        vertices.emplace_back(0.f + position.z);

                        vertices.emplace_back(0.f + position.x);
                        vertices.emplace_back(0.f + position.y);
                        vertices.emplace_back(1.f + position.z);

                        vertices.emplace_back(0.f + position.x);
                        vertices.emplace_back(1.f + position.y);
                        vertices.emplace_back(1.f + position.z);

                        vertices.emplace_back(0.f + position.x);
                        vertices.emplace_back(1.f + position.y);
                        vertices.emplace_back(0.f + position.z);

                        vertices.emplace_back(0.f + position.x);
                        vertices.emplace_back(0.f + position.y);
                        vertices.emplace_back(0.f + position.z);

                        vertices.emplace_back(0.f + position.x);
                        vertices.emplace_back(1.f + position.y);
                        vertices.emplace_back(1.f + position.z);

                        uvs.emplace_back(faceUvs.x);
                        uvs.emplace_back(1.f - (faceUvs.y + faceUvs.w));                        

                        uvs.emplace_back(faceUvs.x + faceUvs.z);
                        uvs.emplace_back(1.f - (faceUvs.y + faceUvs.w));                        

                        uvs.emplace_back(faceUvs.x + faceUvs.z);
                        uvs.emplace_back(1.f - faceUvs.y);

                        uvs.emplace_back(faceUvs.x);
                        uvs.emplace_back(1.f - faceUvs.y);
                        
                        uvs.emplace_back(faceUvs.x);
                        uvs.emplace_back(1.f - (faceUvs.y + faceUvs.w));                        
                                            
                        uvs.emplace_back(faceUvs.x + faceUvs.z);
                        uvs.emplace_back(1.f - faceUvs.y);
                    }
                }
            }
        }
    }

    heVaoDestroy(&chunk->vao);
    heVaoCreate(&chunk->vao);
    heVaoBind(&chunk->vao);
    heVaoAddData(&chunk->vao, vertices, 3);
    heVaoAddData(&chunk->vao, uvs, 2);
    heVaoUnbind(&chunk->vao);
};

void renderChunk(HeRenderEngine* engine, World* world, Chunk* chunk) {
    heShaderLoadUniform(&world->blockShader, "u_transMat", hm::createTransformationMatrix(hm::vec3f(chunk->position * 16), hm::vec3f(), hm::vec3f(1.f)));
    heVaoBind(&chunk->vao);
    heVaoRender(&chunk->vao);
};

void createWorld(World* world) {
    heShaderCreateProgram(&world->blockShader, "res/shaders/block.glsl");
    heTextureLoadFromImageFile(&world->texturePack, "res/textures/defaultPack.png", true);
    heTextureFilterLinear(&world->texturePack);

    heWin32TimerStart();
    
    for(int32_t x = -10; x <= 10; ++x) {
        for(int32_t z = -10; z <= 10; ++z) {
            Chunk* c = &world->chunks.emplace_back();
            c->position = hm::vec3i(x, 0, z);
            generateChunk(world, c);
            buildChunk(world, c);
        }
    }
    
    heWin32TimerPrint("World creation");    
};

void renderWorld(HeRenderEngine* engine, World* world) {
    heCullEnable(true);
    heDepthEnable(true);
    
    heD3CameraClampRotation(&world->camera);
    world->camera.projectionMatrix = hm::createPerspectiveProjectionMatrix(world->camera.frustum.viewInfo.fov, engine->window->windowInfo.aspectRatio, world->camera.frustum.viewInfo.nearPlane, world->camera.frustum.viewInfo.farPlane);
    world->camera.viewMatrix = hm::createViewMatrix(world->camera.position, world->camera.rotation);

    heShaderBind(&world->blockShader);
    heShaderLoadUniform(&world->blockShader, "u_projMat", world->camera.projectionMatrix);
    heShaderLoadUniform(&world->blockShader, "u_viewMat", world->camera.viewMatrix);

    heTextureBind(&world->texturePack, heShaderGetSamplerLocation(&world->blockShader, "u_texture", 0));
    
    for(Chunk& chunks : world->chunks)
        renderChunk(engine, world, &chunks);
};
