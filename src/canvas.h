#ifndef CANVAS_H
#define CANVAS_H

#include "SDL.h"
#include "SDL_ttf.h"
#include "types.h"
#include "textures.h"
#include "palette.h"

struct CanvasRects {
    SDL_Rect panel;
};

void canvas_layout(CanvasRects &rects);
void canvas_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state,
                 const CanvasRects &rects, const Textures &tex);
bool canvas_handle_event(const SDL_Event &e, AppState &state,
                         const CanvasRects &rects,
                         const PaletteRects &pal_rects,
                         TTF_Font *font);

#endif