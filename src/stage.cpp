#include "stage.h"
#include "config.h"
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

void stage_layout(StageRects &rects)
{
    int col_x = WINDOW_WIDTH - RIGHT_COLUMN_WIDTH;
    int col_h = WINDOW_HEIGHT - NAVBAR_HEIGHT;
    int stage_h = col_h * STAGE_HEIGHT_RATIO / 100;

    rects.panel.x = col_x;
    rects.panel.y = NAVBAR_HEIGHT;
    rects.panel.w = RIGHT_COLUMN_WIDTH;
    rects.panel.h = stage_h;

    int margin = 8;
    rects.stage_area.x = col_x + margin;
    rects.stage_area.y = NAVBAR_HEIGHT + margin;
    rects.stage_area.w = RIGHT_COLUMN_WIDTH - margin * 2;
    rects.stage_area.h = stage_h - margin * 2;
}

void stage_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state,
                const StageRects &rects)
{
    /* panel bg */
    set_color(r, COL_STAGE_BG);
    SDL_RenderFillRect(r, &rects.panel);

    /* border */
    set_color(r, COL_STAGE_BORDER);
    SDL_RenderDrawRect(r, &rects.stage_area);

    /* sprite indicator (small dot at sprite position inside stage) */
    {
        int cx = rects.stage_area.x + rects.stage_area.w / 2 + state.sprite.x;
        int cy = rects.stage_area.y + rects.stage_area.h / 2 - state.sprite.y;

        /* clamp inside stage */
        if (cx >= rects.stage_area.x && cx <= rects.stage_area.x + rects.stage_area.w &&
            cy >= rects.stage_area.y && cy <= rects.stage_area.y + rects.stage_area.h)
        {
            if (state.sprite.visible) {
                renderer_fill_circle(r, cx, cy, 6, 76, 151, 255);
                renderer_draw_circle(r, cx, cy, 6, 50, 100, 200);
            }
        }
    }

    /* stage label */
    draw_text(r, font, "Stage",
              rects.stage_area.x + rects.stage_area.w / 2 - 18,
              rects.stage_area.y + rects.stage_area.h - 20,
              COL_STAGE_TEXT);
}

bool stage_handle_event(const SDL_Event &e, AppState & /*state*/,
                        const StageRects &rects)
{
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        if (point_in_rect(e.button.x, e.button.y, rects.panel))
            return true;
    }
    return false;
}
