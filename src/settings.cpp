#include "settings.h"
#include "config.h"
#include "renderer.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

static bool point_in_rect(int px, int py, const SDL_Rect &r) {
    return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
}

static void set_color(SDL_Renderer *r, Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255);
}

static void draw_text(SDL_Renderer *r, TTF_Font *f, const char *txt, int x, int y, Color c) {
    SDL_Color sc = { (Uint8)c.r, (Uint8)c.g, (Uint8)c.b, 255 };
    SDL_Surface *s = TTF_RenderUTF8_Blended(f, txt, sc);
    if (!s) return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
    SDL_Rect dst = { x, y, s->w, s->h };
    SDL_RenderCopy(r, t, NULL, &dst);
    SDL_DestroyTexture(t);
    SDL_FreeSurface(s);
}

static void draw_input_box(SDL_Renderer *r, TTF_Font *font, const SDL_Rect &rect, const char *text, bool active) {
    set_color(r, COL_SETTINGS_INPUT_BG);
    SDL_RenderFillRect(r, &rect);
    
    if (active) set_color(r, COL_SETTINGS_INPUT_ACTIVE);
    else set_color(r, COL_SETTINGS_INPUT_BORDER);
    
    SDL_RenderDrawRect(r, &rect);
    if (text[0] != '\0') {
        int tx = rect.x + 8;
        int ty = rect.y + (rect.h - 13) / 2;
        draw_text(r, font, text, tx, ty, COL_SETTINGS_INPUT_TEXT);
    }
}

void settings_layout(SettingsRects &rects) {
    int col_x = WINDOW_WIDTH - RIGHT_COLUMN_WIDTH;
    int col_h = WINDOW_HEIGHT - NAVBAR_HEIGHT;
    int stage_h = col_h * STAGE_HEIGHT_RATIO / 100;
    int top = NAVBAR_HEIGHT + stage_h;
    
    rects.panel.x = col_x;
    rects.panel.y = top;
    rects.panel.w = RIGHT_COLUMN_WIDTH;
    rects.panel.h = 80;

    int px = col_x + 10;
    int py = top + 10;

    /* ROW 1 */
    rects.sprite_name_label = {px, py, 45, SETTINGS_INPUT_H};
    rects.sprite_name_input = {px + 45, py, 110, SETTINGS_INPUT_H};
    
    int cur_x = rects.sprite_name_input.x + 130;
    rects.x_icon = {cur_x, py + 3, 20, 20};
    rects.x_input = {cur_x + 25, py, 60, SETTINGS_INPUT_H};
    
    cur_x += 100;
    rects.y_icon = {cur_x, py + 3, 20, 20};
    rects.y_input = {cur_x + 25, py, 60, SETTINGS_INPUT_H};

    /* ROW 2 */
    py += SETTINGS_INPUT_H + 10;
    px = col_x + 10;
    
    rects.show_label = {px, py, 40, SETTINGS_INPUT_H};
    rects.vis_on_btn = {px + 45, py, 35, SETTINGS_INPUT_H};
    rects.vis_off_btn = {px + 80, py, 35, SETTINGS_INPUT_H};
    
    cur_x = px + 140;
    rects.size_label = {cur_x, py, 35, SETTINGS_INPUT_H};
    rects.size_input = {cur_x + 35, py, 60, SETTINGS_INPUT_H};
    
    cur_x += 115;
    rects.dir_label_pos = {cur_x, py, 60, SETTINGS_INPUT_H};
    rects.dir_input = {cur_x + 60, py, 60, SETTINGS_INPUT_H};
}

void settings_draw(SDL_Renderer *r, TTF_Font *font, AppState &state,
                   const SettingsRects &rects, const Textures &tex) {
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderFillRect(r, &rects.panel);
    
    set_color(r, COL_SETTINGS_BORDER);
    SDL_RenderDrawLine(r, rects.panel.x, rects.panel.y, rects.panel.x + rects.panel.w, rects.panel.y);
    SDL_RenderDrawLine(r, rects.panel.x, rects.panel.y + rects.panel.h - 1, rects.panel.x + rects.panel.w, rects.panel.y + rects.panel.h - 1);

    int ty_offset = (SETTINGS_INPUT_H - 13) / 2;
    draw_text(r, font, "Sprite", rects.sprite_name_label.x, rects.sprite_name_label.y + ty_offset, COL_SETTINGS_TEXT);
    draw_text(r, font, "x", rects.x_icon.x, rects.x_icon.y, COL_SETTINGS_TEXT);
    draw_text(r, font, "y", rects.y_icon.x, rects.y_icon.y, COL_SETTINGS_TEXT);
    draw_text(r, font, "Show", rects.show_label.x, rects.show_label.y + ty_offset, COL_SETTINGS_TEXT);
    draw_text(r, font, "Size", rects.size_label.x, rects.size_label.y + ty_offset, COL_SETTINGS_TEXT);
    draw_text(r, font, "Direction", rects.dir_label_pos.x, rects.dir_label_pos.y + ty_offset, COL_SETTINGS_TEXT);

    std::string txt = (state.active_input == INPUT_SPRITE_NAME) ? state.input_buffer + "|" : state.sprite.name;
    draw_input_box(r, font, rects.sprite_name_input, txt.c_str(), state.active_input == INPUT_SPRITE_NAME);

    char buf[32];
    if (state.active_input == INPUT_X) {
        draw_input_box(r, font, rects.x_input, (state.input_buffer + "|").c_str(), true);
    } else {
        std::sprintf(buf, "%d", state.sprite.x);
        draw_input_box(r, font, rects.x_input, buf, false);
    }

    if (state.active_input == INPUT_Y) {
        draw_input_box(r, font, rects.y_input, (state.input_buffer + "|").c_str(), true);
    } else {
        std::sprintf(buf, "%d", state.sprite.y);
        draw_input_box(r, font, rects.y_input, buf, false);
    }

    if (state.active_input == INPUT_SIZE) {
        draw_input_box(r, font, rects.size_input, (state.input_buffer + "|").c_str(), true);
    } else {
        std::sprintf(buf, "%d", state.sprite.size);
        draw_input_box(r, font, rects.size_input, buf, false);
    }

    if (state.active_input == INPUT_DIRECTION) {
        draw_input_box(r, font, rects.dir_input, (state.input_buffer + "|").c_str(), true);
    } else {
        std::sprintf(buf, "%d", state.sprite.direction);
        draw_input_box(r, font, rects.dir_input, buf, false);
    }

    SDL_SetRenderDrawColor(r, 240, 240, 240, 255);
    SDL_RenderFillRect(r, &rects.vis_on_btn);
    SDL_RenderFillRect(r, &rects.vis_off_btn);
    
    SDL_Rect active_vis = state.sprite.visible ? rects.vis_on_btn : rects.vis_off_btn;
    SDL_SetRenderDrawColor(r, 215, 230, 255, 255);
    SDL_RenderFillRect(r, &active_vis);
    
    set_color(r, COL_SETTINGS_INPUT_BORDER);
    SDL_RenderDrawRect(r, &rects.vis_on_btn);
    SDL_RenderDrawRect(r, &rects.vis_off_btn);
    
    if (tex.vis_on_active) renderer_draw_texture_fit(r, tex.vis_on_active, &rects.vis_on_btn);
    if (tex.vis_off_inactive) renderer_draw_texture_fit(r, tex.vis_off_inactive, &rects.vis_off_btn);
}

static void commit_input(AppState &state) {
    if (state.active_input == INPUT_SPRITE_NAME) {
        if (!state.input_buffer.empty()) state.sprite.name = state.input_buffer;
    } else if (state.active_input == INPUT_X) {
        if (!state.input_buffer.empty() && state.input_buffer != "-")
            state.sprite.x = std::atoi(state.input_buffer.c_str());
    } else if (state.active_input == INPUT_Y) {
        if (!state.input_buffer.empty() && state.input_buffer != "-")
            state.sprite.y = std::atoi(state.input_buffer.c_str());
    } else if (state.active_input == INPUT_SIZE) {
        if (!state.input_buffer.empty()) {
            int val = std::atoi(state.input_buffer.c_str());
            if (val < 0) val = 0;
            if (val > 999) val = 999; /* Allow sizes up to 999 now */
            state.sprite.size = val;
        }
    } else if (state.active_input == INPUT_DIRECTION) {
        if (!state.input_buffer.empty() && state.input_buffer != "-")
            state.sprite.direction = std::atoi(state.input_buffer.c_str());
    }
    state.active_input = INPUT_NONE;
    state.input_buffer.clear();
}

bool settings_handle_event(const SDL_Event &e, AppState &state, const SettingsRects &rects) {
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x, my = e.button.y;

        if (state.active_input != INPUT_NONE && state.active_input != INPUT_PROJECT_NAME && state.active_input != INPUT_BLOCK_FIELD) {
            commit_input(state);
        }

        /* Notice we set state.input_buffer = "" (empty) instead of pre-filling it */
        if (point_in_rect(mx, my, rects.sprite_name_input)) {
            state.active_input = INPUT_SPRITE_NAME; state.input_buffer = ""; return true;
        }
        if (point_in_rect(mx, my, rects.x_input)) {
            state.active_input = INPUT_X; state.input_buffer = ""; return true;
        }
        if (point_in_rect(mx, my, rects.y_input)) {
            state.active_input = INPUT_Y; state.input_buffer = ""; return true;
        }
        if (point_in_rect(mx, my, rects.size_input)) {
            state.active_input = INPUT_SIZE; state.input_buffer = ""; return true;
        }
        if (point_in_rect(mx, my, rects.dir_input)) {
            state.active_input = INPUT_DIRECTION; state.input_buffer = ""; return true;
        }
        if (point_in_rect(mx, my, rects.vis_on_btn)) {
            state.sprite.visible = true; return true;
        }
        if (point_in_rect(mx, my, rects.vis_off_btn)) {
            state.sprite.visible = false; return true;
        }
        if (point_in_rect(mx, my, rects.panel)) return true;
    }

    if (state.active_input != INPUT_NONE && state.active_input != INPUT_PROJECT_NAME && state.active_input != INPUT_BLOCK_FIELD) {
        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) {
                commit_input(state); return true;
            }
            if (e.key.keysym.sym == SDLK_ESCAPE) {
                state.active_input = INPUT_NONE; state.input_buffer.clear(); return true;
            }
            if (e.key.keysym.sym == SDLK_BACKSPACE) {
                if (!state.input_buffer.empty()) state.input_buffer.pop_back();
                return true;
            }
        }
        if (e.type == SDL_TEXTINPUT) {
            state.input_buffer += e.text.text; return true;
        }
    }
    return false;
}