#include "minecraft.h"
#include "heWindow.h"
#include "generator.h"
#include "heWin32Layer.h"
#include "heCore.h"


Block* getBlockAt(Chunk* chunk, BlockPosition const& position) {
    return &chunk->layers[position.y].blocks[position.x * CHUNK_SIZE + position.z];
};

b8 blockHasNeighbour(World* world, Chunk* chunk, BlockPosition const& position, char const direction) {
    if(chunk->blockCount == 0)
        return false;

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
            hasNeighbour = blockHasNeighbour(world, getChunkAt(world, chunk->position + ChunkPosition(1, 0)), BlockPosition(-1, position.y, position.z), 'R');
        else
            hasNeighbour = getBlockAt(chunk, position + hm::vec3i(1, 0, 0))->type != BLOCK_TYPE_AIR;
        break;
    }

    case 'L': {
        if(position.x == 0)
            hasNeighbour = blockHasNeighbour(world, getChunkAt(world, chunk->position + ChunkPosition(-1, 0)), BlockPosition(CHUNK_SIZE, position.y, position.z), 'L');
        else
            hasNeighbour = getBlockAt(chunk, position - hm::vec3i(1, 0, 0))->type != BLOCK_TYPE_AIR;
        break;
    }

    case 'B': {
        if(position.z == CHUNK_SIZE - 1)
            hasNeighbour = blockHasNeighbour(world, getChunkAt(world, chunk->position + ChunkPosition(0, 1)), BlockPosition(position.x, position.y, -1), 'B');
        else
            hasNeighbour = getBlockAt(chunk, position + hm::vec3i(0, 0, 1))->type != BLOCK_TYPE_AIR;
        break;
    }

    case 'F': {
        if(position.z == 0)
            hasNeighbour = blockHasNeighbour(world, getChunkAt(world, chunk->position + ChunkPosition(0, -1)), BlockPosition(position.x, position.y, CHUNK_SIZE), 'F');
        else
            hasNeighbour = getBlockAt(chunk, position - hm::vec3i(0, 0, 1))->type != BLOCK_TYPE_AIR;
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
    if(chunk->blockCount == 0)
        return;

    // blocks a 6 faces a 6 positions a 3 floats
    chunk->meshInfo.vertices.reserve(chunk->blockCount * 6 * 6 * 3);
    chunk->meshInfo.uvs.reserve(chunk->blockCount * 6 * 6 * 2);
    
    for(uint32_t z = 0; z < CHUNK_SIZE; ++z) {
        for(uint32_t x = 0; x < CHUNK_SIZE; ++x) {
            for(uint32_t y = 0; y < CHUNK_HEIGHT; ++y) {
                hm::vec3i position(x, y, z);
                Block* b = getBlockAt(chunk, position);
                if(b->type != BLOCK_TYPE_AIR) {
                    if(!blockHasNeighbour(world, chunk, position, 'B')) { // back face (+z)
                        hm::vec4f chunkUvs = getUvCoordsForFace(getFaceForType(b->type, 'B'));

                        chunk->meshInfo.vertices.emplace_back(0.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(1.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(0.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(0.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(1.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(1.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.z);

                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x + chunkUvs.z);
                        chunk->meshInfo.uvs.emplace_back(1.f - (chunkUvs.y + chunkUvs.w));
                    
                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x);                        
                        chunk->meshInfo.uvs.emplace_back(1.f - chunkUvs.y);

                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x + chunkUvs.z);
                        chunk->meshInfo.uvs.emplace_back(1.f - chunkUvs.y);
                    
                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x + chunkUvs.z);
                        chunk->meshInfo.uvs.emplace_back(1.f - (chunkUvs.y + chunkUvs.w));
                    
                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x);
                        chunk->meshInfo.uvs.emplace_back(1.f - (chunkUvs.y + chunkUvs.w));
                    
                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x);                        
                        chunk->meshInfo.uvs.emplace_back(1.f - chunkUvs.y);
                    }
                    
                    if(!blockHasNeighbour(world, chunk, position, 'F')){ // front face (-z)
                        hm::vec4f chunkUvs = getUvCoordsForFace(getFaceForType(b->type, 'F'));

                        chunk->meshInfo.vertices.emplace_back(0.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(0.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(1.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(1.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(0.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(1.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.z);

                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x);
                        chunk->meshInfo.uvs.emplace_back(1.f - (chunkUvs.y + chunkUvs.w));
                        
                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x);
                        chunk->meshInfo.uvs.emplace_back(1.f - chunkUvs.y);
                    
                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x + chunkUvs.z);
                        chunk->meshInfo.uvs.emplace_back(1.f - chunkUvs.y);
                    
                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x + chunkUvs.z);
                        chunk->meshInfo.uvs.emplace_back(1.f - (chunkUvs.y + chunkUvs.w));
                    
                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x);
                        chunk->meshInfo.uvs.emplace_back(1.f - (chunkUvs.y + chunkUvs.w));
                                            
                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x + chunkUvs.z);
                        chunk->meshInfo.uvs.emplace_back(1.f - chunkUvs.y);
                    }

                    if(!blockHasNeighbour(world, chunk, position, 'T')){ // top face (+y)
                        hm::vec4f chunkUvs = getUvCoordsForFace(getFaceForType(b->type, 'T'));

                        chunk->meshInfo.vertices.emplace_back(0.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(0.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(1.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(1.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(0.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(1.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.z);

                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x);
                        chunk->meshInfo.uvs.emplace_back(1.f - (chunkUvs.y + chunkUvs.w));
                        
                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x);
                        chunk->meshInfo.uvs.emplace_back(1.f - chunkUvs.y);
                    
                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x + chunkUvs.z);
                        chunk->meshInfo.uvs.emplace_back(1.f - chunkUvs.y);
                    
                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x + chunkUvs.z);
                        chunk->meshInfo.uvs.emplace_back(1.f - (chunkUvs.y + chunkUvs.w));
                    
                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x);
                        chunk->meshInfo.uvs.emplace_back(1.f - (chunkUvs.y + chunkUvs.w));
                                            
                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x + chunkUvs.z);
                        chunk->meshInfo.uvs.emplace_back(1.f - chunkUvs.y);
                    }

                    if(!blockHasNeighbour(world, chunk, position, 'D')){ // down face (-y)
                        hm::vec4f chunkUvs = getUvCoordsForFace(getFaceForType(b->type, 'D'));
                        
                        chunk->meshInfo.vertices.emplace_back(0.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(0.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(1.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(0.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(1.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(1.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.z);

                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x);
                        chunk->meshInfo.uvs.emplace_back(1.f - chunkUvs.y);

                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x);
                        chunk->meshInfo.uvs.emplace_back(1.f - (chunkUvs.y + chunkUvs.w));
                    
                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x + chunkUvs.z);
                        chunk->meshInfo.uvs.emplace_back(1.f - chunkUvs.y);
                        
                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x);
                        chunk->meshInfo.uvs.emplace_back(1.f - (chunkUvs.y + chunkUvs.w));
                    
                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x + chunkUvs.z);
                        chunk->meshInfo.uvs.emplace_back(1.f - (chunkUvs.y + chunkUvs.w));
                    
                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x + chunkUvs.z);
                        chunk->meshInfo.uvs.emplace_back(1.f - chunkUvs.y);
                    }

                    if(!blockHasNeighbour(world, chunk, position, 'R')){ // left face (+x)
                        hm::vec4f chunkUvs = getUvCoordsForFace(getFaceForType(b->type, 'R'));

                        chunk->meshInfo.vertices.emplace_back(1.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(1.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(1.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.z);
                        
                        chunk->meshInfo.vertices.emplace_back(1.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(1.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(1.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.z);
                        
                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x);
                        chunk->meshInfo.uvs.emplace_back(1.f - (chunkUvs.y + chunkUvs.w));

                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x + chunkUvs.z);
                        chunk->meshInfo.uvs.emplace_back(1.f - chunkUvs.y);
                    
                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x + chunkUvs.z);
                        chunk->meshInfo.uvs.emplace_back(1.f - (chunkUvs.y + chunkUvs.w));

                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x);
                        chunk->meshInfo.uvs.emplace_back(1.f - (chunkUvs.y + chunkUvs.w));
                    
                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x);
                        chunk->meshInfo.uvs.emplace_back(1.f - chunkUvs.y);
                    
                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x + chunkUvs.z);
                        chunk->meshInfo.uvs.emplace_back(1.f - chunkUvs.y);
                    }

                    if(!blockHasNeighbour(world, chunk, position, 'L')){ // right face (-x)
                        hm::vec4f chunkUvs = getUvCoordsForFace(getFaceForType(b->type, 'L'));

                        chunk->meshInfo.vertices.emplace_back(0.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(0.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(0.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(0.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(0.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(0.f + position.z);

                        chunk->meshInfo.vertices.emplace_back(0.f + position.x);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.y);
                        chunk->meshInfo.vertices.emplace_back(1.f + position.z);

                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x);
                        chunk->meshInfo.uvs.emplace_back(1.f - (chunkUvs.y + chunkUvs.w));

                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x + chunkUvs.z);
                        chunk->meshInfo.uvs.emplace_back(1.f - (chunkUvs.y + chunkUvs.w));

                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x + chunkUvs.z);
                        chunk->meshInfo.uvs.emplace_back(1.f - chunkUvs.y);

                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x);
                        chunk->meshInfo.uvs.emplace_back(1.f - chunkUvs.y);
                        
                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x);
                        chunk->meshInfo.uvs.emplace_back(1.f - (chunkUvs.y + chunkUvs.w));
                                            
                        chunk->meshInfo.uvs.emplace_back(chunkUvs.x + chunkUvs.z);
                        chunk->meshInfo.uvs.emplace_back(1.f - chunkUvs.y);
                    }
                }
            }
        }
    }


#if USE_PHYSICS == 1
    HePhysicsShapeInfo info;
    info.type = HE_PHYSICS_SHAPE_CONCAVE_MESH;
    info.mass = 0.f;
    info.mesh = chunk->meshInfo.vertices.data();
    info.floatCount = chunk->meshInfo.vertices.size();
    hePhysicsComponentCreate(&chunk->physics, info);
    hePhysicsComponentSetPosition(&chunk->physics, hm::vec3f(chunk->position.x * 16, 0, chunk->position.y * 16));
#endif
    chunk->state = CHUNK_STATE_BUILT;
};

void generateChunkNeighbours(World* world, Chunk* chunk) {
    /*
    Chunk* c0 = getChunkAt(world, chunk->position + ChunkPosition( 1,  0));
    if(c0->blockCount == 0) {
        generateChunk(world, c0);
        c0->position = chunk->position + ChunkPosition( 1,  0);
    }
    
    Chunk* c1 = getChunkAt(world, chunk->position + ChunkPosition(-1,  0));
    if(c1->blockCount == 0) {
        generateChunk(world, c1);
        c1->position = chunk->position + ChunkPosition(-1,  0);
    }
        
    Chunk* c2 = getChunkAt(world, chunk->position + ChunkPosition( 0,  1));
    if(c2->blockCount == 0) {
        generateChunk(world, c2);
        c2->position = chunk->position + ChunkPosition( 0,  1);
    }
        
    Chunk* c3 = getChunkAt(world, chunk->position + ChunkPosition( 0, -1));
    if(c3->blockCount == 0) {
        generateChunk(world, c3);
        c3->position = chunk->position + ChunkPosition( 0,  -1);
    }
    */

    for(int8_t x = -1; x <= 1; ++x) {
        for(int8_t z = -1; z <= 1; ++z) {
            if(x != 0 || z != 0) {
                Chunk* c0 = getChunkAt(world, chunk->position + ChunkPosition(x, z));
                if(c0->blockCount == 0) {
                    generateChunk(world, c0);
                    c0->position = chunk->position + ChunkPosition(x, z);
                }
            }
        }
    }
};

void loadChunk(World* world, Chunk* chunk) {
    heVaoDestroy(&chunk->vao);
    heVaoCreate(&chunk->vao);
    heVaoBind(&chunk->vao);
    heVaoAddData(&chunk->vao, chunk->meshInfo.vertices, 3);
    heVaoAddData(&chunk->vao, chunk->meshInfo.uvs, 2);
    heVaoUnbind(&chunk->vao);
    chunk->meshInfo.vertices.clear();
    chunk->meshInfo.uvs.clear();
    std::vector<float>().swap(chunk->meshInfo.vertices);
    std::vector<float>().swap(chunk->meshInfo.uvs);
    hePhysicsLevelAddComponent(&world->physics, &chunk->physics);
    chunk->state = CHUNK_STATE_LOADED;
};

void unloadChunk(World* world, Chunk* chunk) {
    heVaoDestroy(&chunk->vao);
    hePhysicsLevelRemoveComponent(&world->physics, &chunk->physics);
    hePhysicsComponentDestroy(&chunk->physics);
    chunk->state = CHUNK_STATE_UNLOADED;
    world->chunks.erase(chunk->position);
    HE_LOG("Unloaded chunk");
};

void renderChunk(HeRenderEngine* engine, World* world, Chunk* chunk) {
    heShaderLoadUniform(&world->blockShader, "u_transMat", hm::createTransformationMatrix(hm::vec3f(chunk->position.x * 16, 0, chunk->position.y * 16), hm::vec3f(), hm::vec3f(1.f)));
    heVaoBind(&chunk->vao);
    heVaoRender(&chunk->vao);
};


void createWorld(World* world) {
    heShaderCreateProgram(&world->blockShader, "res/shaders/block.glsl");
    heTextureLoadFromImageFile(&world->texturePack, "res/textures/defaultPack.png", true);
    heTextureFilterLinear(&world->texturePack);
    
#if USE_PHYSICS == 1
    hePhysicsLevelCreate(&world->physics, HePhysicsLevelInfo(hm::vec3f(0.f, -20.f, 0.f)));

	HePhysicsShapeInfo actorShape;
	actorShape.type = HE_PHYSICS_SHAPE_CAPSULE;
	actorShape.capsule = hm::vec2f(0.75f, 2.0f);
	
	HePhysicsActorInfo actorInfo;
	actorInfo.eyeOffset = 1.6f;
	actorInfo.jumpHeight = 7.f;
	
	hePhysicsActorCreate(&world->actor, actorShape, actorInfo);
    hePhysicsLevelSetActor(&world->physics, &world->actor);
#endif
    
    heWin32TimerStart();
    
    Chunk* c = getChunkAt(world, ChunkPosition(0, 0));
    c->position = ChunkPosition(0, 0);
    generateChunk(world, c);
    generateChunkNeighbours(world, c);    
    buildChunk(world, c);
    loadChunk(world, c);
    
    heWin32TimerPrint("World creation");    

#if USE_PHYSICS == 1
    Chunk* spawnChunk = getChunkAt(world, ChunkPosition(0, 0));    
    hePhysicsActorSetEyePosition(&app.world.actor, hm::vec3f(8.f, 20.f, 8.f));
#endif
};

void renderWorld(HeRenderEngine* engine, World* world) {
    heCullEnable(false);
    heDepthEnable(true);
    
    heD3CameraClampRotation(&world->camera);
    world->camera.projectionMatrix = hm::createPerspectiveProjectionMatrix(world->camera.frustum.viewInfo.fov, engine->window->windowInfo.aspectRatio, world->camera.frustum.viewInfo.nearPlane, world->camera.frustum.viewInfo.farPlane);
    world->camera.viewMatrix = hm::createViewMatrix(world->camera.position, world->camera.rotation);

    heShaderBind(&world->blockShader);
    heShaderLoadUniform(&world->blockShader, "u_projMat", world->camera.projectionMatrix);
    heShaderLoadUniform(&world->blockShader, "u_viewMat", world->camera.viewMatrix);

    heTextureBind(&world->texturePack, heShaderGetSamplerLocation(&world->blockShader, "u_texture", 0));

    ChunkPosition playerChunk((int16_t) std::floor(world->camera.position.x / CHUNK_SIZE), (int16_t) std::floor(world->camera.position.z / CHUNK_SIZE));
    

    std::vector<Chunk*> chunksToUnload;
    for(auto& all : world->chunks) {
        Chunk& chunks = all.second;

        if(chunks.state == CHUNK_STATE_GENERATED) {
            if(std::abs(chunks.position.x - playerChunk.x) <= world->viewDistance && std::abs(chunks.position.y - playerChunk.y) <= world->viewDistance) {
                chunks.requestGeneration = true;
            }
        }                

        if(chunks.state == CHUNK_STATE_BUILT)
            loadChunk(world, &chunks);

        if(chunks.state == CHUNK_STATE_LOADED) {
            renderChunk(engine, world, &chunks);
            if(std::abs(chunks.position.x - playerChunk.x) > world->viewDistance || std::abs(chunks.position.y - playerChunk.y) > world->viewDistance) {
                chunksToUnload.emplace_back(&chunks);
            }
        }
    }

    for(Chunk* all : chunksToUnload) {
        unloadChunk(world, all);
    }

    hePhysicsLevelDebugDraw(&world->physics);
};

void updateWorld(World* world) {
#if USE_PHYSICS == 1
    hePhysicsLevelUpdate(&app.world.physics, (float) app.window.frameTime);
#endif

    if(!app.freeCamera)
        world->camera.position = hePhysicsActorGetEyePosition(&world->actor);
};

Chunk* getChunkAt(World* world, ChunkPosition const& position) {
    return &world->chunks[position];
};

Chunk* getChunkAtIfExists(World* world, ChunkPosition const& position) {
    auto it = world->chunks.find(position);
    if(it == world->chunks.end())
        return nullptr;
    else
        return &it->second;
};


void worldCreationThread(World* world) {
    while(app.state == GAME_STATE_INGAME || app.state == GAME_STATE_PAUSED) {
        for(auto& all : world->chunks) {
            Chunk& chunks = all.second;
            if(chunks.requestGeneration) {
                generateChunkNeighbours(world, &chunks);
                buildChunk(world, &chunks);
                chunks.requestGeneration = false;
            }
        }

        heThreadSleep(1000);
    }
};
