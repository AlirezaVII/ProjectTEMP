#ifndef PALETTE_H
#define PALETTE_H

#include "SDL.h"
#include "SDL_ttf.h"
#include "types.h"
#include "textures.h"

struct PaletteRects {
    SDL_Rect panel;
};

void palette_layout(PaletteRects &rects);
void palette_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state,
                  const PaletteRects &rects, const Textures &tex);
bool palette_handle_event(const SDL_Event &e, AppState &state,
                          const PaletteRects &rects,
                          TTF_Font *font);

#endif