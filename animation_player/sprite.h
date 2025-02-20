#include <stdint.h>
#include "chunk.h"
#include "graphic.h"

#ifndef _SPRITE_H_
#define _SPRITE_H_

struct sprite_s;
typedef struct sprite_s sprite_t;

void sprite_free(sprite_t *sprite);
sprite_t *sprite_load(chunk_t *sprite_chunk, int scale, graphic_t *graphic);
void sprite_set_cur_palette(sprite_t *sprite, int index);
size_t sprite_get_tile_count(sprite_t *sprite);
size_t sprite_get_tile_pos_info_count(sprite_t *sprite);
tex_t *sprite_get_tile(sprite_t *sprite, int index);
void sprite_paint_single_image(sprite_t *sprite, graphic_t *graphic, int tex_index, int x, int y);
void sprite_paint_tiles_image(sprite_t *sprite, graphic_t *graphic, int pos_info_index, int x, int y);

#endif
