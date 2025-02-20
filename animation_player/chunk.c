#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

enum
{
    POS_OFFSET = 0,
    POS_SIZE,
};

typedef union
{
    uint32_t pos[2];
} chunk_size_t;

typedef struct
{
    size_t size;
    uint8_t data[0];
} chunk_t;

chunk_t *chunk_load(const char *filename, int chunk_index)
{
    int total_chunk;
    FILE *fp = NULL;
    chunk_t *res = NULL;
    chunk_size_t *size_info = NULL;

    if(!filename || chunk_index < 0)
    {
        return NULL;
    }

    fp = fopen(filename, "rb");
    if(!fp) return NULL;

    total_chunk = fgetc(fp);
    if(total_chunk <= 0 || chunk_index >= total_chunk)
    {
        goto fail;
    }

    size_info = (chunk_size_t*)malloc(total_chunk * sizeof(chunk_size_t));
    if(!size_info || fread(size_info, sizeof(chunk_size_t), total_chunk, fp) != total_chunk)
    {
        goto fail;
    }

    if(fseek(fp, size_info[chunk_index].pos[POS_OFFSET], SEEK_CUR)) goto fail;
    if(!(res = (chunk_t*)malloc(size_info[chunk_index].pos[POS_SIZE] + sizeof(chunk_t)))) goto fail;
    res->size = size_info[chunk_index].pos[POS_SIZE];
    if(!fread(res->data, res->size, 1, fp)) goto fail;

    fclose(fp);
    free(size_info);
    return res;
fail:
    if(fp) fclose(fp);
    if(res) free(res);
    if(size_info) free(size_info);
    return NULL;
}

int chunk_save(const char *filename, chunk_t *chunk)
{
    if(!filename || !chunk) return -1;

    FILE *fp = fopen(filename, "wb");
    if(!fp) return -1;

    if(!fwrite(chunk->data, chunk->size, 1, fp)) goto fail;

    fflush(fp);
    fclose(fp);
    return 0;
fail:
    fclose(fp);
    return -1;
}

#define CHUNK_TEST 0
#if CHUNK_TEST

int main(int argc, const char **argv)
{
    if(argc < 4) return -1;

    const char *chunk_filename = argv[1];
    int index = atoi(argv[2]);
    const char *save_name = argv[3];
    chunk_t *chunk = NULL;

    if(!(chunk = chunk_load(chunk_filename, index)))
    {
        fprintf(stderr, "Failed to load \"%s\"\n", chunk_filename);
        return -1;
    }

    if(chunk_save(save_name, chunk))
    {
        fprintf(stderr, "Failed to save \"%s\"\n", save_name);
        return -1;
    }

    free(chunk);
    return 0;
}

#endif
