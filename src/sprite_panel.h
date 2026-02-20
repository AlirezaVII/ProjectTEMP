#ifndef SPRITE_PANEL_H
#define SPRITE_PANEL_H

#include "SDL.h"
#include "SDL_ttf.h"
#include "types.h"
#include "textures.h"

struct SpritePanelRects {
    /* Sprite list area (left side, below sprite-info) */
    SDL_Rect sprite_list_area;

    /* Individual sprite thumbnail slot (just one for now: Sprite1) */
    SDL_Rect sprite_thumb;

    /* Sprite "+" circle button (bottom-right of sprite list) */
    SDL_Rect sprite_btn;

    /* Sprite hover menu (3 items: upload, surprise, search) */
    SDL_Rect sprite_menu;
    SDL_Rect sprite_menu_items[3];

    /* Backdrop panel (right side, below stage) */
    SDL_Rect backdrop_area;

    /* Backdrop preview thumbnail */
    SDL_Rect backdrop_thumb;

    /* Backdrop label area */
    SDL_Rect backdrop_label;

    /* Backdrop "+" circle button (bottom-right of backdrop area) */
    SDL_Rect backdrop_btn;

    /* Backdrop hover menu (3 items: upload, surprise, search) */
    SDL_Rect backdrop_menu;
    SDL_Rect backdrop_menu_items[3];
};

void sprite_panel_layout(SpritePanelRects &rects);
void sprite_panel_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state,
                       const Textures &tex, const SpritePanelRects &rects);
bool sprite_panel_handle_event(const SDL_Event &e, AppState &state,
                               const SpritePanelRects &rects);

#endif
