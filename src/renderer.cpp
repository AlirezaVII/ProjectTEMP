#include "renderer.h"
#include "SDL_image.h"
#include <cmath>
#include <algorithm>

void renderer_fill_circle(SDL_Renderer *r, int cx, int cy, int radius,
                          int red, int green, int blue)
{
    SDL_SetRenderDrawColor(r, red, green, blue, 255);
    for (int dy = -radius; dy <= radius; ++dy)
    {
        int dx = 0;
        while (dx * dx + dy * dy <= radius * radius)
            ++dx;
        --dx;
        SDL_RenderDrawLine(r, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

void renderer_draw_circle(SDL_Renderer *r, int cx, int cy, int radius,
                          int red, int green, int blue)
{
    SDL_SetRenderDrawColor(r, red, green, blue, 255);
    int x = radius, y = 0, err = 1 - radius;
    while (x >= y)
    {
        SDL_RenderDrawPoint(r, cx + x, cy + y);
        SDL_RenderDrawPoint(r, cx - x, cy + y);
        SDL_RenderDrawPoint(r, cx + x, cy - y);
        SDL_RenderDrawPoint(r, cx - x, cy - y);
        SDL_RenderDrawPoint(r, cx + y, cy + x);
        SDL_RenderDrawPoint(r, cx - y, cy + x);
        SDL_RenderDrawPoint(r, cx + y, cy - x);
        SDL_RenderDrawPoint(r, cx - y, cy - x);
        ++y;
        if (err < 0)
        {
            err += 2 * y + 1;
        }
        else
        {
            --x;
            err += 2 * (y - x) + 1;
        }
    }
}

void renderer_fill_rounded_rect(SDL_Renderer *r, const SDL_Rect *rect,
                                int radius, int red, int green, int blue)
{
    SDL_SetRenderDrawColor(r, red, green, blue, 255);

    /* center body */
    SDL_Rect body;
    body.x = rect->x + radius;
    body.y = rect->y;
    body.w = rect->w - 2 * radius;
    body.h = rect->h;
    SDL_RenderFillRect(r, &body);

    /* left strip */
    SDL_Rect left;
    left.x = rect->x;
    left.y = rect->y + radius;
    left.w = radius;
    left.h = rect->h - 2 * radius;
    SDL_RenderFillRect(r, &left);

    /* right strip */
    SDL_Rect right;
    right.x = rect->x + rect->w - radius;
    right.y = rect->y + radius;
    right.w = radius;
    right.h = rect->h - 2 * radius;
    SDL_RenderFillRect(r, &right);

    /* four corners */
    int corners[4][2] = {
        {rect->x + radius, rect->y + radius},
        {rect->x + rect->w - radius - 1, rect->y + radius},
        {rect->x + radius, rect->y + rect->h - radius - 1},
        {rect->x + rect->w - radius - 1, rect->y + rect->h - radius - 1}};
    for (int c = 0; c < 4; ++c)
    {
        int ccx = corners[c][0];
        int ccy = corners[c][1];
        for (int dy = -radius; dy <= radius; ++dy)
        {
            int dx = 0;
            while (dx * dx + dy * dy <= radius * radius)
                ++dx;
            --dx;
            SDL_RenderDrawLine(r, ccx - dx, ccy + dy, ccx + dx, ccy + dy);
        }
    }
}

SDL_Texture *renderer_load_texture(SDL_Renderer *r, const char *path)
{
    SDL_Surface *surf = IMG_Load(path);
    if (!surf)
    {
        SDL_Log("IMG_Load failed for '%s': %s", path, IMG_GetError());
        return NULL;
    }
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_FreeSurface(surf);
    if (!tex)
    {
        SDL_Log("CreateTexture failed for '%s': %s", path, SDL_GetError());
    }
    return tex;
}

void renderer_draw_texture(SDL_Renderer *r, SDL_Texture *tex, const SDL_Rect *dst)
{
    if (tex)
    {
        SDL_RenderCopy(r, tex, NULL, dst);
    }
}

void renderer_draw_texture_fit(SDL_Renderer *r, SDL_Texture *tex, const SDL_Rect *dst)
{
    if (!tex)
        return;
    int tw = 0, th = 0;
    SDL_QueryTexture(tex, NULL, NULL, &tw, &th);
    if (tw == 0 || th == 0)
        return;

    float sw = (float)dst->w / (float)tw;
    float sh = (float)dst->h / (float)th;
    float scale = (sw < sh) ? sw : sh;

    int fw = (int)(tw * scale);
    int fh = (int)(th * scale);

    SDL_Rect fit;
    fit.x = dst->x + (dst->w - fw) / 2;
    fit.y = dst->y + (dst->h - fh) / 2;
    fit.w = fw;
    fit.h = fh;
    SDL_RenderCopy(r, tex, NULL, &fit);
}

// ---> PEN ENGINE IMPLEMENTATION (Using your exact math!) <---
SDL_Renderer *g_pen_renderer = nullptr;
SDL_Texture *g_pen_layer = nullptr;

void renderer_init_pen_layer(SDL_Renderer *r)
{
    g_pen_renderer = r;
    if (g_pen_layer)
        SDL_DestroyTexture(g_pen_layer);

    // Explicit fixed 480x360 coordinate system
    g_pen_layer = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 480, 360);
    SDL_SetTextureBlendMode(g_pen_layer, SDL_BLENDMODE_BLEND);
    renderer_clear_pen_layer();
}

void renderer_clear_pen_layer()
{
    if (!g_pen_layer || !g_pen_renderer)
        return;

    SDL_Texture *prev_target = SDL_GetRenderTarget(g_pen_renderer);
    SDL_SetRenderTarget(g_pen_renderer, g_pen_layer);
    SDL_SetRenderDrawColor(g_pen_renderer, 0, 0, 0, 0); // Transparent Background
    SDL_RenderClear(g_pen_renderer);
    SDL_SetRenderTarget(g_pen_renderer, prev_target); // Safely restore
}

void renderer_draw_line_on_pen_layer(int x1, int y1, int x2, int y2, int size, SDL_Color color)
{
    if (!g_pen_layer || !g_pen_renderer)
        return;

    SDL_Texture *prev_target = SDL_GetRenderTarget(g_pen_renderer);
    SDL_SetRenderTarget(g_pen_renderer, g_pen_layer);

    // Convert Scratch coordinates to Window screen coordinates
    int screen_x1 = x1 + 240;
    int screen_y1 = 180 - y1;
    int screen_x2 = x2 + 240;
    int screen_y2 = 180 - y2;

    int dx = screen_x2 - screen_x1;
    int dy = screen_y2 - screen_y1;
    int steps = std::max(abs(dx), abs(dy));

    // Draw thick interpolated line
    if (steps == 0)
    {
        renderer_fill_circle(g_pen_renderer, screen_x1, screen_y1, size, color.r, color.g, color.b);
    }
    else
    {
        float x_inc = dx / (float)steps;
        float y_inc = dy / (float)steps;
        float cx = screen_x1;
        float cy = screen_y1;
        for (int i = 0; i <= steps; i++)
        {
            renderer_fill_circle(g_pen_renderer, (int)cx, (int)cy, size, color.r, color.g, color.b);
            cx += x_inc;
            cy += y_inc;
        }
    }
    SDL_SetRenderTarget(g_pen_renderer, prev_target);
}

void renderer_stamp_on_pen_layer(const Sprite &spr)
{
    if (!g_pen_layer || !g_pen_renderer || !spr.texture)
        return;
        
    SDL_Texture *prev_target = SDL_GetRenderTarget(g_pen_renderer);
    SDL_SetRenderTarget(g_pen_renderer, g_pen_layer);

    int cx = 240 + spr.x;
    int cy = 180 - spr.y;
    int tex_w = 100, tex_h = 100;
    SDL_QueryTexture(spr.texture, NULL, NULL, &tex_w, &tex_h);

    // YOUR EXACT MATH
    int base_w = tex_w, base_h = tex_h;
    int MAX_DEFAULT = 120;
    if (base_w > MAX_DEFAULT || base_h > MAX_DEFAULT)
    {
        if (base_w > base_h)
        {
            base_h = (base_h * MAX_DEFAULT) / base_w;
            base_w = MAX_DEFAULT;
        }
        else
        {
            base_w = (base_w * MAX_DEFAULT) / base_h;
            base_h = MAX_DEFAULT;
        }
    }
    int w = (base_w * spr.size) / 100;
    int h = (base_h * spr.size) / 100;
    SDL_Rect dest = {cx - w / 2, cy - h / 2, w, h};

    double angle = spr.direction - 90.0;
    
    // 1. Check Flips
    SDL_RendererFlip flip = SDL_FLIP_NONE;
    if (!spr.costumes.empty() && spr.selected_costume >= 0 && spr.selected_costume < (int)spr.costumes.size()) {
        if (spr.costumes[spr.selected_costume].flip_h) flip = (SDL_RendererFlip)(flip | SDL_FLIP_HORIZONTAL);
        if (spr.costumes[spr.selected_costume].flip_v) flip = (SDL_RendererFlip)(flip | SDL_FLIP_VERTICAL);
    }

    // 2. Safely apply Blend Mode so backgrounds stay transparent!
    SDL_BlendMode oldMode;
    SDL_GetTextureBlendMode(spr.texture, &oldMode);
    SDL_SetTextureBlendMode(spr.texture, SDL_BLENDMODE_BLEND);

    // 3. Draw!
    SDL_RenderCopyEx(g_pen_renderer, spr.texture, NULL, &dest, angle, NULL, flip);
    
    // 4. Safely restore
    SDL_SetTextureBlendMode(spr.texture, oldMode);
    SDL_SetRenderTarget(g_pen_renderer, prev_target);
}