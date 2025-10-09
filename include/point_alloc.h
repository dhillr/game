#ifndef POINT_ALLOC_FILE
#define POINT_ALLOC_FILE

#define BLOCK_SIZE 1024

typedef struct {
    float x, y;
    uint32_t __block_index;
} vec2;

typedef struct {
    float x, y, z;
    uint32_t __block_index;
} vec3;

typedef struct {
    vec2* block;
    uint32_t index, block_size;
} point_alloc;

point_alloc POINT_ALLOC_DEFAULT;

uint32_t alloc_point(vec2 p) {
    if (POINT_ALLOC_DEFAULT.index >= POINT_ALLOC_DEFAULT.block_size) {
        POINT_ALLOC_DEFAULT.block = realloc(POINT_ALLOC_DEFAULT.block, POINT_ALLOC_DEFAULT.block_size + BLOCK_SIZE);
        POINT_ALLOC_DEFAULT.block_size += BLOCK_SIZE;
    }

    POINT_ALLOC_DEFAULT.block[POINT_ALLOC_DEFAULT.index] = p;
    POINT_ALLOC_DEFAULT.index++;

    return POINT_ALLOC_DEFAULT.index - 1;
}

void remove_points(uint32_t from, uint32_t to) {
    int first_block_size = from;
    int last_block_size = POINT_ALLOC_DEFAULT.block_size - to - 1;

    vec2* first_block = malloc(first_block_size * sizeof(vec2));
    vec2* last_block = malloc(last_block_size * sizeof(vec2));

    memcpy(first_block, POINT_ALLOC_DEFAULT.block, first_block_size * sizeof(vec2));
    memcpy(last_block, POINT_ALLOC_DEFAULT.block + to + 1, last_block_size * sizeof(vec2));

    // free(POINT_ALLOC_DEFAULT.block);
    // POINT_ALLOC_DEFAULT.block = calloc(POINT_ALLOC_DEFAULT.block_size, sizeof(vec2));
    memset(POINT_ALLOC_DEFAULT.block, 0, POINT_ALLOC_DEFAULT.block_size * sizeof(vec2));

    memcpy(POINT_ALLOC_DEFAULT.block, first_block, first_block_size * sizeof(vec2));
    memcpy(POINT_ALLOC_DEFAULT.block + from, last_block, last_block_size * sizeof(vec2));

    POINT_ALLOC_DEFAULT.index--;

    free(first_block);
    free(last_block);
}

void remove_point(uint32_t index) {
    remove_points(index, index);
}

#endif