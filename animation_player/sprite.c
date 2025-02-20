#include <stdint.h>
#include <stdlib.h>
#include "chunk.h"
#include "graphic.h"
#include "palette.h"
#include "texture.h"

typedef struct
{
    int width;
    int height;
} dims_t;   // dimensions

typedef struct
{
    int count;
    int offset;
} tile_pos_info_t;

typedef struct
{
    int index;
    int x;
    int y;
    int transform;
} tile_pos_t;

typedef struct
{
    dims_t *dims;
    int cur_palette;
    tex_t **textures;
    size_t total_tiles;
    size_t total_palettes;
    size_t total_tile_pos;
    size_t total_tile_pos_info;
    tile_pos_t *tile_pos;
    tile_pos_info_t *tile_pos_info;
} sprite_t;

static inline void bytes_copy(const uint8_t *src, size_t *offset, void *dest, size_t size)
{
    uint8_t *dst = (uint8_t*)dest;
    for(int i = 0; i < size; i++)
    {
        dst[i] = src[i + (*offset)];
    }
    *offset += size;
    return;
}

void sprite_free(sprite_t *sprite)
{
    if(!sprite) return;

    dims_t *dims = sprite->dims;
    tex_t **textures = sprite->textures;
    tile_pos_t *tile_pos = sprite->tile_pos;
    tile_pos_info_t *tile_pos_info = sprite->tile_pos_info;

    if(dims) free(dims);
    if(tile_pos) free(tile_pos);
    if(tile_pos_info) free(tile_pos_info);
    if(textures)
    {
        for(int i = 0; i < sprite->total_palettes * sprite->total_tiles; i++)
        {
            if(!textures[i]) break;
            graphic_destroy_texture(textures[i]);
        }
        free(textures);
    }
    free(sprite);
    return;
}

sprite_t *sprite_load(chunk_t *sprite_chunk, int scale, graphic_t *graphic)
{
    dims_t *dims = NULL;
    tex_t **textures = NULL;
    tile_pos_t *tile_pos = NULL;
    container_t *palettes = NULL;
    tile_pos_info_t *tile_pos_info = NULL;

    uint16_t tmp;

    sprite_t *res = NULL;

    if(!sprite_chunk) return NULL;
    if(scale <= 1) scale = 1;

    res = (sprite_t*)calloc(1, sizeof(sprite_t));
    if(!res) return NULL;

    // TODO: Add sprite file magic check
    size_t cur_pos = 6;
    uint8_t *sprite_data = sprite_chunk->data;

    // tiles_count
    bytes_copy(sprite_data, &cur_pos, &res->total_tiles, 2);
    if(res->total_tiles <= 0) goto fail;

    // dimensions
    dims = (dims_t*)malloc(sizeof(dims_t) * res->total_tiles);
    if(!dims) goto fail;

    res->dims = dims;
    for(int i = 0; i < res->total_tiles; i++)
    {
        dims[i].width = sprite_data[cur_pos++];
        dims[i].height = sprite_data[cur_pos++];
    }

    // tile_pos
    bytes_copy(sprite_data, &cur_pos, &tmp, 2);
    res->total_tile_pos = tmp;
    if(tmp) tile_pos = (tile_pos_t*)malloc(sizeof(tile_pos_t) * tmp);
    for(int i = 0; i < tmp; i++)
    {
        tile_pos[i].index = sprite_data[cur_pos++];
        tile_pos[i].x = (int8_t)sprite_data[cur_pos++] * scale;
        tile_pos[i].y = (int8_t)sprite_data[cur_pos++] * scale;
        tile_pos[i].transform = sprite_data[cur_pos++];
    }
    res->tile_pos = tile_pos;

    // tile_pos_info
    bytes_copy(sprite_data, &cur_pos, &tmp, 2);
    res->total_tile_pos_info = tmp;
    if(tmp) tile_pos_info = (tile_pos_info_t*)malloc(sizeof(tile_pos_info_t) * tmp);
    for(int i = 0; i < tmp; i++)
    {
        tile_pos_info[i].count = sprite_data[cur_pos++];
        cur_pos++;
        tile_pos_info[i].offset = 0;
        bytes_copy(sprite_data, &cur_pos, &(tile_pos_info[i].offset), 2);
    }
    res->tile_pos_info = tile_pos_info;

    // unkonwn data
    cur_pos += tmp * 4;

    // unkonwn data
    bytes_copy(sprite_data, &cur_pos, &tmp, 2);
    cur_pos += tmp * 5;

    // animation data (ignore)
    bytes_copy(sprite_data, &cur_pos, &tmp, 2);
    cur_pos += tmp * 4;

    // palette load
    uint16_t palette_type;
    uint8_t total_palette;
    uint8_t total_palette_pixel;
    bytes_copy(sprite_data, &cur_pos, &palette_type, 2);
    bytes_copy(sprite_data, &cur_pos, &total_palette, 1);
    bytes_copy(sprite_data, &cur_pos, &total_palette_pixel, 1);
    palettes = palette_load(sprite_data + cur_pos, palette_type, total_palette, total_palette_pixel);
    if(!palettes) goto fail;
    res->total_palettes = total_palette;
    cur_pos += total_palette * total_palette_pixel * (palette_type == 0x8888 ? 4 : 2);

    // texture decode type (ignore)
    bytes_copy(sprite_data, &cur_pos, &tmp, 2);

    // texture decode
    textures = (tex_t**)calloc(total_palette * res->total_tiles, sizeof(void*));
    if(!textures) goto fail;

    res->textures = textures;
    int cur_tex_pos = 0;
    for(int i = 0; i < total_palette; i++)
    {
        texture_t *texture_tmp;
        size_t cur_pos_tmp = cur_pos;
        palette_t *cur_palette = get_palette(palettes, i);
        if(!cur_palette) goto fail;
        for(int j = 0; j < res->total_tiles; j++)
        {
            int w = dims[j].width;
            int h = dims[j].height;

            // palette color index
            bytes_copy(sprite_data, &cur_pos_tmp, &tmp, 2);

            texture_tmp = texture_load(sprite_data + cur_pos_tmp, cur_palette, w, h, scale);
            textures[cur_tex_pos] = texture_create_graphic_texture(texture_tmp, graphic);
            if(!textures[cur_tex_pos++]) goto fail;
            texture_free(texture_tmp);

            cur_pos_tmp += tmp;
        }
    }
    res->cur_palette = 0;
    // Don't forget to free palettes
    palette_free(palettes);
    return res;

fail:
    if(res)
    {
        if(dims) free(dims);
        if(tile_pos) free(tile_pos);
        if(tile_pos_info) free(tile_pos_info);

        if(textures)
        {
            for(int i = 0; i < res->total_palettes * res->total_tiles; i++)
            {
                if(!textures[i]) break;
                graphic_destroy_texture(textures[i]);
            }
            free(textures);
        }
        palette_free(palettes);
        free(res);
    }
    return NULL;
}

void sprite_set_cur_palette(sprite_t *sprite, int index)
{
    if(!sprite || index < 0) return;

    size_t total_palettes = sprite->total_palettes;
    sprite->cur_palette = (total_palettes <= index) ? 0 : index;
    return;
}

size_t sprite_get_tile_count(sprite_t *sprite)
{
    if(!sprite) return 0;

    return sprite->total_tiles;
}

size_t sprite_get_tile_pos_info_count(sprite_t *sprite)
{
    if(!sprite) return 0;

    return sprite->total_tile_pos_info;
}

tex_t *sprite_get_tile(sprite_t *sprite, int index)
{
    if(!sprite) return NULL;
    if(index < 0) return NULL;

    size_t total_tiles = sprite->total_tiles;
    if(total_tiles <= index) return NULL;

    int cur_palette = sprite->cur_palette;
    tex_t **textures = sprite->textures;
    return textures[cur_palette * total_tiles + index];
}

void sprite_paint_single_image(sprite_t *sprite, graphic_t *graphic, int tex_index, int x, int y)
{
    if(!sprite || !graphic) return;
    if(tex_index < 0) return;

    size_t total_tex = sprite->total_tiles;
    if(total_tex <= tex_index) return;

    graphic_draw_region(graphic, sprite_get_tile(sprite, tex_index), x, y, 0);
    return;
}

void sprite_paint_tiles_image(sprite_t *sprite, graphic_t *graphic, int pos_info_index, int x, int y)
{
    int count, offset;
    size_t total_pos_info;
    tile_pos_t *tile_pos;
    tile_pos_info_t *tile_pos_info;

    if(!sprite || !graphic) return;
    if(pos_info_index < 0) return;

    total_pos_info = sprite->total_tile_pos_info;
    if(total_pos_info <= pos_info_index) return;

    // 不加 NULL 判断也没事，毕竟在 sprite_load 里面就检查了
    tile_pos = sprite->tile_pos;
    tile_pos_info = sprite->tile_pos_info;

    count = tile_pos_info[pos_info_index].count;
    offset = tile_pos_info[pos_info_index].offset;

    int tex_index, tile_x, tile_y, transform;
    for(int i = 0; i < count; i++)
    {
        tex_index = tile_pos[i+offset].index;
        tile_x = tile_pos[i+offset].x;
        tile_y = tile_pos[i+offset].y;
        transform = tile_pos[i+offset].transform;
        graphic_draw_region(graphic, sprite_get_tile(sprite, tex_index), x + tile_x, y + tile_y, transform);
    }
    return;
}
