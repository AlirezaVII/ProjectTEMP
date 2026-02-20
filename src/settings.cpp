#include "settings.h"
#include "config.h"
#include "renderer.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

static bool point_in_rect(int px, int py, const SDL_Rect &r)
{
    return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
}

static void set_color(SDL_Renderer *r, Color c)
{
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255);
}

static void draw_text(SDL_Renderer *r, TTF_Font *f, const char *txt,
                      int x, int y, Color c)
{
    SDL_Color sc;
    sc.r = c.r; sc.g = c.g; sc.b = c.b; sc.a = 255;
    SDL_Surface *s = TTF_RenderUTF8_Blended(f, txt, sc);
    if (!s) return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
    SDL_Rect dst;
    dst.x = x; dst.y = y; dst.w = s->w; dst.h = s->h;
    SDL_RenderCopy(r, t, NULL, &dst);
    SDL_DestroyTexture(t);
    SDL_FreeSurface(s);
}

static void draw_input_box(SDL_Renderer *r, TTF_Font *font,
                           const SDL_Rect &rect, const char *text, bool active)
{
    set_color(r, COL_SETTINGS_INPUT_BG);
    SDL_RenderFillRect(r, &rect);
    if (active) {
        set_color(r, COL_SETTINGS_INPUT_ACTIVE);
    } else {
        set_color(r, COL_SETTINGS_INPUT_BORDER);
    }
    SDL_RenderDrawRect(r, &rect);
    if (text[0] != '\0') {
        int tx = rect.x + 4;
        int ty = rect.y + (rect.h - 13) / 2;
        draw_text(r, font, text, tx, ty, COL_SETTINGS_INPUT_TEXT);
    }
}

void settings_layout(SettingsRects &rects)
{
    int col_x = WINDOW_WIDTH - RIGHT_COLUMN_WIDTH;
    int col_h = WINDOW_HEIGHT - NAVBAR_HEIGHT;
    int stage_h = col_h * STAGE_HEIGHT_RATIO / 100;
    int top = NAVBAR_HEIGHT + stage_h;
    int panel_h = WINDOW_HEIGHT - top;

    rects.panel.x = col_x;
    rects.panel.y = top;
    rects.panel.w = RIGHT_COLUMN_WIDTH;
    rects.panel.h = panel_h;

    int px = col_x + 10;
    int py = top + 40;
    int icon_sz = 22;

    /* sprite name input - full width */
    rects.sprite_name_input.x = px;
    rects.sprite_name_input.y = py;
    rects.sprite_name_input.w = RIGHT_COLUMN_WIDTH - 20;
    rects.sprite_name_input.h = SETTINGS_INPUT_H;
    py += SETTINGS_INPUT_H + 14;

    /* Row: [fit_width] [x_input]  [height] [y_input]  dir: [dir_input] [eye] */
    int cur_x = px;

    rects.x_icon.x = cur_x;
    rects.x_icon.y = py + (SETTINGS_INPUT_H - icon_sz) / 2;
    rects.x_icon.w = icon_sz;
    rects.x_icon.h = icon_sz;
    cur_x += icon_sz + 30;

    rects.x_input.x = cur_x;
    rects.x_input.y = py;
    rects.x_input.w = SETTINGS_INPUT_W;
    rects.x_input.h = SETTINGS_INPUT_H;
    cur_x += SETTINGS_INPUT_W + 10;

    rects.y_icon.x = cur_x;
    rects.y_icon.y = py + (SETTINGS_INPUT_H - icon_sz) / 2;
    rects.y_icon.w = icon_sz;
    rects.y_icon.h = icon_sz;
    cur_x += icon_sz + 4;

    rects.y_input.x = cur_x + 30;
    rects.y_input.y = py;
    rects.y_input.w = SETTINGS_INPUT_W;
    rects.y_input.h = SETTINGS_INPUT_H;
    cur_x += SETTINGS_INPUT_W + 14;

    /* dir label position (just for reference) */
    rects.dir_label_pos.x = cur_x + 30;
    rects.dir_label_pos.y = py;
    rects.dir_label_pos.w = 24;
    rects.dir_label_pos.h = SETTINGS_INPUT_H;
    cur_x += 28;

    rects.dir_input.x = cur_x + 30;
    rects.dir_input.y = py;
    rects.dir_input.w = SETTINGS_INPUT_W;
    rects.dir_input.h = SETTINGS_INPUT_H;
    cur_x += SETTINGS_INPUT_W + 10;

    rects.vis_icon.x = cur_x + 30;
    rects.vis_icon.y = py + (SETTINGS_INPUT_H - VIS_ICON_SIZE) / 2;
    rects.vis_icon.w = VIS_ICON_SIZE;
    rects.vis_icon.h = VIS_ICON_SIZE;
}

void settings_draw(SDL_Renderer *r, TTF_Font *font, AppState &state,
                   const SettingsRects &rects, const Textures &tex)
{
    /* panel bg */
    set_color(r, COL_SETTINGS_BG);
    SDL_RenderFillRect(r, &rects.panel);

    /* top border */
    set_color(r, COL_SETTINGS_BORDER);
    SDL_RenderDrawLine(r, rects.panel.x, rects.panel.y,
                       rects.panel.x + rects.panel.w, rects.panel.y);

    /* sprite label */
    draw_text(r, font, "Sprite",
              rects.sprite_name_input.x,
              rects.sprite_name_input.y - 20,
              COL_SETTINGS_TEXT);

    /* sprite name input */
    {
        std::string txt;
        if (state.active_input == INPUT_SPRITE_NAME) {
            txt = state.input_buffer + "|";
        } else {
            txt = state.sprite.name;
        }
        draw_input_box(r, font, rects.sprite_name_input, txt.c_str(),
                       state.active_input == INPUT_SPRITE_NAME);
    }

    /* x icon */
    renderer_draw_texture_fit(r, tex.fit_width, &rects.x_icon);
    draw_text(r, font, "x :", rects.x_icon.x + 28, rects.x_icon.y + 3, COL_SETTINGS_TEXT);

    /* x input */
    {
        char buf[32];
        if (state.active_input == INPUT_X) {
            std::string t = state.input_buffer + "|";
            draw_input_box(r, font, rects.x_input, t.c_str(), true);
        } else {
            std::sprintf(buf, "%d", state.sprite.x);
            draw_input_box(r, font, rects.x_input, buf, false);
        }
    }

    /* y icon */
    renderer_draw_texture_fit(r, tex.height_icon, &rects.y_icon);
    draw_text(r, font, "y :", rects.y_icon.x + 28, rects.y_icon.y + 3, COL_SETTINGS_TEXT);

    /* y input */
    {
        char buf[32];
        if (state.active_input == INPUT_Y) {
            std::string t = state.input_buffer + "|";
            draw_input_box(r, font, rects.y_input, t.c_str(), true);
        } else {
            std::sprintf(buf, "%d", state.sprite.y);
            draw_input_box(r, font, rects.y_input, buf, false);
        }
    }

    /* dir label */
    draw_text(r, font, "dir :",
              rects.dir_label_pos.x,
              rects.dir_label_pos.y + (SETTINGS_INPUT_H - 13) / 2,
              COL_SETTINGS_TEXT);

    /* dir input */
    {
        char buf[32];
        if (state.active_input == INPUT_DIRECTION) {
            std::string t = state.input_buffer + "|";
            draw_input_box(r, font, rects.dir_input, t.c_str(), true);
        } else {
            std::sprintf(buf, "%d", state.sprite.direction);
            draw_input_box(r, font, rects.dir_input, buf, false);
        }
    }

    /* visibility icon */
    {
        SDL_Texture *vis_tex = NULL;
        if (state.sprite.visible) {
            vis_tex = tex.vis_on_active;
            if (!vis_tex) vis_tex = tex.vis_on_inactive;
        } else {
            vis_tex = tex.vis_off_active;
            if (!vis_tex) vis_tex = tex.vis_off_inactive;
        }
        if (vis_tex) {
            renderer_draw_texture_fit(r, vis_tex, &rects.vis_icon);
        } else {
            /* fallback circles */
            int cx = rects.vis_icon.x + rects.vis_icon.w / 2;
            int cy = rects.vis_icon.y + rects.vis_icon.h / 2;
            if (state.sprite.visible) {
                renderer_fill_circle(r, cx, cy, 10, 95, 149, 247);
                renderer_fill_circle(r, cx, cy, 4, 255, 255, 255);
            } else {
                renderer_fill_circle(r, cx, cy, 10, 160, 160, 160);
                renderer_fill_circle(r, cx, cy, 4, 200, 200, 200);
                SDL_SetRenderDrawColor(r, 200, 60, 60, 255);
                SDL_RenderDrawLine(r, cx - 10, cy + 10, cx + 10, cy - 10);
            }
        }
    }
}

static void commit_input(AppState &state)
{
    if (state.active_input == INPUT_SPRITE_NAME) {
        if (!state.input_buffer.empty()) {
            state.sprite.name = state.input_buffer;
        }
    } else if (state.active_input == INPUT_X) {
        state.sprite.x = std::atoi(state.input_buffer.c_str());
    } else if (state.active_input == INPUT_Y) {
        state.sprite.y = std::atoi(state.input_buffer.c_str());
    } else if (state.active_input == INPUT_DIRECTION) {
        state.sprite.direction = std::atoi(state.input_buffer.c_str());
    }
    state.active_input = INPUT_NONE;
    state.input_buffer.clear();
}

bool settings_handle_event(const SDL_Event &e, AppState &state,
                           const SettingsRects &rects)
{
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x, my = e.button.y;

        /*
            IMPORTANT:
            If workspace block field is active, Settings must NOT commit/steal it.
        */
        if (state.active_input != INPUT_NONE &&
            state.active_input != INPUT_PROJECT_NAME &&
            state.active_input != INPUT_BLOCK_FIELD) {
            commit_input(state);
        }

        if (point_in_rect(mx, my, rects.sprite_name_input)) {
            state.active_input = INPUT_SPRITE_NAME;
            state.input_buffer = state.sprite.name;
            return true;
        }
        if (point_in_rect(mx, my, rects.x_input)) {
            state.active_input = INPUT_X;
            char buf[32]; std::sprintf(buf, "%d", state.sprite.x);
            state.input_buffer = buf;
            return true;
        }
        if (point_in_rect(mx, my, rects.y_input)) {
            state.active_input = INPUT_Y;
            char buf[32]; std::sprintf(buf, "%d", state.sprite.y);
            state.input_buffer = buf;
            return true;
        }
        if (point_in_rect(mx, my, rects.dir_input)) {
            state.active_input = INPUT_DIRECTION;
            char buf[32]; std::sprintf(buf, "%d", state.sprite.direction);
            state.input_buffer = buf;
            return true;
        }
        if (point_in_rect(mx, my, rects.vis_icon)) {
            state.sprite.visible = !state.sprite.visible;
            return true;
        }
        if (point_in_rect(mx, my, rects.panel)) {
            return true;
        }
    }

    /*
        keyboard for settings inputs:
        do NOT handle when INPUT_BLOCK_FIELD (let workspace handle it).
    */
    if (state.active_input != INPUT_NONE &&
        state.active_input != INPUT_PROJECT_NAME &&
        state.active_input != INPUT_BLOCK_FIELD)
    {
        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_RETURN ||
                e.key.keysym.sym == SDLK_KP_ENTER) {
                commit_input(state);
                return true;
            }
            if (e.key.keysym.sym == SDLK_ESCAPE) {
                state.active_input = INPUT_NONE;
                state.input_buffer.clear();
                return true;
            }
            if (e.key.keysym.sym == SDLK_BACKSPACE) {
                if (!state.input_buffer.empty()) {
                    state.input_buffer.erase(state.input_buffer.size() - 1);
                }
                return true;
            }
        }
        if (e.type == SDL_TEXTINPUT) {
            state.input_buffer += e.text.text;
            return true;
        }
    }

    return false;
}