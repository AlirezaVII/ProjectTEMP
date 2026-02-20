#include "canvas.h"
#include "config.h"
#include "workspace.h"

static bool point_in_rect(int px, int py, const SDL_Rect &r)
{
    return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
}

static void set_color(SDL_Renderer *r, Color c)
{
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255);
}

void canvas_layout(CanvasRects &rects)
{
    int top = NAVBAR_HEIGHT + TAB_BAR_HEIGHT;
    int left = CATEGORY_WIDTH + PALETTE_WIDTH;
    int right = WINDOW_WIDTH - RIGHT_COLUMN_WIDTH;

    rects.panel.x = left;
    rects.panel.y = top;
    rects.panel.w = right - left;
    rects.panel.h = WINDOW_HEIGHT - top;
}

void canvas_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state,
                 const CanvasRects &rects, const Textures &tex)
{
    set_color(r, COL_CANVAS_BG);
    SDL_RenderFillRect(r, &rects.panel);

    set_color(r, COL_CANVAS_GRID);
    int spacing = 30;
    for (int x = rects.panel.x; x < rects.panel.x + rects.panel.w; x += spacing) {
        SDL_RenderDrawLine(r, x, rects.panel.y, x, rects.panel.y + rects.panel.h);
    }
    for (int y = rects.panel.y; y < rects.panel.y + rects.panel.h; y += spacing) {
        SDL_RenderDrawLine(r, rects.panel.x, y, rects.panel.x + rects.panel.w, y);
    }

    workspace_draw(r, font, tex, state, rects.panel, COL_CANVAS_BG);
}

bool canvas_handle_event(const SDL_Event &e, AppState &state,
                         const CanvasRects &rects,
                         const PaletteRects &pal_rects,
                         TTF_Font *font)
{
    if (e.type == SDL_MOUSEMOTION ||
        (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) ||
        (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) ||
        e.type == SDL_TEXTINPUT ||
        e.type == SDL_KEYDOWN) {

        if (state.drag.active) {
            return workspace_handle_event(e, state, rects.panel, pal_rects.panel, font);
        }

        if (e.type == SDL_MOUSEBUTTONDOWN) {
            if (!point_in_rect(e.button.x, e.button.y, rects.panel)) return false;
        }

        return workspace_handle_event(e, state, rects.panel, pal_rects.panel, font);
    }

    return false;
}