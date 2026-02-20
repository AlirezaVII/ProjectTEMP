#ifndef RENDERER_H
#define RENDERER_H

#include "SDL.h"

/* Circle drawing */
void renderer_fill_circle(SDL_Renderer *r, int cx, int cy, int radius,
                          int red, int green, int blue);
void renderer_draw_circle(SDL_Renderer *r, int cx, int cy, int radius,
                          int red, int green, int blue);

/* Rounded rect */
void renderer_fill_rounded_rect(SDL_Renderer *r, const SDL_Rect *rect,
                                int radius, int red, int green, int blue);

/* Texture loading (PNG via SDL2_image) */
SDL_Texture* renderer_load_texture(SDL_Renderer *r, const char *path);

/* Draw texture stretched into dst rect */
void renderer_draw_texture(SDL_Renderer *r, SDL_Texture *tex, const SDL_Rect *dst);

/* Draw texture keeping aspect ratio, centered in dst rect */
void renderer_draw_texture_fit(SDL_Renderer *r, SDL_Texture *tex, const SDL_Rect *dst);

#endif
