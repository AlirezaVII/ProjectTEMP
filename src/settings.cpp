#include "settings.h"
#include "config.h"
#include "renderer.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

static bool point_in_rect(int px, int py, const SDL_Rect &r) { return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h; }
static void set_color(SDL_Renderer *r, Color c) { SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255); }

static void draw_text(SDL_Renderer *r, TTF_Font *f, const char *txt, int x, int y, Color c)
{
    SDL_Color sc = {(Uint8)c.r, (Uint8)c.g, (Uint8)c.b, 255};
    SDL_Surface *s = TTF_RenderUTF8_Blended(f, txt, sc);
    if (!s)
        return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
    SDL_Rect dst = {x, y, s->w, s->h};
    SDL_RenderCopy(r, t, NULL, &dst);
    SDL_DestroyTexture(t);
    SDL_FreeSurface(s);
}

static void draw_input_box(SDL_Renderer *r, TTF_Font *font, const SDL_Rect &rect, const char *text, bool active)
{
    set_color(r, COL_SETTINGS_INPUT_BG);
    SDL_RenderFillRect(r, &rect);
    if (active)
        set_color(r, COL_SETTINGS_INPUT_ACTIVE);
    else
        set_color(r, COL_SETTINGS_INPUT_BORDER);
    SDL_RenderDrawRect(r, &rect);
    if (text[0] != '\0')
    {
        int tx = rect.x + 8;
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
    rects.panel = {col_x, top, RIGHT_COLUMN_WIDTH, 80};
    int px = col_x + 10, py = top + 10;

    rects.sprite_name_label = {px, py, 45, SETTINGS_INPUT_H};
    rects.sprite_name_input = {px + 45, py, 110, SETTINGS_INPUT_H};
    int cur_x = rects.sprite_name_input.x + 130;
    rects.x_icon = {cur_x, py + 3, 20, 20};
    rects.x_input = {cur_x + 25, py, 60, SETTINGS_INPUT_H};
    cur_x += 100;
    rects.y_icon = {cur_x, py + 3, 20, 20};
    rects.y_input = {cur_x + 25, py, 60, SETTINGS_INPUT_H};

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

void settings_draw(SDL_Renderer *r, TTF_Font *font, AppState &state, const SettingsRects &rects, const Textures &tex)
{
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderFillRect(r, &rects.panel);

    // ---> FIXED: SEPARATORS AND LEFT BORDER <---
    set_color(r, COL_SETTINGS_BORDER);
    SDL_RenderDrawLine(r, rects.panel.x, rects.panel.y, rects.panel.x + rects.panel.w, rects.panel.y);
    SDL_RenderDrawLine(r, rects.panel.x, rects.panel.y + rects.panel.h - 1, rects.panel.x + rects.panel.w, rects.panel.y + rects.panel.h - 1);
    SDL_SetRenderDrawColor(r, 220, 220, 220, 255);
    SDL_RenderDrawLine(r, rects.panel.x, rects.panel.y, rects.panel.x, rects.panel.y + rects.panel.h);

    int ty_offset = (SETTINGS_INPUT_H - 13) / 2;

    // ---> FIXED: DISABLE SETTINGS IF STAGE SELECTED <---
    if (state.editing_target_is_stage)
    {
        draw_text(r, font, "Stage selected: No motion settings", rects.sprite_name_label.x, rects.sprite_name_label.y + ty_offset, COL_SETTINGS_TEXT);
        return;
    }

    draw_text(r, font, "Sprite", rects.sprite_name_label.x, rects.sprite_name_label.y + ty_offset, COL_SETTINGS_TEXT);
    draw_text(r, font, "x", rects.x_icon.x, rects.x_icon.y, COL_SETTINGS_TEXT);
    draw_text(r, font, "y", rects.y_icon.x, rects.y_icon.y, COL_SETTINGS_TEXT);
    draw_text(r, font, "Show", rects.show_label.x, rects.show_label.y + ty_offset, COL_SETTINGS_TEXT);
    draw_text(r, font, "Size", rects.size_label.x, rects.size_label.y + ty_offset, COL_SETTINGS_TEXT);
    draw_text(r, font, "Direction", rects.dir_label_pos.x, rects.dir_label_pos.y + ty_offset, COL_SETTINGS_TEXT);

    Sprite *active_spr = (state.selected_sprite >= 0 && state.selected_sprite < (int)state.sprites.size()) ? &state.sprites[state.selected_sprite] : nullptr;

    std::string txt = (state.active_input == INPUT_SPRITE_NAME) ? state.input_buffer + "|" : (active_spr ? active_spr->name : "");
    draw_input_box(r, font, rects.sprite_name_input, txt.c_str(), state.active_input == INPUT_SPRITE_NAME);

    char buf[32];
    if (state.active_input == INPUT_X)
        draw_input_box(r, font, rects.x_input, (state.input_buffer + "|").c_str(), true);
    else
    {
        if (active_spr)
            std::snprintf(buf, sizeof(buf), "%d", active_spr->x);
        draw_input_box(r, font, rects.x_input, active_spr ? buf : "-", false);
    }

    if (state.active_input == INPUT_Y)
        draw_input_box(r, font, rects.y_input, (state.input_buffer + "|").c_str(), true);
    else
    {
        if (active_spr)
            std::snprintf(buf, sizeof(buf), "%d", active_spr->y);
        draw_input_box(r, font, rects.y_input, active_spr ? buf : "-", false);
    }

    if (state.active_input == INPUT_SIZE)
        draw_input_box(r, font, rects.size_input, (state.input_buffer + "|").c_str(), true);
    else
    {
        if (active_spr)
            std::snprintf(buf, sizeof(buf), "%d", active_spr->size);
        draw_input_box(r, font, rects.size_input, active_spr ? buf : "-", false);
    }

    if (state.active_input == INPUT_DIRECTION)
        draw_input_box(r, font, rects.dir_input, (state.input_buffer + "|").c_str(), true);
    else
    {
        if (active_spr)
            std::snprintf(buf, sizeof(buf), "%d", active_spr->direction);
        draw_input_box(r, font, rects.dir_input, active_spr ? buf : "-", false);
    }

    SDL_SetRenderDrawColor(r, 240, 240, 240, 255);
    SDL_RenderFillRect(r, &rects.vis_on_btn);
    SDL_RenderFillRect(r, &rects.vis_off_btn);
    bool is_vis = active_spr ? active_spr->visible : false;
    SDL_Rect active_vis = is_vis ? rects.vis_on_btn : rects.vis_off_btn;
    SDL_SetRenderDrawColor(r, 215, 230, 255, 255);
    SDL_RenderFillRect(r, &active_vis);

    set_color(r, COL_SETTINGS_INPUT_BORDER);
    SDL_RenderDrawRect(r, &rects.vis_on_btn);
    SDL_RenderDrawRect(r, &rects.vis_off_btn);
    if (tex.vis_on_active)
        renderer_draw_texture_fit(r, tex.vis_on_active, &rects.vis_on_btn);
    if (tex.vis_off_inactive)
        renderer_draw_texture_fit(r, tex.vis_off_inactive, &rects.vis_off_btn);
}

static void commit_input(AppState &state)
{
    if (state.selected_sprite < 0 || state.selected_sprite >= (int)state.sprites.size())
    {
        state.active_input = INPUT_NONE;
        state.input_buffer.clear();
        return;
    }
    Sprite &spr = state.sprites[state.selected_sprite];
    if (state.active_input == INPUT_SPRITE_NAME)
    {
        if (!state.input_buffer.empty())
            spr.name = state.input_buffer;
    }
    else if (state.active_input == INPUT_X)
    {
        if (!state.input_buffer.empty() && state.input_buffer != "-")
            spr.x = std::atoi(state.input_buffer.c_str());
    }
    else if (state.active_input == INPUT_Y)
    {
        if (!state.input_buffer.empty() && state.input_buffer != "-")
            spr.y = std::atoi(state.input_buffer.c_str());
    }
    else if (state.active_input == INPUT_SIZE)
    {
        if (!state.input_buffer.empty())
        {
            int val = std::atoi(state.input_buffer.c_str());
            spr.size = std::max(0, std::min(val, 999));
        }
    }
    else if (state.active_input == INPUT_DIRECTION)
    {
        if (!state.input_buffer.empty() && state.input_buffer != "-")
            spr.direction = std::atoi(state.input_buffer.c_str());
    }
    state.active_input = INPUT_NONE;
    state.input_buffer.clear();
}

bool settings_handle_event(const SDL_Event &e, AppState &state, const SettingsRects &rects)
{
    if (state.editing_target_is_stage)
        return false; // Ignore clicks if stage selected

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        int mx = e.button.x, my = e.button.y;
        if (state.active_input != INPUT_NONE && state.active_input != INPUT_PROJECT_NAME && state.active_input != INPUT_BLOCK_FIELD && state.active_input != INPUT_COSTUME_NAME)
            commit_input(state);

        if (point_in_rect(mx, my, rects.sprite_name_input))
        {
            state.active_input = INPUT_SPRITE_NAME;
            state.input_buffer = "";
            return true;
        }
        if (point_in_rect(mx, my, rects.x_input))
        {
            state.active_input = INPUT_X;
            state.input_buffer = "";
            return true;
        }
        if (point_in_rect(mx, my, rects.y_input))
        {
            state.active_input = INPUT_Y;
            state.input_buffer = "";
            return true;
        }
        if (point_in_rect(mx, my, rects.size_input))
        {
            state.active_input = INPUT_SIZE;
            state.input_buffer = "";
            return true;
        }
        if (point_in_rect(mx, my, rects.dir_input))
        {
            state.active_input = INPUT_DIRECTION;
            state.input_buffer = "";
            return true;
        }

        if (point_in_rect(mx, my, rects.vis_on_btn))
        {
            if (state.selected_sprite >= 0 && state.selected_sprite < (int)state.sprites.size())
                state.sprites[state.selected_sprite].visible = true;
            return true;
        }
        if (point_in_rect(mx, my, rects.vis_off_btn))
        {
            if (state.selected_sprite >= 0 && state.selected_sprite < (int)state.sprites.size())
                state.sprites[state.selected_sprite].visible = false;
            return true;
        }
        if (point_in_rect(mx, my, rects.panel))
            return true;
    }

    if (state.active_input != INPUT_NONE && state.active_input != INPUT_PROJECT_NAME && state.active_input != INPUT_BLOCK_FIELD && state.active_input != INPUT_COSTUME_NAME)
    {
        if (e.type == SDL_KEYDOWN)
        {
            if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER)
            {
                commit_input(state);
                return true;
            }
            if (e.key.keysym.sym == SDLK_ESCAPE)
            {
                state.active_input = INPUT_NONE;
                state.input_buffer.clear();
                return true;
            }
            if (e.key.keysym.sym == SDLK_BACKSPACE)
            {
                if (!state.input_buffer.empty())
                    state.input_buffer.pop_back();
                return true;
            }
        }
        if (e.type == SDL_TEXTINPUT)
        {
            state.input_buffer += e.text.text;
            return true;
        }
    }
    return false;
}