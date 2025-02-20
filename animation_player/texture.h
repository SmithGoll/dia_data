#include <stdint.h>
#include "graphic.h"
#include "palette.h"

#ifndef _TEXTURE_H_
#define _TEXTURE_H_

struct texture_s;
typedef struct texture_s texture_t;

int get_texture_width(const texture_t *tex);
int get_texture_height(const texture_t *tex);
const uint32_t *get_texture_pixels(const texture_t *tex);
tex_t *texture_create_graphic_texture(texture_t *texture, graphic_t *graphic);
void texture_free(texture_t *texture);
texture_t *texture_load(const uint8_t *palette_index, palette_t *palette, int w, int h, int scale);

#endif
