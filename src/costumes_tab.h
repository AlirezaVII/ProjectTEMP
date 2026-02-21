#ifndef COSTUMES_TAB_H
#define COSTUMES_TAB_H

#include "SDL.h"
#include "SDL_ttf.h"
#include "types.h"
#include "textures.h"
#include <vector>

struct CostumesRects {
    SDL_Rect name_box;
    SDL_Rect tools[7];
    SDL_Rect del_tool;
    SDL_Rect flip_h;
    SDL_Rect flip_v;
    SDL_Rect canvas;
    std::vector<SDL_Rect> thumbs;
    std::vector<SDL_Rect> thumb_dels;
    SDL_Rect add_btn;
};

CostumesRects get_costumes_rects(const AppState &state);
void costumes_tab_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state, const Textures &tex);
bool costumes_tab_handle_event(const SDL_Event &e, AppState &state);

#endif