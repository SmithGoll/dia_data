#include "graphic.h"
#include "sprite.h"

#define FPS 4

// For animation use
int frames_count = 0;
int display_single_image = 0;

int chunk_index;
const char *animation_src;

sprite_t *animation;
sprite_t *background;

int init_res(graphic_t *graphic)
{
    chunk_t *chunks[2];
    // Animation chunk load
    chunks[0] = chunk_load(animation_src, chunk_index);
    // Background
    chunks[1] = chunk_load("files/0.f", 3);

    animation = sprite_load(chunks[0], 5, graphic);
    //sprite_set_cur_palette(animation, 2);
    background = sprite_load(chunks[1], 5, graphic);
    if(!animation || !background) return 1;

    for(int i = 0; i < 2; i++)
        free(chunks[i]);
    return 0;
}

void draw_background(graphic_t *graphic)
{
    for(int y = 0; y < 3; y++)
    {
        for(int x = 0; x < 3; x++)
        {
            sprite_paint_single_image(background, graphic, 0, x * 120, y * 120);
        }
    }
    return;
}

void draw_animation(graphic_t *graphic)
{
    if(display_single_image)
        sprite_paint_single_image(animation, graphic, frames_count, 120, 120);
    else
        sprite_paint_tiles_image(animation, graphic, frames_count, 120, 120);
    return;
}

int main(int argc, const char **argv)
{
    graphic_t *graphic;

    if(argc != 3) return -1;
    animation_src = argv[1];
    chunk_index = atoi(argv[2]);

    if(graphic_init(&graphic, "DiamondRush Animation Player", 120 * 3, 120 * 3))
    {
        fprintf(stderr, "Failed to init graphic.\n");
        return -1;
    }

    if(init_res(graphic))
    {
        fprintf(stderr, "Failed to init resources.\n");
        return -1;
    }

    int total_image = sprite_get_tile_pos_info_count(animation);
    if(!total_image)
    {
        display_single_image = 1;
        if(!(total_image = sprite_get_tile_count(animation)))
        {
            fprintf(stderr, "No images found in file.\n");
            return -1;
        }
        fprintf(stderr, "Info: Player will display single image.\n");
    }

    graphic_show_window(graphic);

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

        draw_background(graphic);
        draw_animation(graphic);

        graphic_present(graphic);

        uint32_t current = SDL_GetTicks();
        uint32_t cost = current - begin;
        long delay = (1000/FPS) - cost;
        if(delay > 0)
        {
            SDL_Delay(delay);
        }

        frames_count = (frames_count + 1) % total_image;
    }
    sprite_free(background);
    sprite_free(animation);
    graphic_quit(graphic);
    return 0;
}