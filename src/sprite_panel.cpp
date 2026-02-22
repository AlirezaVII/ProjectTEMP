#include "sprite_panel.h"
#include "config.h"
#include <cstdio>
#include <cmath>
#include <string>

static void sp_draw_text_centered(SDL_Renderer *r, TTF_Font *font, const char *txt, int cx, int cy, Uint8 cr, Uint8 cg, Uint8 cb)
{
    SDL_Color col = {cr, cg, cb, 255};
    SDL_Surface *s = TTF_RenderUTF8_Blended(font, txt, col);
    if (!s)
        return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
    SDL_Rect dst = {cx - s->w / 2, cy - s->h / 2, s->w, s->h};
    SDL_RenderCopy(r, t, NULL, &dst);
    SDL_DestroyTexture(t);
    SDL_FreeSurface(s);
}

static bool sp_point_in(const SDL_Rect &r, int x, int y) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; }
static void sp_fill_rect(SDL_Renderer *r, const SDL_Rect &rc, Uint8 cr, Uint8 cg, Uint8 cb, Uint8 ca)
{
    SDL_SetRenderDrawColor(r, cr, cg, cb, ca);
    SDL_RenderFillRect(r, &rc);
}
static void sp_draw_circle(SDL_Renderer *r, int cx, int cy, int rad, Uint8 cr, Uint8 cg, Uint8 cb)
{
    SDL_SetRenderDrawColor(r, cr, cg, cb, 255);
    for (int dy = -rad; dy <= rad; dy++)
    {
        int dx = (int)std::sqrt((double)(rad * rad - dy * dy));
        SDL_RenderDrawLine(r, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

static const int SETTINGS_PANEL_H = 80;
static const int BACKDROP_W = 90;
static const int BTN_RAD = 20;
static const int MENU_ITEM_SZ = 36;
static const int MENU_PAD = 4;
static const int MENU_W = MENU_ITEM_SZ + MENU_PAD * 2;
static const int THUMB_W = 64, THUMB_H = 64;

void sprite_panel_layout(SpritePanelRects &rects)
{
    int rc_x = WINDOW_WIDTH - RIGHT_COLUMN_WIDTH, col_h = WINDOW_HEIGHT - NAVBAR_HEIGHT;
    int stage_h = col_h * STAGE_HEIGHT_RATIO / 100, panel_top = NAVBAR_HEIGHT + stage_h + SETTINGS_PANEL_H;
    int panel_h = WINDOW_HEIGHT - panel_top;

    rects.backdrop_area = {rc_x + RIGHT_COLUMN_WIDTH - BACKDROP_W, panel_top, BACKDROP_W, panel_h};
    rects.backdrop_thumb = {rects.backdrop_area.x + (BACKDROP_W - 64) / 2, rects.backdrop_area.y + 30, 64, 48};
    rects.backdrop_label = {rects.backdrop_area.x, rects.backdrop_thumb.y + 48, BACKDROP_W, 30};
    rects.backdrop_btn = {rects.backdrop_area.x + BACKDROP_W - BTN_RAD * 2 - 6, rects.backdrop_area.y + panel_h - BTN_RAD * 2 - 6, BTN_RAD * 2, BTN_RAD * 2};

    int menu_h = 3 * (MENU_ITEM_SZ + MENU_PAD) + MENU_PAD;
    rects.backdrop_menu = {rects.backdrop_btn.x + (BTN_RAD * 2 - MENU_W) / 2, rects.backdrop_btn.y - menu_h, MENU_W, menu_h};
    for (int i = 0; i < 3; i++)
        rects.backdrop_menu_items[i] = {rects.backdrop_menu.x + MENU_PAD, rects.backdrop_menu.y + MENU_PAD + i * (MENU_ITEM_SZ + MENU_PAD), MENU_ITEM_SZ, MENU_ITEM_SZ};

    rects.sprite_list_area = {rc_x, panel_top, RIGHT_COLUMN_WIDTH - BACKDROP_W, panel_h};
    rects.sprite_btn = {rects.sprite_list_area.x + rects.sprite_list_area.w - BTN_RAD * 2 - 6, rects.sprite_list_area.y + panel_h - BTN_RAD * 2 - 6, BTN_RAD * 2, BTN_RAD * 2};

    rects.sprite_menu = {rects.sprite_btn.x + (BTN_RAD * 2 - MENU_W) / 2, rects.sprite_btn.y - menu_h, MENU_W, menu_h};
    for (int i = 0; i < 3; i++)
        rects.sprite_menu_items[i] = {rects.sprite_menu.x + MENU_PAD, rects.sprite_menu.y + MENU_PAD + i * (MENU_ITEM_SZ + MENU_PAD), MENU_ITEM_SZ, MENU_ITEM_SZ};
}

void sprite_panel_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state, const Textures &tex, const SpritePanelRects &rects)
{
    sp_fill_rect(r, rects.sprite_list_area, 233, 238, 242, 255);
    SDL_SetRenderDrawColor(r, 220, 220, 220, 255);
    SDL_RenderDrawLine(r, rects.backdrop_area.x, rects.backdrop_area.y, rects.backdrop_area.x, rects.backdrop_area.y + rects.backdrop_area.h);
    SDL_RenderDrawLine(r, rects.sprite_list_area.x, rects.sprite_list_area.y, rects.sprite_list_area.x + rects.sprite_list_area.w + rects.backdrop_area.w, rects.sprite_list_area.y);
    SDL_RenderDrawLine(r, rects.sprite_list_area.x, rects.sprite_list_area.y, rects.sprite_list_area.x, rects.sprite_list_area.y + rects.sprite_list_area.h);

    int grid_x = rects.sprite_list_area.x + 12, grid_y = rects.sprite_list_area.y + 12;
    for (size_t i = 0; i < state.sprites.size(); i++)
    {
        SDL_Rect thumb = {grid_x, grid_y, THUMB_W, THUMB_H};

        if (!state.editing_target_is_stage && (int)i == state.selected_sprite)
        {
            SDL_Rect border = {thumb.x - 3, thumb.y - 3, thumb.w + 6, thumb.h + 22};
            sp_fill_rect(r, border, 77, 151, 255, 255);
            SDL_Rect inner = {thumb.x - 2, thumb.y - 2, thumb.w + 4, thumb.h + 4};
            sp_fill_rect(r, inner, 255, 255, 255, 255);

            int cx = border.x + border.w - 10;
            int cy = border.y + 2;
            sp_draw_circle(r, cx, cy, 12, 255, 60, 60);
            SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
            int d = 4;
            for (int w = -1; w <= 1; w++)
            {
                SDL_RenderDrawLine(r, cx - d + w, cy - d, cx + d + w, cy + d);
                SDL_RenderDrawLine(r, cx - d + w, cy + d, cx + d + w, cy - d);
            }

            if (state.sprites[i].texture)
                SDL_RenderCopy(r, state.sprites[i].texture, NULL, &thumb);
            sp_draw_text_centered(r, font, state.sprites[i].name.c_str(), thumb.x + thumb.w / 2, thumb.y + thumb.h + 8, 255, 255, 255);
        }
        else
        {
            SDL_Rect inner = {thumb.x - 2, thumb.y - 2, thumb.w + 4, thumb.h + 4};
            sp_fill_rect(r, inner, 255, 255, 255, 255);
            SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
            SDL_RenderDrawRect(r, &inner);
            if (state.sprites[i].texture)
                SDL_RenderCopy(r, state.sprites[i].texture, NULL, &thumb);
            sp_draw_text_centered(r, font, state.sprites[i].name.c_str(), thumb.x + thumb.w / 2, thumb.y + thumb.h + 8, 80, 80, 80);
        }

        grid_x += THUMB_W + 15;
        if (grid_x + THUMB_W > rects.sprite_list_area.x + rects.sprite_list_area.w - 60)
        {
            grid_x = rects.sprite_list_area.x + 12;
            grid_y += THUMB_H + 25;
        }
    }

    int cx = rects.sprite_btn.x + BTN_RAD, cy = rects.sprite_btn.y + BTN_RAD;
    sp_draw_circle(r, cx, cy, BTN_RAD, 77, 151, 255);
    if (tex.sprite_btn_icon)
    {
        SDL_Rect ir = {cx - 12, cy - 12, 24, 24};
        SDL_RenderCopy(r, tex.sprite_btn_icon, NULL, &ir);
    }
    else
        sp_draw_text_centered(r, font, "+", cx, cy, 255, 255, 255);

    if (state.sprite_menu_open)
    {
        sp_fill_rect(r, rects.sprite_menu, 77, 151, 255, 255);
        SDL_Texture *icons[3] = {tex.upload_icon, tex.surprise_icon, tex.search_icon};
        for (int i = 0; i < 3; i++)
        {
            int icx = rects.sprite_menu_items[i].x + MENU_ITEM_SZ / 2, icy = rects.sprite_menu_items[i].y + MENU_ITEM_SZ / 2;
            sp_draw_circle(r, icx, icy, MENU_ITEM_SZ / 2 - 2, 255, 255, 255);
            if (icons[i])
            {
                SDL_Rect ir = {icx - 10, icy - 10, 20, 20};
                SDL_RenderCopy(r, icons[i], NULL, &ir);
            }
        }
    }

    sp_fill_rect(r, rects.backdrop_area, 255, 255, 255, 255);
    sp_draw_text_centered(r, font, "Stage", rects.backdrop_area.x + rects.backdrop_area.w / 2, rects.backdrop_area.y + 16, 80, 80, 80);

    if (state.editing_target_is_stage)
    {
        SDL_Rect border = {rects.backdrop_thumb.x - 4, rects.backdrop_thumb.y - 4, rects.backdrop_thumb.w + 8, rects.backdrop_thumb.h + 8};
        sp_fill_rect(r, border, 77, 151, 255, 255);
    }

    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderFillRect(r, const_cast<SDL_Rect *>(&rects.backdrop_thumb));

    if (state.selected_backdrop >= 0 && state.selected_backdrop < (int)state.backdrops.size())
    {
        if (state.backdrops[state.selected_backdrop].texture)
            SDL_RenderCopy(r, state.backdrops[state.selected_backdrop].texture, NULL, const_cast<SDL_Rect *>(&rects.backdrop_thumb));
        sp_draw_text_centered(r, font, "Backdrops", rects.backdrop_area.x + rects.backdrop_area.w / 2, rects.backdrop_label.y + 12, 100, 100, 100);
        sp_draw_text_centered(r, font, state.backdrops[state.selected_backdrop].name.c_str(), rects.backdrop_area.x + rects.backdrop_area.w / 2, rects.backdrop_label.y + 30, 140, 140, 140);
    }
    else
    {
        sp_draw_text_centered(r, font, "Backdrops", rects.backdrop_area.x + rects.backdrop_area.w / 2, rects.backdrop_label.y + 12, 100, 100, 100);
        sp_draw_text_centered(r, font, "1", rects.backdrop_area.x + rects.backdrop_area.w / 2, rects.backdrop_label.y + 30, 140, 140, 140);
    }

    SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
    SDL_RenderDrawRect(r, const_cast<SDL_Rect *>(&rects.backdrop_thumb));

    cx = rects.backdrop_btn.x + BTN_RAD;
    cy = rects.backdrop_btn.y + BTN_RAD;
    sp_draw_circle(r, cx, cy, BTN_RAD, 77, 151, 255);
    if (tex.backdrop_btn_icon)
    {
        SDL_Rect ir = {cx - 12, cy - 12, 24, 24};
        SDL_RenderCopy(r, tex.backdrop_btn_icon, NULL, &ir);
    }
    else
        sp_draw_text_centered(r, font, "+", cx, cy, 255, 255, 255);

    if (state.backdrop_menu_open)
    {
        sp_fill_rect(r, rects.backdrop_menu, 77, 151, 255, 255);
        SDL_Texture *icons[3] = {tex.upload_icon, tex.surprise_icon, tex.search_icon};
        for (int i = 0; i < 3; i++)
        {
            int icx = rects.backdrop_menu_items[i].x + MENU_ITEM_SZ / 2, icy = rects.backdrop_menu_items[i].y + MENU_ITEM_SZ / 2;
            sp_draw_circle(r, icx, icy, MENU_ITEM_SZ / 2 - 2, 255, 255, 255);
            if (icons[i])
            {
                SDL_Rect ir = {icx - 10, icy - 10, 20, 20};
                SDL_RenderCopy(r, icons[i], NULL, &ir);
            }
        }
    }
}

bool sprite_panel_handle_event(const SDL_Event &e, AppState &state, const SpritePanelRects &rects)
{
    if (e.type == SDL_MOUSEMOTION)
    {
        int mx = e.motion.x, my = e.motion.y;
        state.sprite_menu_open = sp_point_in(rects.sprite_btn, mx, my) || (state.sprite_menu_open && sp_point_in(rects.sprite_menu, mx, my));
        state.backdrop_menu_open = sp_point_in(rects.backdrop_btn, mx, my) || (state.backdrop_menu_open && sp_point_in(rects.backdrop_menu, mx, my));
        if (state.sprite_menu_open || state.backdrop_menu_open)
            return true;
    }
    else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        int mx = e.button.x, my = e.button.y;

        if (sp_point_in(rects.backdrop_area, mx, my))
        {
            if (!sp_point_in(rects.backdrop_btn, mx, my) && !state.backdrop_menu_open)
            {
                state.editing_target_is_stage = true;
                state.current_tab = TAB_COSTUMES;
                return true;
            }
        }

        if (sp_point_in(rects.sprite_list_area, mx, my) && !state.sprite_menu_open)
        {
            int grid_x = rects.sprite_list_area.x + 12, grid_y = rects.sprite_list_area.y + 12;
            for (size_t i = 0; i < state.sprites.size(); i++)
            {
                SDL_Rect thumb = {grid_x, grid_y, THUMB_W, THUMB_H};

                if (!state.editing_target_is_stage && (int)i == state.selected_sprite)
                {
                    SDL_Rect border = {thumb.x - 3, thumb.y - 3, thumb.w + 6, thumb.h + 22};
                    SDL_Rect del_r = {border.x + border.w - 22, border.y - 10, 24, 24};
                    if (sp_point_in(del_r, mx, my))
                    {
                        // ---> DELETE OS FILES FOR THIS SPRITE <---
                        for (auto &c : state.sprites[i].costumes)
                            delete_asset_from_project(c.source_path);
                        for (auto &s : state.sprites[i].sounds)
                            delete_asset_from_project(s.source_path);

                        state.sprites.erase(state.sprites.begin() + i);
                        if (state.selected_sprite >= (int)state.sprites.size())
                            state.selected_sprite = state.sprites.size() - 1;
                        return true;
                    }
                }

                if (sp_point_in(thumb, mx, my))
                {
                    state.selected_sprite = i;
                    state.editing_target_is_stage = false;
                    return true;
                }

                grid_x += THUMB_W + 15;
                if (grid_x + THUMB_W > rects.sprite_list_area.x + rects.sprite_list_area.w - 60)
                {
                    grid_x = rects.sprite_list_area.x + 12;
                    grid_y += THUMB_H + 25;
                }
            }
            return true;
        }

        if (state.backdrop_menu_open)
        {
            if (sp_point_in(rects.backdrop_menu_items[0], mx, my) || sp_point_in(rects.backdrop_menu_items[1], mx, my))
            {
                state.current_tab = TAB_COSTUMES;
            }
        }
    }
    return false;
}