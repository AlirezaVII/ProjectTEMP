#include "categories.h"
#include "config.h"
#include "blocks.h"
#include "renderer.h"

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

void categories_layout(CategoriesRects &rects)
{
    int top = NAVBAR_HEIGHT + TAB_BAR_HEIGHT;
    int panel_h = WINDOW_HEIGHT - top;

    rects.panel.x = 0;
    rects.panel.y = top;
    rects.panel.w = CATEGORY_WIDTH;
    rects.panel.h = panel_h;

    int item_h = 32;
    int y = top + 6;
    for (int i = 0; i < NUM_CATEGORIES; ++i) {
        rects.items[i].x = 0;
        rects.items[i].y = y;
        rects.items[i].w = CATEGORY_WIDTH;
        rects.items[i].h = item_h;

        rects.dots[i].x = 6;
        rects.dots[i].y = y + item_h / 2 - 4;
        rects.dots[i].w = 8;
        rects.dots[i].h = 8;

        y += item_h;
    }

    // Layout the extension button at the bottom of the column
    int btn_h = 40;
    rects.ext_btn.x = 0;
    rects.ext_btn.y = WINDOW_HEIGHT - btn_h;
    rects.ext_btn.w = CATEGORY_WIDTH;
    rects.ext_btn.h = btn_h;
}

void categories_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state,
                     const CategoriesRects &rects)
{
    set_color(r, COL_CAT_BG);
    SDL_RenderFillRect(r, &rects.panel);

    /* right border */
    set_color(r, COL_SEPARATOR);
    SDL_RenderDrawLine(r, rects.panel.x + rects.panel.w - 1, rects.panel.y,
                       rects.panel.x + rects.panel.w - 1,
                       rects.panel.y + rects.panel.h);

    // If extension is not enabled, we only draw up to My Blocks (9 items). If enabled, 10 items.
    int active_cats = state.pen_extension_enabled ? NUM_CATEGORIES : (NUM_CATEGORIES - 1);

    for (int i = 0; i < active_cats; ++i) {
        bool sel = (state.selected_category == i);

        if (sel) {
            set_color(r, COL_CAT_SELECTED_BG);
            SDL_RenderFillRect(r, &rects.items[i]);
        }

        /* dot */
        Color dc = blocks_category_color(i);
        int cx = rects.dots[i].x + rects.dots[i].w / 2;
        int cy = rects.dots[i].y + rects.dots[i].h / 2;
        renderer_fill_circle(r, cx, cy, 4, dc.r, dc.g, dc.b);

        /* label */
        const char *name = blocks_category_name(i);
        Color tc = sel ? COL_CAT_SELECTED_TEXT : COL_CAT_TEXT;
        int tx = rects.dots[i].x + rects.dots[i].w + 4;
        int ty = rects.items[i].y + (rects.items[i].h - 12) / 2;
        draw_text(r, font, name, tx, ty, tc);
    }

    // Draw Extension Button (Blue Background)
    SDL_SetRenderDrawColor(r, 76, 151, 255, 255); 
    SDL_RenderFillRect(r, &rects.ext_btn);
    
    // ---> FIXED: PERFECTLY CENTERED PLUS SIGN <---
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    
    int center_x = rects.ext_btn.x + rects.ext_btn.w / 2;
    int center_y = rects.ext_btn.y + rects.ext_btn.h / 2;
    int thickness = 4;
    int length = 22;

    SDL_Rect plus_v = { center_x - thickness / 2, center_y - length / 2, thickness, length };
    SDL_Rect plus_h = { center_x - length / 2, center_y - thickness / 2, length, thickness };
    
    SDL_RenderFillRect(r, &plus_v);
    SDL_RenderFillRect(r, &plus_h);
}

bool categories_handle_event(const SDL_Event &e, AppState &state,
                             const CategoriesRects &rects)
{
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x, my = e.button.y;

        // Check if Extension Button was clicked
        if (point_in_rect(mx, my, rects.ext_btn)) {
            state.mode = MODE_EXTENSION_LIBRARY;
            return true;
        }

        int active_cats = state.pen_extension_enabled ? NUM_CATEGORIES : (NUM_CATEGORIES - 1);
        for (int i = 0; i < active_cats; ++i) {
            if (point_in_rect(mx, my, rects.items[i])) {
                state.selected_category = i;
                return true;
            }
        }
        if (point_in_rect(mx, my, rects.panel)) return true;
    }
    return false;
}