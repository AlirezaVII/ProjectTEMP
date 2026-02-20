#ifndef STAGE_H
#define STAGE_H

#include "SDL.h"
#include "SDL_ttf.h"
#include "types.h"

struct StageRects {
    SDL_Rect panel;
    SDL_Rect stage_area;
};

void stage_layout(StageRects &rects);
void stage_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state,
                const StageRects &rects);
bool stage_handle_event(const SDL_Event &e, AppState &state,
                        const StageRects &rects);

#endif
