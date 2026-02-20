#ifndef TEXTURES_H
#define TEXTURES_H

#include "SDL.h"

struct Textures {
    /* tab icons */
    SDL_Texture *code_active;
    SDL_Texture *code_inactive;
    SDL_Texture *brush_active;
    SDL_Texture *brush_inactive;
    SDL_Texture *volume_active;
    SDL_Texture *volume_inactive;

    /* control */
    SDL_Texture *green_flag;
    SDL_Texture *pause1;
    SDL_Texture *pause2;

    /* settings icons */
    SDL_Texture *fit_width;
    SDL_Texture *height_icon;
    SDL_Texture *vis_on_active;
    SDL_Texture *vis_on_inactive;
    SDL_Texture *vis_off_active;
    SDL_Texture *vis_off_inactive;

    /* motion icons */
    SDL_Texture *rotate_left;
    SDL_Texture *rotate_right;

    /* logo */
    SDL_Texture *logo;
    /* sprite panel icons */
    SDL_Texture *search_icon;
    SDL_Texture *surprise_icon;
    SDL_Texture *upload_icon;
    SDL_Texture *sprite_btn_icon;
    SDL_Texture *backdrop_btn_icon;
    SDL_Texture *scratch_cat;
    SDL_Texture *delete_sprite;

    /* NEW CLOUD TEXTURE */
    SDL_Texture *cloud;
};

/* Load all textures from assets/icons/. Returns false on critical failure. */
bool textures_load(Textures &tex, SDL_Renderer *r);

/* Destroy all textures */
void textures_free(Textures &tex);

#endif