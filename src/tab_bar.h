#ifndef TAB_BAR_H
#define TAB_BAR_H

#include "SDL.h"
#include "SDL_ttf.h"
#include "types.h"
#include "textures.h"

struct TabBarRects {
    SDL_Rect bar;
    SDL_Rect tabs[3];
    SDL_Rect tab_icons[3];
    SDL_Rect start_btn;
    SDL_Rect stop_btn;
};

void tab_bar_layout(TabBarRects &rects);
void tab_bar_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state,
                  const TabBarRects &rects, const Textures &tex);
bool tab_bar_handle_event(const SDL_Event &e, AppState &state,
                          const TabBarRects &rects);

#endif
