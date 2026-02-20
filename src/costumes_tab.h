#ifndef COSTUMES_TAB_H
#define COSTUMES_TAB_H

#include "SDL.h"
#include "SDL_ttf.h"
#include "types.h"

void costumes_tab_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state);
bool costumes_tab_handle_event(const SDL_Event &e, AppState &state);

#endif
