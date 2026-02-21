#ifndef SOUNDS_TAB_H
#define SOUNDS_TAB_H

#include <SDL.h>
#include <SDL_ttf.h>
#include "types.h"
#include "textures.h" // <--- FIXED: Added this include

// <--- FIXED: Added 'const Textures &tex' to the signature
void sounds_tab_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state, const Textures &tex);
bool sounds_tab_handle_event(const SDL_Event &e, AppState &state);

#endif