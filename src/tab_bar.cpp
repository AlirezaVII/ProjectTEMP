#include "tab_bar.h"
#include "config.h"
#include "renderer.h"
#include <cstring>

static bool point_in_rect(int px, int py, const SDL_Rect &r)
{
    return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
}

static bool point_in_circle(int px, int py, int cx, int cy, int rad)
{
    int dx = px - cx, dy = py - cy;
    return (dx * dx + dy * dy) <= (rad * rad);
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

void tab_bar_layout(TabBarRects &rects)
{
    int y = NAVBAR_HEIGHT;
    int bar_w = WINDOW_WIDTH - RIGHT_COLUMN_WIDTH;

    rects.bar.x = 0;
    rects.bar.y = y;
    rects.bar.w = bar_w;
    rects.bar.h = TAB_BAR_HEIGHT;

    int icon_sz = 20;
    int tab_widths[3];
    tab_widths[0] = TAB_HPADDING + icon_sz + 4 + 36 + TAB_HPADDING;   /* Code    */
    tab_widths[1] = TAB_HPADDING + icon_sz + 4 + 72 + TAB_HPADDING;   /* Costumes */
    tab_widths[2] = TAB_HPADDING + icon_sz + 4 + 56 + TAB_HPADDING;   /* Sounds  */

    int tx = 0;
    for (int i = 0; i < 3; ++i) {
        rects.tabs[i].x = tx;
        rects.tabs[i].y = y;
        rects.tabs[i].w = tab_widths[i];
        rects.tabs[i].h = TAB_BAR_HEIGHT;

        rects.tab_icons[i].x = tx + TAB_HPADDING;
        rects.tab_icons[i].y = y + (TAB_BAR_HEIGHT - icon_sz) / 2;
        rects.tab_icons[i].w = icon_sz;
        rects.tab_icons[i].h = icon_sz;

        tx += tab_widths[i];
    }

    int btn_area_x = bar_w - 80;
    int cy = y + TAB_BAR_HEIGHT / 2;

    rects.start_btn.x = btn_area_x;
    rects.start_btn.y = cy - START_BTN_RADIUS;
    rects.start_btn.w = START_BTN_RADIUS * 2;
    rects.start_btn.h = START_BTN_RADIUS * 2;

    rects.stop_btn.x = btn_area_x + START_BTN_RADIUS * 2 + 10;
    rects.stop_btn.y = cy - STOP_BTN_RADIUS;
    rects.stop_btn.w = STOP_BTN_RADIUS * 2;
    rects.stop_btn.h = STOP_BTN_RADIUS * 2;
}

void tab_bar_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state,
                  const TabBarRects &rects, const Textures &tex)
{
    /* bar bg */
    set_color(r, COL_TAB_BAR_BG);
    SDL_RenderFillRect(r, &rects.bar);

    /* bottom line */
    set_color(r, COL_SEPARATOR);
    SDL_RenderDrawLine(r, rects.bar.x, rects.bar.y + rects.bar.h - 1,
                       rects.bar.x + rects.bar.w, rects.bar.y + rects.bar.h - 1);

    const char *labels[3] = {"Code", "Costumes", "Sounds"};

    SDL_Texture *active_icons[3];
    SDL_Texture *inactive_icons[3];
    active_icons[0]   = tex.code_active;
    inactive_icons[0] = tex.code_inactive;
    active_icons[1]   = tex.brush_active;
    inactive_icons[1] = tex.brush_inactive;
    active_icons[2]   = tex.volume_active;
    inactive_icons[2] = tex.volume_inactive;

    for (int i = 0; i < 3; ++i) {
        bool active = ((int)state.current_tab == i);

        if (active) {
            set_color(r, COL_TAB_ACTIVE_BG);
            SDL_RenderFillRect(r, &rects.tabs[i]);
            /* accent underline */
            SDL_SetRenderDrawColor(r, 95, 149, 247, 255);
            SDL_Rect ul;
            ul.x = rects.tabs[i].x;
            ul.y = rects.tabs[i].y + rects.tabs[i].h - 3;
            ul.w = rects.tabs[i].w;
            ul.h = 3;
            SDL_RenderFillRect(r, &ul);
        }

        /* icon */
        SDL_Texture *ico = active ? active_icons[i] : inactive_icons[i];
        if (ico) {
            renderer_draw_texture_fit(r, ico, &rects.tab_icons[i]);
        }

        /* label */
        Color tc = active ? COL_TAB_ACTIVE_TEXT : COL_TAB_INACTIVE_TEXT;
        int text_x = rects.tab_icons[i].x + rects.tab_icons[i].w + 4;
        int text_y = rects.tabs[i].y + (rects.tabs[i].h - 14) / 2;
        draw_text(r, font, labels[i], text_x, text_y, tc);
    }

    /* --- green flag button --- */
    if (tex.green_flag) {
        renderer_draw_texture_fit(r, tex.green_flag, &rects.start_btn);
    } else {
        int cx = rects.start_btn.x + START_BTN_RADIUS;
        int cy = rects.start_btn.y + START_BTN_RADIUS;
        Color col = state.start_hover ? COL_START_HOVER : COL_START_BTN;
        renderer_fill_circle(r, cx, cy, START_BTN_RADIUS, col.r, col.g, col.b);
    }

    /* --- stop button --- */
    {
        SDL_Texture *stop_tex = tex.pause1;
        if (state.stop_hover && tex.pause2) {
            stop_tex = tex.pause2;
        }
        if (stop_tex) {
            renderer_draw_texture_fit(r, stop_tex, &rects.stop_btn);
        } else {
            int cx = rects.stop_btn.x + STOP_BTN_RADIUS;
            int cy = rects.stop_btn.y + STOP_BTN_RADIUS;
            Color col = state.stop_hover ? COL_STOP_HOVER : COL_STOP_BTN;
            renderer_fill_circle(r, cx, cy, STOP_BTN_RADIUS, col.r, col.g, col.b);
            set_color(r, COL_WHITE);
            int half = STOP_BTN_RADIUS / 2;
            SDL_Rect sq;
            sq.x = cx - half; sq.y = cy - half;
            sq.w = half * 2; sq.h = half * 2;
            SDL_RenderFillRect(r, &sq);
        }
    }
}

bool tab_bar_handle_event(const SDL_Event &e, AppState &state,
                          const TabBarRects &rects)
{
    if (e.type == SDL_MOUSEMOTION) {
        int mx = e.motion.x, my = e.motion.y;
        int scx = rects.start_btn.x + START_BTN_RADIUS;
        int scy = rects.start_btn.y + START_BTN_RADIUS;
        state.start_hover = point_in_circle(mx, my, scx, scy, START_BTN_RADIUS + 2);
        int ocx = rects.stop_btn.x + STOP_BTN_RADIUS;
        int ocy = rects.stop_btn.y + STOP_BTN_RADIUS;
        state.stop_hover = point_in_circle(mx, my, ocx, ocy, STOP_BTN_RADIUS + 2);
        return false;
    }

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x, my = e.button.y;

        for (int i = 0; i < 3; ++i) {
            if (point_in_rect(mx, my, rects.tabs[i])) {
                state.current_tab = (Tab)i;
                return true;
            }
        }

        int scx = rects.start_btn.x + START_BTN_RADIUS;
        int scy = rects.start_btn.y + START_BTN_RADIUS;
        if (point_in_circle(mx, my, scx, scy, START_BTN_RADIUS + 2)) {
            state.running = true;
            return true;
        }

        int ocx = rects.stop_btn.x + STOP_BTN_RADIUS;
        int ocy = rects.stop_btn.y + STOP_BTN_RADIUS;
        if (point_in_circle(mx, my, ocx, ocy, STOP_BTN_RADIUS + 2)) {
            state.running = false;
            return true;
        }

        if (point_in_rect(mx, my, rects.bar)) return true;
    }

    return false;
}
