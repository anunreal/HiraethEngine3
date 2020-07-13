#ifndef WORLD_H
#define WORLD_H

#include "hm/hm.hpp"
#include "heD3.h"
#include <list>

#define CHUNK_SIZE 16

enum BlockType {
    BLOCK_TYPE_GRASS,
};

struct Block {
    BlockType type;
    hm::vec3i position; // relative in chunk, range [0:15] in every direction
};

struct BlockLayer {
    Block blocks[CHUNK_SIZE * CHUNK_SIZE];
};

struct Chunk {
    HeVao vao;
    BlockLayer layers[CHUNK_SIZE];
    hm::vec3i position; // the chunk position, in chunk space (not world space)
};

struct World {
    HeShaderProgram blockShader;
    std::list<HeD3LightSource> lights;
    std::list<Chunk> chunks;
};

extern void renderChunk(World* world, Chunk* chunk);

#endif
