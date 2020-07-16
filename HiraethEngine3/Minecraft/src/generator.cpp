#include "generator.h"
#include "heCore.h"

void generateChunk(World* world, Chunk* chunk) {
    chunk->blockCount = 0;
    memset(chunk->layers, 0, sizeof(chunk->layers));
    
    for(uint8_t y = 0; y < CHUNK_SIZE; ++y) {
        for(uint8_t x = 0; x < CHUNK_SIZE; ++x) {
            for(uint8_t z = 0; z < CHUNK_SIZE; ++z) {
                BlockType type;

                if(y < 6)
                    type = BLOCK_TYPE_STONE;
                else if(y < 9)
                    type = BLOCK_TYPE_DIRT;
                else
                    type = BLOCK_TYPE_GRASS;

                chunk->layers[y].blocks[x * CHUNK_SIZE + z].type = type;
                chunk->blockCount++;
            }
        }
    }

    chunk->state = CHUNK_STATE_GENERATED;
};
