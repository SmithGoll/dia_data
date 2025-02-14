#ifndef _GRAPHIC_H_
#define _GRAPHIC_H_

struct graphic_s;
typedef struct graphic_s graphic_t;

struct tex_s;
typedef struct tex_s tex_t;

int graphic_init(graphic_t **graphic, const char *window_name, int width, int height);
int graphic_get_width(graphic_t *graphic);
int graphic_get_height(graphic_t *graphic);
void *graphic_get_render(graphic_t *graphic);
void graphic_show_window(graphic_t *graphic);
void graphic_draw_region(graphic_t *graphic, tex_t *texture, int x, int y, int transform);
void graphic_present(graphic_t *graphic);
void graphic_quit(graphic_t *graphic);
tex_t *graphic_create_texture(graphic_t *graphic, const void *pixels, int w, int h, int pitch);
void graphic_destroy_texture(tex_t *tex);
int graphic_take_screenshot(graphic_t *graphic, const char *filename);

#endif
