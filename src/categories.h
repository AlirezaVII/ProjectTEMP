#ifndef CATEGORIES_H
#define CATEGORIES_H

#include "SDL.h"
#include "SDL_ttf.h"
#include "types.h"

static const int NUM_CATEGORIES = 9;

struct CategoriesRects {
    SDL_Rect panel;
    SDL_Rect items[NUM_CATEGORIES];
    SDL_Rect dots[NUM_CATEGORIES];
};

void categories_layout(CategoriesRects &rects);
void categories_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state,
                     const CategoriesRects &rects);
bool categories_handle_event(const SDL_Event &e, AppState &state,
                             const CategoriesRects &rects);

#endif
