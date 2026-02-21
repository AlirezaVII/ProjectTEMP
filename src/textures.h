#ifndef TEXTURES_H
#define TEXTURES_H

#include "SDL.h"

struct Textures {
    SDL_Texture *code_active; SDL_Texture *code_inactive;
    SDL_Texture *brush_active; SDL_Texture *brush_nonactive;
    SDL_Texture *brush_tab_active;
    SDL_Texture *volume_active; SDL_Texture *volume_inactive;

    SDL_Texture *green_flag; SDL_Texture *pause1; SDL_Texture *pause2;

    SDL_Texture *fit_width; SDL_Texture *height_icon;
    SDL_Texture *vis_on_active; SDL_Texture *vis_on_inactive;
    SDL_Texture *vis_off_active; SDL_Texture *vis_off_inactive;
    SDL_Texture *rotate_left; SDL_Texture *rotate_right;

    SDL_Texture *logo;
    SDL_Texture *search_icon; SDL_Texture *surprise_icon; SDL_Texture *upload_icon;
    SDL_Texture *sprite_btn_icon; SDL_Texture *backdrop_btn_icon;
    SDL_Texture *scratch_cat; SDL_Texture *delete_sprite;

    SDL_Texture *cloud; SDL_Texture *vol_up; SDL_Texture *vol_down;
    SDL_Texture *vol_mute; SDL_Texture *play_icon;

    // ---> NEW: EDITOR ICONS <---
    SDL_Texture *mouse_active; SDL_Texture *mouse_nonactive;
    SDL_Texture *eraser_active; SDL_Texture *eraser_nonactive;
    SDL_Texture *text_active; SDL_Texture *text_nonactive;
    SDL_Texture *fill_active; SDL_Texture *fill_nonactive;
    SDL_Texture *rect_active; SDL_Texture *rect_nonactive;
    SDL_Texture *circ_active; SDL_Texture *circ_nonactive;
    SDL_Texture *trash_active; SDL_Texture *trash_nonactive;
    SDL_Texture *flip_h; SDL_Texture *flip_v;
};

bool textures_load(Textures &tex, SDL_Renderer *r);
void textures_free(Textures &tex);

#endif