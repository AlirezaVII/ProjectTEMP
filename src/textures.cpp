#include "textures.h"
#include "renderer.h"
#include <cstring>
#include <string>

static SDL_Texture* load(SDL_Renderer *r, const char *filename) {
    std::string path = "assets/icons/";
    path += filename;
    return renderer_load_texture(r, path.c_str());
}

bool textures_load(Textures &tex, SDL_Renderer *r) {
    std::memset(&tex, 0, sizeof(tex));
    tex.code_active = load(r, "code_active.png");
    tex.code_inactive = load(r, "code_inactive.png");
    tex.volume_active = load(r, "volume_active.png");
    tex.volume_inactive = load(r, "volume_inactive.png");

    tex.green_flag = load(r, "green_flag.png");
    tex.pause1 = load(r, "pause1.png");
    tex.pause2 = load(r, "pause2.png");

    tex.fit_width = load(r, "fit_width.png");
    tex.height_icon = load(r, "height.png");
    tex.vis_on_active = load(r, "visibility_on_active.png");
    tex.vis_on_inactive = load(r, "visibility_on_inactive.png");
    tex.vis_off_active = load(r, "visibility_off_active.png");
    tex.vis_off_inactive = load(r, "visibility_off_inactive.png");
    tex.rotate_left = load(r, "rotate_left.png");
    tex.rotate_right = load(r, "rotate_right.png");

    tex.logo = load(r, "logo.png");
    tex.search_icon = load(r, "search_icon.png");
    tex.surprise_icon = load(r, "surprise_icon.png");
    tex.upload_icon = load(r, "upload_icon.png");
    tex.sprite_btn_icon = load(r, "sprite_btn_icon.png");
    tex.backdrop_btn_icon = load(r, "backdrop_btn_icon.png");
    
    tex.scratch_cat = renderer_load_texture(r, "assets/sprites/scratch_cat.png");
    tex.delete_sprite = load(r, "delete_icon.png");

    tex.cloud = load(r, "cloud.png"); 
    tex.vol_up = load(r, "volume_up.png");
    tex.vol_down = load(r, "volume_down.png");
    tex.vol_mute = load(r, "volume_mute.png");
    tex.play_icon = load(r, "play_icon.png");

    // ---> NEW: EDITOR ICONS <---
    tex.mouse_active = load(r, "mouse_active.png");
    tex.brush_active = load(r, "brush_active.png");
    tex.brush_nonactive = load(r, "brush_nonactive.png");
    tex.brush_tab_active = load(r, "brush_tab_active.png");
    tex.mouse_nonactive = load(r, "mouse_nonactive.png");
    
    tex.eraser_active = load(r, "eraser_active.png");
    tex.eraser_nonactive = load(r, "eraser_nonactive.png");
    tex.text_active = load(r, "text_active.png");
    tex.text_nonactive = load(r, "text_nonactive.png");
    tex.fill_active = load(r, "fill_active.png");
    tex.fill_nonactive = load(r, "fill_nonactive.png");
    tex.rect_active = load(r, "rectangle_active.png");
    tex.rect_nonactive = load(r, "rectangle_nonactive.png");
    tex.circ_active = load(r, "circle_active.png");
    tex.circ_nonactive = load(r, "circle_nonactive.png");
    tex.trash_active = load(r, "trash_active.png");
    tex.trash_nonactive = load(r, "trash_nonactive.png");
    tex.flip_h = load(r, "flip_horizontal.png");
    tex.flip_v = load(r, "flip_vertical.png");

    return true;
}

void textures_free(Textures &tex) {
    if (tex.code_active) SDL_DestroyTexture(tex.code_active);
    if (tex.code_inactive) SDL_DestroyTexture(tex.code_inactive);
    if (tex.brush_active) SDL_DestroyTexture(tex.brush_active);
    if (tex.brush_nonactive) SDL_DestroyTexture(tex.brush_nonactive);
    if (tex.volume_active) SDL_DestroyTexture(tex.volume_active);
    if (tex.volume_inactive) SDL_DestroyTexture(tex.volume_inactive);
    if (tex.green_flag) SDL_DestroyTexture(tex.green_flag);
    if (tex.pause1) SDL_DestroyTexture(tex.pause1);
    if (tex.pause2) SDL_DestroyTexture(tex.pause2);
    if (tex.fit_width) SDL_DestroyTexture(tex.fit_width);
    if (tex.height_icon) SDL_DestroyTexture(tex.height_icon);
    if (tex.vis_on_active) SDL_DestroyTexture(tex.vis_on_active);
    if (tex.vis_on_inactive) SDL_DestroyTexture(tex.vis_on_inactive);
    if (tex.vis_off_active) SDL_DestroyTexture(tex.vis_off_active);
    if (tex.vis_off_inactive) SDL_DestroyTexture(tex.vis_off_inactive);
    if (tex.rotate_left) SDL_DestroyTexture(tex.rotate_left);
    if (tex.rotate_right) SDL_DestroyTexture(tex.rotate_right);
    if (tex.logo) SDL_DestroyTexture(tex.logo);
    if (tex.search_icon) SDL_DestroyTexture(tex.search_icon);
    if (tex.surprise_icon) SDL_DestroyTexture(tex.surprise_icon);
    if (tex.upload_icon) SDL_DestroyTexture(tex.upload_icon);
    if (tex.sprite_btn_icon) SDL_DestroyTexture(tex.sprite_btn_icon);
    if (tex.backdrop_btn_icon) SDL_DestroyTexture(tex.backdrop_btn_icon);
    if (tex.scratch_cat) SDL_DestroyTexture(tex.scratch_cat);
    if (tex.delete_sprite) SDL_DestroyTexture(tex.delete_sprite);
    if (tex.cloud) SDL_DestroyTexture(tex.cloud);
    if (tex.vol_up) SDL_DestroyTexture(tex.vol_up);
    if (tex.vol_down) SDL_DestroyTexture(tex.vol_down);
    if (tex.vol_mute) SDL_DestroyTexture(tex.vol_mute);
    if (tex.play_icon) SDL_DestroyTexture(tex.play_icon);
    
    // Clean up editor icons
    if (tex.mouse_active) SDL_DestroyTexture(tex.mouse_active);
    if (tex.mouse_nonactive) SDL_DestroyTexture(tex.mouse_nonactive);
    if (tex.eraser_active) SDL_DestroyTexture(tex.eraser_active);
    if (tex.eraser_nonactive) SDL_DestroyTexture(tex.eraser_nonactive);
    if (tex.text_active) SDL_DestroyTexture(tex.text_active);
    if (tex.text_nonactive) SDL_DestroyTexture(tex.text_nonactive);
    if (tex.fill_active) SDL_DestroyTexture(tex.fill_active);
    if (tex.fill_nonactive) SDL_DestroyTexture(tex.fill_nonactive);
    if (tex.rect_active) SDL_DestroyTexture(tex.rect_active);
    if (tex.rect_nonactive) SDL_DestroyTexture(tex.rect_nonactive);
    if (tex.circ_active) SDL_DestroyTexture(tex.circ_active);
    if (tex.circ_nonactive) SDL_DestroyTexture(tex.circ_nonactive);
    if (tex.trash_active) SDL_DestroyTexture(tex.trash_active);
    if (tex.trash_nonactive) SDL_DestroyTexture(tex.trash_nonactive);
    if (tex.flip_h) SDL_DestroyTexture(tex.flip_h);
    if (tex.flip_v) SDL_DestroyTexture(tex.flip_v);
}