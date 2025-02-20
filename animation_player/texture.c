#include <stdint.h>
#include <stdlib.h>
#include "graphic.h"
#include "palette.h"

typedef struct
{
    int w;
    int h;
    uint32_t *pixel_data;
} texture_t;

int get_texture_width(const texture_t *tex)
{
    if(!tex) return -1;

    return tex->w;
}

int get_texture_height(const texture_t *tex)
{
    if(!tex) return -1;

    return tex->h;
}

const uint32_t *get_texture_pixels(const texture_t *tex)
{
    if(!tex) return NULL;

    return tex->pixel_data;
}

tex_t *texture_create_graphic_texture(texture_t *texture, graphic_t *graphic)
{
    if(!texture || !graphic) return NULL;
    return graphic_create_texture(graphic, texture->pixel_data, texture->w, texture->h, texture->w * 4);
}

void texture_free(texture_t *texture)
{
    if(!texture) return;

    free(texture->pixel_data);
    free(texture);
    return;
}

// 只支持类型 0x1600
texture_t *texture_load(const uint8_t *palette_index, palette_t *palette, int w, int h, int scale)
{
    int need_scale = 0;
    texture_t *res = NULL;
    uint32_t *pixels = NULL;
    int total_index = w * h / 2;
    uint32_t *pixels_tmp = NULL;

    if(total_index <= 0) return NULL;
    if(!palette_index || !palette) return NULL;
    if(scale > 1) need_scale = 1;

    pixels_tmp = (uint32_t*)malloc(w * h);
    res = (texture_t*)malloc(sizeof(texture_t));
    if(!res || !pixels_tmp)
    {
        goto fail;
    }

    int cur_pos = 0;
    for(int i = 0; i < total_index; i++)
    {
        pixels_tmp[cur_pos++] = get_palette_color(palette, (palette_index[i] >> 4) & 0xF);
        pixels_tmp[cur_pos++] = get_palette_color(palette, palette_index[i] & 0xF);
    }

    // Rescale the image
    if(need_scale)
    {
        w *= scale;
        h *= scale;

        pixels = (uint32_t*)malloc(w * h * sizeof(uint32_t));
        if(!pixels) goto fail;

        cur_pos = 0;
        for(int y = 0; y < h; y++)
        {
            for(int x = 0; x < w; x++)
            {
                pixels[cur_pos++] = pixels_tmp[x / scale + (y / scale) * (w / scale)];
            }
        }
        free(pixels_tmp);
        res->pixel_data = pixels;
    }
    else
    {
        res->pixel_data = pixels_tmp;
    }
    res->w = w;
    res->h = h;
    return res;
fail:
    if(res) free(res);
    if(pixels_tmp) free(pixels_tmp);
    return NULL;
}
