#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "container.h"

typedef struct
{
    int total_pixels;
    uint32_t *pixel_data;
} palette_t;

static inline void bytes_copy(const uint8_t *src, size_t offset, void *dest, size_t size)
{
    uint8_t *dst = (uint8_t*)dest;
    for(int i = 0; i < size; i++)
    {
        dst[i] = src[i + offset];
    }
    return;
}

size_t get_palette_count(container_t *palettes)
{
    return container_get_size(palettes);
}

palette_t *get_palette(container_t *palettes, size_t index)
{
    return (palette_t*)container_get(palettes, index);
}

uint32_t get_palette_color(palette_t *palette, size_t color_index)
{
    if(!palette || color_index >= palette->total_pixels)
    {
        return 0;
    }
    return palette->pixel_data[color_index];
}

void palette_free(container_t *palettes)
{
    if(!palettes) return;

    size_t total_palette = get_palette_count(palettes);
    if(total_palette)
    {
        for(int i = 0; i < total_palette; i++)
        {
            palette_t *tmp = get_palette(palettes, i);
            if(tmp)
            {
                free(tmp->pixel_data);
                free(tmp);
            }
        }
    }
    free(palettes);
    return;
}

container_t *palette_load(const uint8_t *data, uint16_t pixel_format, uint8_t total_palette, uint8_t total_pixels)
{
    if(!total_palette || !total_pixels) return NULL;
    if(!(pixel_format == 0x8888 || pixel_format == 0x4444 || pixel_format == 0x5515 || pixel_format == 0x6505)) return NULL;
    container_t *res = container_alloc(total_palette);
    if(!res) return NULL;

    int cur_pos = 0;
    for(int i = 0; i < total_palette; i++)
    {
        palette_t *palette_tmp = (palette_t*)malloc(sizeof(palette_t));
        uint32_t *t = (uint32_t*)calloc(total_pixels, sizeof(uint32_t));
        if(!t)
        {
            cur_pos += pixel_format == 0x8888 ? 4 * total_pixels : 2 * total_pixels;
            if(!palette_tmp) free(palette_tmp);
            container_put(res, NULL, i);
            continue;
        }

        switch(pixel_format)
        {
            case 0x8888: {
                for(int j = 0; j < total_pixels; j++)
                {
                    uint32_t c;
                    bytes_copy((const uint8_t*)data, cur_pos, (void*)&c, sizeof(c));
                    cur_pos += 4;
                    if((c & 0xFF000000) != 0xFF000000)
                    {
                        // have_alpha = true;
                    }
                    t[j] = c;
                }
                break;
            }
            case 0x4444: {
                for(int j = 0; j < total_pixels; j++)
                {
                    uint16_t c;
                    bytes_copy((const uint8_t*)data, cur_pos, (void*)&c, sizeof(c));
                    cur_pos += 2;
                    if((c & 0xF000) != 0xF000)
                    {
                        // have_alpha = true;
                    }
                    t[j] = ((c & 0xF000) << 16 |    // A
                            (c & 0xF000) << 12 |    // A
                            (c & 0x0F00) << 12 |    // R
                            (c & 0x0F00) << 8 |     // R
                            (c & 0x00F0) << 8 |     // G
                            (c & 0x00F0) << 4 |     // G
                            (c & 0x000F) << 4 |     // B
                            (c & 0x000F));          // B
                }
                break;
            }
            case 0x5515: {
                for(int j = 0; j < total_pixels; j++)
                {
                    uint16_t c;
                    bytes_copy((const uint8_t*)data, cur_pos, (void*)&c, sizeof(c));
                    cur_pos += 2;
                    unsigned int alpha = 0xFF000000;
                    if((c & 0x8000) != 0x8000)
                    {
                        // alpha = 0;
                        // have_alpha = true;
                        t[j] = 0;
                        continue;
                    }
                    t[j] = (alpha | (c & 0x7C00) << 9 | (c & 0x3E0) << 6 | (c & 0x1F) << 3);
                }
                break;
            }
            case 0x6505: {
                for(int j = 0; j < total_pixels; j++)
                {
                    uint16_t c;
                    bytes_copy((const uint8_t*)data, cur_pos, (void*)&c, sizeof(c));
                    cur_pos += 2;
                    unsigned int alpha = 0xFF000000;
                    if(c == 0xF81F)
                    {
                        // alpha = 0;
                        // have_alpha = true;
                        t[j] = 0;
                        continue;
                    }
                    t[j] = (alpha | (c & 0xF800) << 8 | (c & 0x7E0) << 5 | (c & 0x1F) << 3);
                }
            }
        }
        palette_tmp->total_pixels = total_pixels;
        palette_tmp->pixel_data = t;
        container_put(res, palette_tmp, i);
    }
    return res;
}
