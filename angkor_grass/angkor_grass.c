#include <SDL2/SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#define STB_IMAGE_STATIC
#define STBI_ONLY_PNG
#include "stb_image.h"

#include "graphic.h"

tex_t *textures[14] = {};
struct
{
    int count;
    int offset;
} pos_info[] = {
    { 0x01, 0x00 },
    { 0x08, 0x01 },
    { 0x08, 0x09 },
    { 0x08, 0x11 },
    { 0x08, 0x19 },
    { 0x08, 0x21 },
    { 0x08, 0x29 },
    { 0x07, 0x31 }
};
struct
{
    int index;
    int x;
    int y;
    int transform;
} pos[] = {
    { 0, 0, 0, 0 },
    { 1, 0, 12, 0 },
    { 9, 8, 4, 0 },
    { 10, -1, 6, 0 },
    { 8, 20, 4, 0 },
    { 7, -1, 3, 0 },
    { 6, 15, 0, 0 },
    { 5, 3, 3, 0 },
    { 4, 2, -1, 0 },
    { 1, 0, 12, 0 },
    { 9, 7, 3, 0 },
    { 10, -3, 6, 0 },
    { 8, 21, 4, 0 },
    { 7, -2, 2, 0 },
    { 6, 16, -1, 0 },
    { 5, 2, 2, 0 },
    { 4, 1, -3, 0 },
    { 1, 0, 12, 0 },
    { 9, 6, 3, 0 },
    { 10, -4, 6, 0 },
    { 8, 23, 4, 0 },
    { 7, -3, 2, 0 },
    { 6, 17, -2, 0 },
    { 5, 1, 2, 0 },
    { 4, 0, -3, 0 },
    { 10, -4, 8, 0 },
    { 8, 23, 7, 0 },
    { 7, -4, 4, 0 },
    { 6, 18, 0, 0 },
    { 5, 1, 3, 0 },
    { 4, 0, -2, 0 },
    { 2, 0, 17, 0 },
    { 11, 7, 9, 0 },
    { 8, -3, 11, 1 },
    { 7, -5, 6, 0 },
    { 6, 21, 12, 0 },
    { 5, 1, 5, 0 },
    { 5, -1, 0, 1 },
    { 4, 19, 3, 1 },
    { 2, 0, 17, 0 },
    { 9, 11, 5, 1 },
    { 8, -3, 15, 1 },
    { 7, -6, 8, 0 },
    { 7, 22, 5, 1 },
    { 5, 1, 7, 0 },
    { 5, 22, 15, 0 },
    { 5, -3, 2, 1 },
    { 11, 7, 13, 0 },
    { 3, 0, 16, 0 },
    { 7, -2, 19, 0 },
    { 7, -3, 8, 0 },
    { 7, 21, 18, 1 },
    { 5, 2, 9, 0 },
    { 11, 5, 16, 0 },
    { 3, 0, 16, 0 },
    { 12, 24, 9, 0 }
};

void draw_background(graphic_t *graphic, tex_t *texture)
{
    for(int y = 0; y < 3; y++)
    {
        for(int x = 0; x < 3; x++)
        {
            graphic_draw_region(graphic, texture, x * 120, y * 120, 0);
        }
    }
    return;
}

int frames_count = 0;
void draw_animation(graphic_t *graphic)
{
    // 懒得加判断了
    int tex_index, x, y, transform;
    int count = pos_info[frames_count].count;
    int offset = pos_info[frames_count].offset;
    for(int i = 0; i < count; i++)
    {
        tex_index = pos[i+offset].index;
        x = pos[i+offset].x;
        y = pos[i+offset].y;
        transform = pos[i+offset].transform;

        graphic_draw_region(graphic, textures[tex_index], 120 + (x * 5), 120 + (y * 5), transform);
    }
    frames_count = (frames_count + 1) % 8;
    return;
}

int texture_init(graphic_t *graph)
{
    int w, h, n;
    stbi_uc *pixel_data;

    char buf[128];

    for(int i = 0; i < 14; i++)
    {
        w = h = n = 0;
        sprintf(buf, "tiles/%d.png", i);
        pixel_data = stbi_load(buf, &w, &h, &n, 4);
        if(!w || !h || !n)
        {
            fprintf(stderr, "Error: %s\n", stbi__g_failure_reason);
            return -1;
        }
        textures[i] = graphic_create_texture(graph, pixel_data, w, h, w * 4);
        free(pixel_data);
    }
    return 0;
}

int main(int argc, const char **argv)
{
    int save_frame = 0;
    if(argc != 1) save_frame = 1;

    graphic_t *graph;
    if(graphic_init(&graph, "Angkor Grass Animation", 120 * 3, 120 * 3))
        return -1;

    if(texture_init(graph))
        return -1;

    if(save_frame)
    {
        char buf[128];
        for(int i = 0; i < 8; i++)
        {
            sprintf(buf, "save/angkor_grass_frame%d.png", i+1);
            draw_background(graph, textures[13]);
            draw_animation(graph);
            graphic_present(graph);
            graphic_take_screenshot(graph, buf);
        }
        goto release_res;
    }

    graphic_show_window(graph);

    SDL_Event event;
    SDL_bool running = SDL_TRUE;
    while(1)
    {
        uint32_t begin = SDL_GetTicks();

        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                case SDL_QUIT:
                    running = SDL_FALSE;
                default:
                    break;
            }
        }
        if(!running) break;

        draw_background(graph, textures[13]);
        draw_animation(graph);

        graphic_present(graph);

        uint32_t current = SDL_GetTicks();
        uint32_t cost = current - begin;
        long delay = (1000/8) - cost;
        if(delay > 0)
        {
            SDL_Delay(delay);
        }
    }
release_res:
    // 释放资源
    for(int i = 0; i < 14; i++)
    {
        graphic_destroy_texture(textures[i]);
    }
    graphic_quit(graph);
    return 0;
}
