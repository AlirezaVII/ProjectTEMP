#ifndef SETTINGS_H
#define SETTINGS_H

#include "SDL.h"
#include "SDL_ttf.h"
#include "types.h"
#include "textures.h"

struct SettingsRects {
    SDL_Rect panel;
    
    SDL_Rect sprite_name_label;
    SDL_Rect sprite_name_input;
    
    SDL_Rect x_icon;
    SDL_Rect x_input;
    SDL_Rect y_icon;
    SDL_Rect y_input;
    
    SDL_Rect show_label;
    SDL_Rect vis_on_btn;
    SDL_Rect vis_off_btn;
    
    SDL_Rect size_label;
    SDL_Rect size_input;
    
    SDL_Rect dir_label_pos;
    SDL_Rect dir_input;
};

void settings_layout(SettingsRects &rects);
void settings_draw(SDL_Renderer *r, TTF_Font *font, AppState &state,
                   const SettingsRects &rects, const Textures &tex);
bool settings_handle_event(const SDL_Event &e, AppState &state,
                           const SettingsRects &rects);

#endif