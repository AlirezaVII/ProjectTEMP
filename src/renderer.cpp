#include "renderer.h"
#include "SDL_image.h"

void renderer_fill_circle(SDL_Renderer *r, int cx, int cy, int radius,
                          int red, int green, int blue)
{
    SDL_SetRenderDrawColor(r, red, green, blue, 255);
    for (int dy = -radius; dy <= radius; ++dy) {
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
    while (x >= y) {
        SDL_RenderDrawPoint(r, cx + x, cy + y);
        SDL_RenderDrawPoint(r, cx - x, cy + y);
        SDL_RenderDrawPoint(r, cx + x, cy - y);
        SDL_RenderDrawPoint(r, cx - x, cy - y);
        SDL_RenderDrawPoint(r, cx + y, cy + x);
        SDL_RenderDrawPoint(r, cx - y, cy + x);
        SDL_RenderDrawPoint(r, cx + y, cy - x);
        SDL_RenderDrawPoint(r, cx - y, cy - x);
        ++y;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
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
        {rect->x + radius,              rect->y + radius},
        {rect->x + rect->w - radius - 1, rect->y + radius},
        {rect->x + radius,              rect->y + rect->h - radius - 1},
        {rect->x + rect->w - radius - 1, rect->y + rect->h - radius - 1}
    };
    for (int c = 0; c < 4; ++c) {
        int ccx = corners[c][0];
        int ccy = corners[c][1];
        for (int dy = -radius; dy <= radius; ++dy) {
            int dx = 0;
            while (dx * dx + dy * dy <= radius * radius)
                ++dx;
            --dx;
            SDL_RenderDrawLine(r, ccx - dx, ccy + dy, ccx + dx, ccy + dy);
        }
    }
}

SDL_Texture* renderer_load_texture(SDL_Renderer *r, const char *path)
{
    SDL_Surface *surf = IMG_Load(path);
    if (!surf) {
        SDL_Log("IMG_Load failed for '%s': %s", path, IMG_GetError());
        return NULL;
    }
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_FreeSurface(surf);
    if (!tex) {
        SDL_Log("CreateTexture failed for '%s': %s", path, SDL_GetError());
    }
    return tex;
}

void renderer_draw_texture(SDL_Renderer *r, SDL_Texture *tex, const SDL_Rect *dst)
{
    if (tex) {
        SDL_RenderCopy(r, tex, NULL, dst);
    }
}

void renderer_draw_texture_fit(SDL_Renderer *r, SDL_Texture *tex, const SDL_Rect *dst)
{
    if (!tex) return;
    int tw = 0, th = 0;
    SDL_QueryTexture(tex, NULL, NULL, &tw, &th);
    if (tw == 0 || th == 0) return;

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
