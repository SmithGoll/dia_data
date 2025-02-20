#include <SDL2/SDL.h>

#include <zlib.h>
#include <errno.h>
static unsigned char *_zlib_compress(unsigned char *data, int data_len, int *out_len, int quality)
{
    // quick and dirty
    unsigned long out_len_ = compressBound(data_len);
    unsigned char *data_out = malloc(out_len_);

    if(data_out != NULL)
    {
        if(compress2(data_out, &out_len_, data, data_len, quality) == Z_OK)
        {
            *out_len = (int)out_len_;
            return data_out;
        }
        else {
            // TODO: handle compression error?
            free(data_out);
            return NULL;
        }
    } else {
        // Out of memory?
        return NULL;
    }
}
#define STBIW_ZLIB_COMPRESS _zlib_compress
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#include "stb_image_write.h"

enum
{
    TRANS_NONE = 0,
    TRANS_MIRROR,
    TRANS_MIRROR_ROT180,
    TRANS_ROT180
};

typedef struct
{
    int width;
    int height;
    SDL_Window *window;
    SDL_Renderer *render;
} graphic_t;

typedef struct
{
    int width;
    int height;
    SDL_Texture *texture;
} tex_t;

int graphic_init(graphic_t **graphic, const char *window_name, int width, int height)
{
    graphic_t *res;
    SDL_Window *win = NULL;
    SDL_Renderer *render = NULL;

    if(!graphic)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
            "Error: Invalid param (graphic is NULL)");
        return -1;
    }

    res = (graphic_t*)malloc(sizeof(graphic_t));
    if(!res)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
            "Failed to alloc memory!");
        return -1;
    }
    res->width = width;
    res->height = height;

    // 设置 video 的 LOG_LEVEL
    // SDL_LogSetPriority(SDL_LOG_CATEGORY_VIDEO, SDL_LOG_PRIORITY_ERROR);

    // 初始化 SDL，注意 SDL_INIT_VIDEO 会初始化 SDL_INIT_EVENTS
    if(SDL_Init(SDL_INIT_VIDEO))
    {
        // 初始化失败，处理错误
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
            "Couldn't initialize SDL: %s", SDL_GetError());
        goto fail;
    }

    // 创建窗口
    res->window = win = SDL_CreateWindow(
        window_name, 
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        width, height,
        SDL_WINDOW_HIDDEN | SDL_WINDOW_ALLOW_HIGHDPI);
    if(!win)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
            "CreateWindow failed: %s", SDL_GetError());
        goto fail;
    }

    res->render = render = SDL_CreateRenderer(win, -1, /*SDL_RENDERER_SOFTWARE);*/SDL_RENDERER_ACCELERATED);
    if(!render)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
            "CreateRenderer failed: %s", SDL_GetError());
        goto fail;
    }

    *graphic = res;
    return 0;
fail:
    if(render) SDL_DestroyRenderer(render);
    if(win) SDL_DestroyWindow(win);
    if(res) free(res);
    SDL_Quit();
    return -1;
}

int graphic_get_width(graphic_t *graphic)
{
    if(!graphic) return -1;

    return graphic->width;
}

int graphic_get_height(graphic_t *graphic)
{
    if(!graphic) return -1;

    return graphic->height;
}

void *graphic_get_render(graphic_t *graphic)
{
    if(!graphic) return NULL;

    return (void*)(graphic->render);
}

void graphic_show_window(graphic_t *graphic)
{
    if(!graphic) return;

    SDL_ShowWindow(graphic->window);
    return;
}

void graphic_draw_region(graphic_t *graphic, tex_t *texture, int x, int y, int transform)
{
    double rotate = 0.0f;
    SDL_RendererFlip filp = SDL_FLIP_NONE;

    if(!graphic || !texture) return;
    if(texture->width <= 0 || texture->height <= 0) return;

    SDL_Rect dstrect = { x, y, texture->width, texture->height };

    switch(transform & 0x3)
    {
        case TRANS_MIRROR_ROT180:       // 2
            rotate = 180.0f;
        case TRANS_MIRROR:              // 1
            filp = SDL_FLIP_HORIZONTAL;
            break;
        case TRANS_ROT180:              // 3
            rotate = 180.0f;
        case TRANS_NONE:                // 0
            break;
    }
    SDL_RenderCopyEx(graphic->render, texture->texture, NULL, &dstrect, rotate, NULL, filp);
    return;
}

void graphic_present(graphic_t *graphic)
{
    if(!graphic) return;

    SDL_RenderPresent(graphic->render);
    return;
}

void graphic_quit(graphic_t *graphic)
{
    if(!graphic) return;

    SDL_DestroyWindow(graphic->window);
    SDL_DestroyRenderer(graphic->render);
    free(graphic);
    SDL_Quit();

    return;
}

tex_t *graphic_create_texture(graphic_t *graphic, const void *pixels, int w, int h, int pitch)
{
    tex_t *res;
    SDL_Texture *tex = NULL;

    if(!graphic) return NULL;

    res = (tex_t*)malloc(sizeof(tex_t));
    if(!res) return NULL;
    res->width = w;
    res->height = h;

    tex = SDL_CreateTexture(graphic->render, SDL_PIXELFORMAT_BGRA32, SDL_TEXTUREACCESS_STATIC, w, h);
    if(!tex) goto fail;
    res->texture = tex;

    if(SDL_UpdateTexture(tex, NULL, pixels, pitch))
    {
        goto fail;
    }

    if(SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND)) puts("Failed to set blend");
    return res;
fail:
    if(tex) SDL_DestroyTexture(tex);
    if(res) free(res);
    return NULL;
}

void graphic_destroy_texture(tex_t *tex)
{
    if(!tex) return;
    SDL_DestroyTexture(tex->texture);
    free(tex);
    return;
}

int graphic_take_screenshot(graphic_t *graphic, const char *filename)
{
    int res = -1;

    int w = graphic_get_width(graphic);
    int h = graphic_get_height(graphic);
    if(w <= 0 || h <= 0)
    {
        SDL_SetError("Invalid graphic!");
        goto fail1;
    }

    void *pixel_data = malloc(w * h * 4);
    if(!pixel_data) return -1;

    if(SDL_RenderReadPixels(
        graphic->render, NULL,
        SDL_PIXELFORMAT_RGBA32,
        pixel_data, w * 4))
    {
        SDL_SetError("Failed to read render pixels");
        goto fail2;
    }

    if(!stbi_write_png(filename, w, h, 4, pixel_data, w * 4))
    {
        res = 0;
    }
    else
    {
        SDL_SetError("%s: %s", filename, strerror(errno));
    }

fail2:
    free(pixel_data);
fail1:
    return res;
}
