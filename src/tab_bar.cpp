#include "tab_bar.h"
#include "config.h"
#include "renderer.h"
#include "interpreter.h"
#include <cstring>

static bool point_in_rect(int px, int py, const SDL_Rect &r) { return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h; }
static bool point_in_circle(int px, int py, int cx, int cy, int rad)
{
    int dx = px - cx, dy = py - cy;
    return (dx * dx + dy * dy) <= (rad * rad);
}
static void set_color(SDL_Renderer *r, Color c) { SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255); }

static void draw_text(SDL_Renderer *r, TTF_Font *f, const char *txt, int x, int y, Color c)
{
    SDL_Color sc;
    sc.r = (Uint8)c.r;
    sc.g = (Uint8)c.g;
    sc.b = (Uint8)c.b;
    sc.a = 255;

    SDL_Surface *s = TTF_RenderUTF8_Blended(f, txt, sc);
    if (!s)
        return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
    SDL_Rect dst = {x, y, s->w, s->h};
    SDL_RenderCopy(r, t, NULL, &dst);
    SDL_DestroyTexture(t);
    SDL_FreeSurface(s);
}

void tab_bar_layout(TabBarRects &rects)
{
    int y = NAVBAR_HEIGHT;
    int bar_w = WINDOW_WIDTH - RIGHT_COLUMN_WIDTH;
    rects.bar = {0, y, bar_w, TAB_BAR_HEIGHT};

    int icon_sz = 20;
    int tab_widths[3] = {
        TAB_HPADDING + icon_sz + 4 + 36 + TAB_HPADDING,
        TAB_HPADDING + icon_sz + 4 + 82 + TAB_HPADDING,
        TAB_HPADDING + icon_sz + 4 + 56 + TAB_HPADDING};

    int tx = 0;
    for (int i = 0; i < 3; ++i)
    {
        rects.tabs[i] = {tx, y, tab_widths[i], TAB_BAR_HEIGHT};
        rects.tab_icons[i] = {tx + TAB_HPADDING, y + (TAB_BAR_HEIGHT - icon_sz) / 2, icon_sz, icon_sz};
        tx += tab_widths[i];
    }

    int btn_area_x = bar_w - 80;
    int cy = y + TAB_BAR_HEIGHT / 2;
    rects.start_btn = {btn_area_x, cy - START_BTN_RADIUS, START_BTN_RADIUS * 2, START_BTN_RADIUS * 2};
    rects.stop_btn = {btn_area_x + START_BTN_RADIUS * 2 + 10, cy - STOP_BTN_RADIUS, STOP_BTN_RADIUS * 2, STOP_BTN_RADIUS * 2};
}

void tab_bar_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state, const TabBarRects &rects, const Textures &tex)
{
    set_color(r, COL_TAB_BAR_BG);
    SDL_RenderFillRect(r, &rects.bar);
    set_color(r, COL_SEPARATOR);
    SDL_RenderDrawLine(r, rects.bar.x, rects.bar.y + rects.bar.h - 1, rects.bar.x + rects.bar.w, rects.bar.y + rects.bar.h - 1);

    const char *labels[3] = {"Code", state.editing_target_is_stage ? "Backdrops" : "Costumes", "Sounds"};

    SDL_Texture *active_icons[3] = {tex.code_active, tex.brush_tab_active, tex.volume_active};
    SDL_Texture *inactive_icons[3] = {tex.code_inactive, tex.brush_nonactive, tex.volume_inactive};

    for (int i = 0; i < 3; ++i)
    {
        bool disabled = state.editing_target_is_stage && (i == 0 || i == 2);
        bool active = ((int)state.current_tab == i) && !disabled;

        if (active)
        {
            set_color(r, COL_TAB_ACTIVE_BG);
            SDL_RenderFillRect(r, &rects.tabs[i]);
            SDL_SetRenderDrawColor(r, 95, 149, 247, 255);
            SDL_Rect ul = {rects.tabs[i].x, rects.tabs[i].y + rects.tabs[i].h - 3, rects.tabs[i].w, 3};
            SDL_RenderFillRect(r, &ul);
        }

        // ---> FIXED: SHOW PROPER BACKDROP ICON <---
        SDL_Texture *ico = nullptr;
        if (i == 1 && state.editing_target_is_stage)
        {
            ico = tex.backdrop_btn_icon ? tex.backdrop_btn_icon : (active ? tex.brush_active : tex.brush_tab_active);
        }
        else
        {
            ico = active ? active_icons[i] : inactive_icons[i];
        }

        if (ico)
        {
            if (disabled)
                SDL_SetTextureColorMod(ico, 150, 150, 150); // Grey out disabled icons
            renderer_draw_texture_fit(r, ico, &rects.tab_icons[i]);
            if (disabled)
                SDL_SetTextureColorMod(ico, 255, 255, 255); // Reset color
        }

        Color tc = active ? COL_TAB_ACTIVE_TEXT : COL_TAB_INACTIVE_TEXT;
        if (disabled)
            tc = {150, 150, 150}; // Grey out text

        int text_x = rects.tab_icons[i].x + rects.tab_icons[i].w + 4;
        int text_y = rects.tabs[i].y + (rects.tabs[i].h - 14) / 2;
        draw_text(r, font, labels[i], text_x, text_y, tc);
    }

    if (tex.green_flag)
        renderer_draw_texture_fit(r, tex.green_flag, &rects.start_btn);
    else
    {
        Color col = state.start_hover ? COL_START_HOVER : COL_START_BTN;
        renderer_fill_circle(r, rects.start_btn.x + START_BTN_RADIUS, rects.start_btn.y + START_BTN_RADIUS, START_BTN_RADIUS, col.r, col.g, col.b);
    }

    SDL_Texture *stop_tex = (state.stop_hover && tex.pause2) ? tex.pause2 : tex.pause1;
    if (stop_tex)
        renderer_draw_texture_fit(r, stop_tex, &rects.stop_btn);
    else
    {
        Color col = state.stop_hover ? COL_STOP_HOVER : COL_STOP_BTN;
        int cx = rects.stop_btn.x + STOP_BTN_RADIUS, cy = rects.stop_btn.y + STOP_BTN_RADIUS;
        renderer_fill_circle(r, cx, cy, STOP_BTN_RADIUS, col.r, col.g, col.b);
        set_color(r, COL_WHITE);
        int half = STOP_BTN_RADIUS / 2;
        SDL_Rect sq = {cx - half, cy - half, half * 2, half * 2};
        SDL_RenderFillRect(r, &sq);
    }
}

bool tab_bar_handle_event(const SDL_Event &e, AppState &state, const TabBarRects &rects)
{
    if (e.type == SDL_MOUSEMOTION)
    {
        int mx = e.motion.x, my = e.motion.y;
        state.start_hover = point_in_circle(mx, my, rects.start_btn.x + START_BTN_RADIUS, rects.start_btn.y + START_BTN_RADIUS, START_BTN_RADIUS + 2);
        state.stop_hover = point_in_circle(mx, my, rects.stop_btn.x + STOP_BTN_RADIUS, rects.stop_btn.y + STOP_BTN_RADIUS, STOP_BTN_RADIUS + 2);
        return false;
    }
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        int mx = e.button.x, my = e.button.y;
        for (int i = 0; i < 3; ++i)
        {
            bool disabled = state.editing_target_is_stage && (i == 0 || i == 2);
            if (point_in_rect(mx, my, rects.tabs[i]))
            {
                if (!disabled)
                {
                    state.current_tab = (Tab)i;
                }
                return true;
            }
        }
        if (point_in_circle(mx, my, rects.start_btn.x + START_BTN_RADIUS, rects.start_btn.y + START_BTN_RADIUS, START_BTN_RADIUS + 2))
        {
            interpreter_trigger_flag(state);
            return true;
        }
        if (point_in_circle(mx, my, rects.stop_btn.x + STOP_BTN_RADIUS, rects.stop_btn.y + STOP_BTN_RADIUS, STOP_BTN_RADIUS + 2))
        {
            interpreter_stop_all(state);
            return true;
        }
        if (point_in_rect(mx, my, rects.bar))
            return true;
    }
    return false;
}