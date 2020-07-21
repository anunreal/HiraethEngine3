#include "generator.h"
#include "heCore.h"
#include "heWin32Layer.h"
#include "hm/hm.hpp"

HePerlinNoise noise;

void generatorCreate() {
    hePerlinNoiseCreate(&noise);
};

void generateChunk(World* world, Chunk* chunk) {
    chunk->blockCount = 0;
    memset(chunk->layers, 0, sizeof(chunk->layers));
    
    hm::vec3d chunkOffset(chunk->position.x * 16., 0, chunk->position.y * 16.);

    for(uint8_t x = 0; x < CHUNK_SIZE; ++x) {
        for(uint8_t z = 0; z < CHUNK_SIZE; ++z) {
            int16_t height = (int16_t) (hePerlinNoise3D(&noise, (hm::vec3d(x, 0, z) + chunkOffset) / 32.) * 20.);
            height = hm::clamp(height, (int16_t) 0, (int16_t) 255);
            
            for(uint16_t y = 0; y <= height; ++y) {
                BlockType type;

                if(y == height)
                    type = BLOCK_TYPE_GRASS;
                else if(height - y <= 3)
                    type = BLOCK_TYPE_DIRT;
                else
                    type = BLOCK_TYPE_STONE;

                chunk->layers[y].blocks[x * CHUNK_SIZE + z].type = type;
                chunk->blockCount++;
            }
        }
    }

    chunk->state = CHUNK_STATE_GENERATED;
};
