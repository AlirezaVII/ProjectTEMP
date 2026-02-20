#ifndef NAVBAR_H
#define NAVBAR_H

#include "SDL.h"
#include "SDL_ttf.h"
#include "types.h"
#include "textures.h"

struct NavbarRects {
    SDL_Rect bar;
    SDL_Rect logo_rect;
    SDL_Rect file_btn;
    SDL_Rect project_input;
};

void navbar_layout(NavbarRects &rects);
void navbar_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state,
                 const NavbarRects &rects, const Textures &tex);
bool navbar_handle_event(const SDL_Event &e, AppState &state,
                         const NavbarRects &rects);

#endif
