#include "costumes_tab.h"
#include "config.h"

static void draw_text_centered(SDL_Renderer *r, TTF_Font *font, const char *txt,
                                int cx, int cy, Uint8 cr, Uint8 cg, Uint8 cb)
{
    SDL_Color col;
    col.r = cr; col.g = cg; col.b = cb; col.a = 255;
    SDL_Surface *s = TTF_RenderUTF8_Blended(font, txt, col);
    if (!s) return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
    SDL_Rect dst;
    dst.x = cx - s->w / 2;
    dst.y = cy - s->h / 2;
    dst.w = s->w;
    dst.h = s->h;
    SDL_RenderCopy(r, t, NULL, &dst);
    SDL_DestroyTexture(t);
    SDL_FreeSurface(s);
}

void costumes_tab_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state)
{
    (void)state;

    int area_x = 0;
    int area_y = NAVBAR_HEIGHT + TAB_BAR_HEIGHT;
    int area_w = WINDOW_WIDTH - RIGHT_COLUMN_WIDTH;
    int area_h = WINDOW_HEIGHT - area_y;

    /* white background */
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_Rect bg;
    bg.x = area_x;
    bg.y = area_y;
    bg.w = area_w;
    bg.h = area_h;
    SDL_RenderFillRect(r, &bg);

    /* separator line at top */
    SDL_SetRenderDrawColor(r, 220, 220, 220, 255);
    SDL_RenderDrawLine(r, area_x, area_y, area_x + area_w, area_y);

    /* placeholder text */
    int cx = area_x + area_w / 2;
    int cy = area_y + area_h / 2;

    draw_text_centered(r, font, "Costumes", cx, cy - 15, 100, 100, 100);
    draw_text_centered(r, font, "(Coming Soon)", cx, cy + 15, 170, 170, 170);
}

bool costumes_tab_handle_event(const SDL_Event &e, AppState &state)
{
    (void)e;
    (void)state;
    return false;
}
