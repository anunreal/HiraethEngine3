#ifndef WORLD_H
#define WORLD_H

#include "hm/hm.hpp"
#include "heD3.h"
#include "heRenderer.h"
#include <list>

#define CHUNK_SIZE 16
#define CHUNK_HEIGHT 256

typedef hm::vec<3, uint8_t> BlockPosition;

enum BlockType : uint8_t {
    BLOCK_TYPE_AIR,
    BLOCK_TYPE_GRASS,
    BLOCK_TYPE_DIRT,
    BLOCK_TYPE_STONE,
};

enum ChunkState : uint8_t {

};

struct Block {
    BlockType type = BLOCK_TYPE_AIR;
    //BlockPosition position; // relative in chunk, range [0:15] in x,z direction, 256 in y direction
};

struct BlockLayer {
    Block blocks[CHUNK_SIZE * CHUNK_SIZE];
};

struct Chunk {
    HeVao vao;
    BlockLayer layers[CHUNK_HEIGHT];
    uint32_t blockCount = 0;
    hm::vec3i position; // the chunk position, in chunk space (not world space)

    ChunkState state;
};

struct World {
    HeShaderProgram blockShader;
    HeTexture texturePack;

    HeD3Camera camera;
    std::list<HeD3LightSource> lights;
    std::list<Chunk> chunks;
};

// returns the block (can be air) at given chunk space position
extern Block* getBlockAt(Chunk* chunk, BlockPosition const& position);
extern void buildChunk(World* world, Chunk* chunk);
extern void renderChunk(HeRenderEngine* engine, World* world, Chunk* chunk);

extern void createWorld(World* world);
extern void renderWorld(HeRenderEngine* engine, World* world);

#endif
