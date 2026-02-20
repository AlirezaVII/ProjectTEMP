#include "sprite_panel.h"
#include "config.h"
#include <cstdio>
#include <cmath>

/* ───────────────── helpers ───────────────── */

static void sp_draw_text(SDL_Renderer *r, TTF_Font *font, const char *txt,
                         int x, int y, Uint8 cr, Uint8 cg, Uint8 cb)
{
    SDL_Color col;
    col.r = cr; col.g = cg; col.b = cb; col.a = 255;
    SDL_Surface *s = TTF_RenderUTF8_Blended(font, txt, col);
    if (!s) return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
    SDL_Rect dst;
    dst.x = x; dst.y = y; dst.w = s->w; dst.h = s->h;
    SDL_RenderCopy(r, t, NULL, &dst);
    SDL_DestroyTexture(t);
    SDL_FreeSurface(s);
}

static void sp_draw_text_centered(SDL_Renderer *r, TTF_Font *font, const char *txt,
                                  int cx, int cy, Uint8 cr, Uint8 cg, Uint8 cb)
{
    SDL_Color col;
    col.r = cr; col.g = cg; col.b = cb; col.a = 255;
    SDL_Surface *s = TTF_RenderUTF8_Blended(font, txt, col);
    if (!s) return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
    SDL_Rect dst;
    dst.x = cx - s->w / 2; dst.y = cy - s->h / 2;
    dst.w = s->w; dst.h = s->h;
    SDL_RenderCopy(r, t, NULL, &dst);
    SDL_DestroyTexture(t);
    SDL_FreeSurface(s);
}

static bool sp_point_in(const SDL_Rect &r, int x, int y)
{
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

static void sp_fill_rect(SDL_Renderer *r, const SDL_Rect &rc,
                          Uint8 cr, Uint8 cg, Uint8 cb, Uint8 ca)
{
    SDL_SetRenderDrawColor(r, cr, cg, cb, ca);
    SDL_RenderFillRect(r, &rc);
}

static void sp_draw_circle(SDL_Renderer *r, int cx, int cy, int rad,
                            Uint8 cr, Uint8 cg, Uint8 cb)
{
    SDL_SetRenderDrawColor(r, cr, cg, cb, 255);
    for (int dy = -rad; dy <= rad; dy++) {
        int dx = (int)std::sqrt((double)(rad * rad - dy * dy));
        SDL_RenderDrawLine(r, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

/* ───────────────── constants ───────────────── */

static const int SETTINGS_PANEL_H = 80;   /* sprite-info / settings height */
static const int BACKDROP_W       = 90;   /* width of backdrop strip */
static const int BTN_RAD          = 20;   /* circle button radius */
static const int MENU_ITEM_SZ     = 36;
static const int MENU_PAD         = 4;
static const int MENU_W           = MENU_ITEM_SZ + MENU_PAD * 2;
static const int THUMB_W          = 64;
static const int THUMB_H          = 64;

/* ───────────────── layout ───────────────── */

void sprite_panel_layout(SpritePanelRects &rects)
{
    int rc_x = WINDOW_WIDTH - RIGHT_COLUMN_WIDTH;

    /*
     * Stage height is computed the same way as stage_layout():
     *   col_h  = WINDOW_HEIGHT - NAVBAR_HEIGHT
     *   stage_h = col_h * STAGE_HEIGHT_RATIO / 100
     *
     * Stage panel starts at y = NAVBAR_HEIGHT and has height = stage_h.
     * Settings panel sits directly below the stage panel.
     * Sprite/Backdrop panel sits below settings.
     */
    int col_h   = WINDOW_HEIGHT - NAVBAR_HEIGHT;
    int stage_h  = col_h * STAGE_HEIGHT_RATIO / 100;

    int stage_bottom    = NAVBAR_HEIGHT + stage_h;
    int settings_bottom = stage_bottom + SETTINGS_PANEL_H;

    /* panel area: from settings_bottom to window bottom */
    int panel_top = settings_bottom;
    int panel_h   = WINDOW_HEIGHT - panel_top;

    /* ── Backdrop area (right strip) ── */
    rects.backdrop_area.x = rc_x + RIGHT_COLUMN_WIDTH - BACKDROP_W;
    rects.backdrop_area.y = panel_top;
    rects.backdrop_area.w = BACKDROP_W;
    rects.backdrop_area.h = panel_h;

    /* backdrop preview thumb (centred near top) */
    rects.backdrop_thumb.w = 64;
    rects.backdrop_thumb.h = 48;
    rects.backdrop_thumb.x = rects.backdrop_area.x +
                             (rects.backdrop_area.w - rects.backdrop_thumb.w) / 2;
    rects.backdrop_thumb.y = rects.backdrop_area.y + 24;

    /* backdrop label area (below thumb) */
    rects.backdrop_label.x = rects.backdrop_area.x;
    rects.backdrop_label.y = rects.backdrop_thumb.y + rects.backdrop_thumb.h + 2;
    rects.backdrop_label.w = rects.backdrop_area.w;
    rects.backdrop_label.h = 30;

    /* backdrop circle button (bottom-right) */
    rects.backdrop_btn.w = BTN_RAD * 2;
    rects.backdrop_btn.h = BTN_RAD * 2;
    rects.backdrop_btn.x = rects.backdrop_area.x +
                           rects.backdrop_area.w - rects.backdrop_btn.w - 6;
    rects.backdrop_btn.y = rects.backdrop_area.y +
                           rects.backdrop_area.h - rects.backdrop_btn.h - 6;

    /* backdrop hover menu (3 items stacked upward from button) */
    int menu_h = 3 * (MENU_ITEM_SZ + MENU_PAD) + MENU_PAD;
    rects.backdrop_menu.w = MENU_W;
    rects.backdrop_menu.h = menu_h;
    rects.backdrop_menu.x = rects.backdrop_btn.x +
                            (rects.backdrop_btn.w - MENU_W) / 2;
    rects.backdrop_menu.y = rects.backdrop_btn.y - menu_h - 4;

    for (int i = 0; i < 3; i++) {
        rects.backdrop_menu_items[i].x = rects.backdrop_menu.x + MENU_PAD;
        rects.backdrop_menu_items[i].y = rects.backdrop_menu.y + MENU_PAD +
                                          i * (MENU_ITEM_SZ + MENU_PAD);
        rects.backdrop_menu_items[i].w = MENU_ITEM_SZ;
        rects.backdrop_menu_items[i].h = MENU_ITEM_SZ;
    }

    /* ── Sprite list area (left of backdrop) ── */
    rects.sprite_list_area.x = rc_x;
    rects.sprite_list_area.y = panel_top;
    rects.sprite_list_area.w = RIGHT_COLUMN_WIDTH - BACKDROP_W;
    rects.sprite_list_area.h = panel_h;

    /* sprite thumbnail (Sprite1, top-left with padding) */
    rects.sprite_thumb.w = THUMB_W;
    rects.sprite_thumb.h = THUMB_H;
    rects.sprite_thumb.x = rects.sprite_list_area.x + 12;
    rects.sprite_thumb.y = rects.sprite_list_area.y + 12;

    /* sprite circle button (bottom-right of sprite list) */
    rects.sprite_btn.w = BTN_RAD * 2;
    rects.sprite_btn.h = BTN_RAD * 2;
    rects.sprite_btn.x = rects.sprite_list_area.x +
                         rects.sprite_list_area.w - rects.sprite_btn.w - 6;
    rects.sprite_btn.y = rects.sprite_list_area.y +
                         rects.sprite_list_area.h - rects.sprite_btn.h - 6;

    /* sprite hover menu (3 items stacked upward) */
    rects.sprite_menu.w = MENU_W;
    rects.sprite_menu.h = menu_h;
    rects.sprite_menu.x = rects.sprite_btn.x +
                          (rects.sprite_btn.w - MENU_W) / 2;
    rects.sprite_menu.y = rects.sprite_btn.y - menu_h - 4;

    for (int i = 0; i < 3; i++) {
        rects.sprite_menu_items[i].x = rects.sprite_menu.x + MENU_PAD;
        rects.sprite_menu_items[i].y = rects.sprite_menu.y + MENU_PAD +
                                        i * (MENU_ITEM_SZ + MENU_PAD);
        rects.sprite_menu_items[i].w = MENU_ITEM_SZ;
        rects.sprite_menu_items[i].h = MENU_ITEM_SZ;
    }
}

/* ───────────────── draw ───────────────── */

void sprite_panel_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state,
                       const Textures &tex, const SpritePanelRects &rects)
{
    /* ════════════ SPRITE LIST ════════════ */

    /* background */
    sp_fill_rect(r, rects.sprite_list_area, 244, 246, 248, 255);

    /* separator line between sprite list and backdrop panel */
    SDL_SetRenderDrawColor(r, 220, 220, 220, 255);
    SDL_RenderDrawLine(r,
        rects.backdrop_area.x, rects.backdrop_area.y,
        rects.backdrop_area.x, rects.backdrop_area.y + rects.backdrop_area.h);

    /* top separator */
    SDL_RenderDrawLine(r,
        rects.sprite_list_area.x, rects.sprite_list_area.y,
        rects.sprite_list_area.x + rects.sprite_list_area.w +
        rects.backdrop_area.w, rects.sprite_list_area.y);

    /* ── Sprite1 thumbnail ── */
    {
        /* blue selection border */
        SDL_Rect border;
        border.x = rects.sprite_thumb.x - 3;
        border.y = rects.sprite_thumb.y - 3;
        border.w = rects.sprite_thumb.w + 6;
        border.h = rects.sprite_thumb.h + 22;
        SDL_SetRenderDrawColor(r, 77, 151, 255, 255);
        SDL_RenderDrawRect(r, &border);

        /* white inner fill */
        SDL_Rect inner;
        inner.x = rects.sprite_thumb.x - 2;
        inner.y = rects.sprite_thumb.y - 2;
        inner.w = rects.sprite_thumb.w + 4;
        inner.h = rects.sprite_thumb.h + 4;
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        SDL_RenderFillRect(r, &inner);

        /* cat image */
        if (tex.scratch_cat) {
            SDL_RenderCopy(r, tex.scratch_cat, NULL,
                           const_cast<SDL_Rect*>(&rects.sprite_thumb));
        } else {
            /* fallback: orange rectangle */
            SDL_SetRenderDrawColor(r, 255, 165, 0, 255);
            SDL_RenderFillRect(r, const_cast<SDL_Rect*>(&rects.sprite_thumb));
        }

        /* "Sprite1" label below thumb */
        int lx = rects.sprite_thumb.x + rects.sprite_thumb.w / 2;
        int ly = rects.sprite_thumb.y + rects.sprite_thumb.h + 8;
        sp_draw_text_centered(r, font, "Sprite1", lx, ly, 80, 80, 80);
    }

    /* ── Sprite "+" button ── */
    {
        int cx = rects.sprite_btn.x + BTN_RAD;
        int cy = rects.sprite_btn.y + BTN_RAD;
        sp_draw_circle(r, cx, cy, BTN_RAD, 77, 151, 255);

        if (tex.sprite_btn_icon) {
            SDL_Rect ir;
            ir.w = 24; ir.h = 24;
            ir.x = cx - 12; ir.y = cy - 12;
            SDL_RenderCopy(r, tex.sprite_btn_icon, NULL, &ir);
        } else {
            sp_draw_text_centered(r, font, "+", cx, cy, 255, 255, 255);
        }
    }

    /* ── Sprite hover menu ── */
    if (state.sprite_menu_open) {
        /* blue menu background */
        sp_fill_rect(r, rects.sprite_menu, 77, 151, 255, 255);

        /* items: 0=search, 1=surprise, 2=upload (top to bottom) */
        SDL_Texture *icons[3];
        icons[0] = tex.search_icon;
        icons[1] = tex.surprise_icon;
        icons[2] = tex.upload_icon;

        for (int i = 0; i < 3; i++) {
            int icx = rects.sprite_menu_items[i].x + MENU_ITEM_SZ / 2;
            int icy = rects.sprite_menu_items[i].y + MENU_ITEM_SZ / 2;

            /* white circle behind icon */
            sp_draw_circle(r, icx, icy, MENU_ITEM_SZ / 2 - 2, 255, 255, 255);

            if (icons[i]) {
                SDL_Rect ir;
                ir.w = 20; ir.h = 20;
                ir.x = icx - 10; ir.y = icy - 10;
                SDL_RenderCopy(r, icons[i], NULL, &ir);
            }
        }

        /* "Choose a Sprite" tooltip */
        int lbl_x = rects.sprite_menu.x - 115;
        int lbl_y = rects.sprite_btn.y + BTN_RAD - 8;
        SDL_Rect lbl_bg;
        lbl_bg.x = lbl_x - 4; lbl_bg.y = lbl_y - 4;
        lbl_bg.w = 112; lbl_bg.h = 22;
        sp_fill_rect(r, lbl_bg, 77, 151, 255, 255);
        sp_draw_text(r, font, "Choose a Sprite", lbl_x, lbl_y, 255, 255, 255);
    }

    /* ════════════ BACKDROP PANEL ════════════ */

    /* background */
    sp_fill_rect(r, rects.backdrop_area, 244, 246, 248, 255);

    /* "Stage" label at top */
    {
        int lx = rects.backdrop_area.x + rects.backdrop_area.w / 2;
        int ly = rects.backdrop_area.y + 10;
        sp_draw_text_centered(r, font, "Stage", lx, ly, 80, 80, 80);
    }

    /* backdrop preview (white rect with border) */
    {
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        SDL_RenderFillRect(r, const_cast<SDL_Rect*>(&rects.backdrop_thumb));
        SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
        SDL_RenderDrawRect(r, const_cast<SDL_Rect*>(&rects.backdrop_thumb));
    }

    /* "Backdrops" + "1" labels */
    {
        int lx = rects.backdrop_area.x + rects.backdrop_area.w / 2;
        int ly = rects.backdrop_label.y + 8;
        sp_draw_text_centered(r, font, "Backdrops", lx, ly, 100, 100, 100);
        sp_draw_text_centered(r, font, "1", lx, ly + 16, 140, 140, 140);
    }

    /* ── Backdrop "+" button ── */
    {
        int cx = rects.backdrop_btn.x + BTN_RAD;
        int cy = rects.backdrop_btn.y + BTN_RAD;
        sp_draw_circle(r, cx, cy, BTN_RAD, 77, 151, 255);

        if (tex.backdrop_btn_icon) {
            SDL_Rect ir;
            ir.w = 24; ir.h = 24;
            ir.x = cx - 12; ir.y = cy - 12;
            SDL_RenderCopy(r, tex.backdrop_btn_icon, NULL, &ir);
        } else {
            sp_draw_text_centered(r, font, "+", cx, cy, 255, 255, 255);
        }
    }

    /* ── Backdrop hover menu ── */
    if (state.backdrop_menu_open) {
        sp_fill_rect(r, rects.backdrop_menu, 77, 151, 255, 255);

        SDL_Texture *icons[3];
        icons[0] = tex.search_icon;
        icons[1] = tex.surprise_icon;
        icons[2] = tex.upload_icon;

        for (int i = 0; i < 3; i++) {
            int icx = rects.backdrop_menu_items[i].x + MENU_ITEM_SZ / 2;
            int icy = rects.backdrop_menu_items[i].y + MENU_ITEM_SZ / 2;

            sp_draw_circle(r, icx, icy, MENU_ITEM_SZ / 2 - 2, 255, 255, 255);

            if (icons[i]) {
                SDL_Rect ir;
                ir.w = 20; ir.h = 20;
                ir.x = icx - 10; ir.y = icy - 10;
                SDL_RenderCopy(r, icons[i], NULL, &ir);
            }
        }

        /* "Choose a Backdrop" tooltip */
        int lbl_x = rects.backdrop_menu.x - 130;
        int lbl_y = rects.backdrop_btn.y + BTN_RAD - 8;
        SDL_Rect lbl_bg;
        lbl_bg.x = lbl_x - 4; lbl_bg.y = lbl_y - 4;
        lbl_bg.w = 128; lbl_bg.h = 22;
        sp_fill_rect(r, lbl_bg, 77, 151, 255, 255);
        sp_draw_text(r, font, "Choose a Backdrop", lbl_x, lbl_y, 255, 255, 255);
    }
}

/* ───────────────── event handling ───────────────── */

bool sprite_panel_handle_event(const SDL_Event &e, AppState &state,
                               const SpritePanelRects &rects)
{
    if (e.type == SDL_MOUSEMOTION) {
        int mx = e.motion.x;
        int my = e.motion.y;

        /* sprite button / menu hover */
        bool over_sprite_btn  = sp_point_in(rects.sprite_btn, mx, my);
        bool over_sprite_menu = state.sprite_menu_open &&
                                sp_point_in(rects.sprite_menu, mx, my);
        state.sprite_menu_open = over_sprite_btn || over_sprite_menu;

        /* backdrop button / menu hover */
        bool over_backdrop_btn  = sp_point_in(rects.backdrop_btn, mx, my);
        bool over_backdrop_menu = state.backdrop_menu_open &&
                                  sp_point_in(rects.backdrop_menu, mx, my);
        state.backdrop_menu_open = over_backdrop_btn || over_backdrop_menu;

        if (over_sprite_btn || over_sprite_menu ||
            over_backdrop_btn || over_backdrop_menu)
            return true;
    }

    return false;
}
