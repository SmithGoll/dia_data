#include <stdint.h>

#ifndef _CHUNK_H_
#define _CHUNK_H_

typedef struct
{
    size_t size;
    uint8_t data[0];
} chunk_t;

chunk_t *chunk_load(const char *filename, int chunk_index);
int chunk_save(const char *filename, chunk_t *chunk);

#endif
