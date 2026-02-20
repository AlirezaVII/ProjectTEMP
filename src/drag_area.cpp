#include "drag_area.h"
#include "config.h"

static void set_color(SDL_Renderer *r, Color c)
{
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255);
}

void drag_area_layout(DragAreaRects &rects)
{
    int top = NAVBAR_HEIGHT + TAB_BAR_HEIGHT;
    int left = CATEGORY_WIDTH + PALETTE_WIDTH;
    int right = WINDOW_WIDTH - RIGHT_COLUMN_WIDTH;

    rects.panel.x = left;
    rects.panel.y = top;
    rects.panel.w = right - left;
    rects.panel.h = WINDOW_HEIGHT - top;
}

void drag_area_draw(SDL_Renderer *r, TTF_Font * /*font*/,
                    const AppState & /*state*/, const DragAreaRects &rects)
{
    set_color(r, COL_CANVAS_BG);
    SDL_RenderFillRect(r, &rects.panel);

    /* grid */
    set_color(r, COL_CANVAS_GRID);
    int spacing = 30;
    for (int x = rects.panel.x; x < rects.panel.x + rects.panel.w; x += spacing) {
        SDL_RenderDrawLine(r, x, rects.panel.y, x, rects.panel.y + rects.panel.h);
    }
    for (int y = rects.panel.y; y < rects.panel.y + rects.panel.h; y += spacing) {
        SDL_RenderDrawLine(r, rects.panel.x, y, rects.panel.x + rects.panel.w, y);
    }
}
