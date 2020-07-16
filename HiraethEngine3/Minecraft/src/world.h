#ifndef WORLD_H
#define WORLD_H

#include "hm/hm.hpp"
#include "heD3.h"
#include "heRenderer.h"
#include <list>

#define CHUNK_SIZE 16
#define CHUNK_HEIGHT 255

typedef hm::vec<3, int16_t> BlockPosition;
typedef hm::vec<2, int16_t> ChunkPosition;

struct ChunkPositionHash {
    size_t operator()(ChunkPosition const& k) const {
        return std::hash<int16_t>()(k.x) ^ std::hash<int16_t>()(k.y);
    }

    b8 operator()(ChunkPosition const& a, ChunkPosition const& b) const {
        return a.x == b.x && a.y == b.y;
    }
};

enum BlockType : uint8_t {
    BLOCK_TYPE_AIR,
    BLOCK_TYPE_GRASS,
    BLOCK_TYPE_DIRT,
    BLOCK_TYPE_STONE,
};

enum ChunkState : uint8_t {
    CHUNK_STATE_UNLOADED,
    CHUNK_STATE_GENERATED,
    CHUNK_STATE_BUILT,
    CHUNK_STATE_LOADED
};

struct Block {
    BlockType type = BLOCK_TYPE_AIR;
};

struct BlockLayer {
    Block blocks[CHUNK_SIZE * CHUNK_SIZE];
};

struct ChunkMeshInfo {
    std::vector<float> vertices;
    std::vector<float> uvs;
};

struct Chunk {
    HeVao vao;
    BlockLayer layers[CHUNK_HEIGHT];
    uint32_t blockCount = 0;
    ChunkPosition position; // the chunk position, in chunk space (not world space)
    HePhysicsComponent physics;
    
    ChunkState state = CHUNK_STATE_UNLOADED;
    ChunkMeshInfo meshInfo;

    b8 requestGeneration = false;
};

struct World {
    HeShaderProgram blockShader;
    HeTexture texturePack;

    HePhysicsLevel physics;
    HePhysicsActor actor;
    HeD3Camera camera;
    std::list<HeD3LightSource> lights;
    std::unordered_map<ChunkPosition, Chunk, ChunkPositionHash> chunks;

    int32_t viewDistance = 3;
};

// returns the block (can be air) at given chunk space position
extern Block* getBlockAt(Chunk* chunk, BlockPosition const& position);
// creates the chunk mesh info for that chunk
extern void buildChunk(World* world, Chunk* chunk);
// loads the mesh info into the vao of that chunk (must be main thread)
extern void loadChunk(World* world, Chunk* chunk);
extern void unloadChunk(World* world, Chunk* chunk);
extern void renderChunk(HeRenderEngine* engine, World* world, Chunk* chunk);

extern void createWorld(World* world);
extern void renderWorld(HeRenderEngine* engine, World* world);
extern void updateWorld(World* world);
extern inline Chunk* getChunkAt(World* world, ChunkPosition const& position);
extern inline Chunk* getChunkAtIfExists(World* world, ChunkPosition const& position);

extern void worldCreationThread(World* world);

#endif
