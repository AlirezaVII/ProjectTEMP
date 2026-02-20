#ifndef DRAG_AREA_H
#define DRAG_AREA_H

#include "SDL.h"
#include "SDL_ttf.h"
#include "types.h"

struct DragAreaRects {
    SDL_Rect panel;
};

void drag_area_layout(DragAreaRects &rects);
void drag_area_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state,
                    const DragAreaRects &rects);

#endif
