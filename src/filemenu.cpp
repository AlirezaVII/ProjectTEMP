#include "filemenu.h"
#include "config.h"
#include "workspace.h"
#include <cstdio>

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

const char* MENU_ITEMS[] = {
    "New", "Open", "Save As"
};
static const int MENU_COUNT = 3;

void filemenu_layout(FileMenuRects &rects, int file_btn_x)
{
    rects.item_count = MENU_COUNT;
    rects.menu.x = file_btn_x;
    rects.menu.y = NAVBAR_HEIGHT;
    rects.menu.w = FILEMENU_WIDTH;
    rects.menu.h = MENU_COUNT * FILEMENU_ITEM_H;

    for (int i = 0; i < MENU_COUNT; ++i) {
        rects.items[i].x = rects.menu.x;
        rects.items[i].y = rects.menu.y + i * FILEMENU_ITEM_H;
        rects.items[i].w = FILEMENU_WIDTH;
        rects.items[i].h = FILEMENU_ITEM_H;
    }
}

void filemenu_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state,
                   const FileMenuRects &rects)
{
    if (!state.file_menu_open) return;

    SDL_SetRenderDrawColor(r, 0, 0, 0, 30);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_Rect shadow = rects.menu;
    shadow.x += 2; shadow.y += 2;
    SDL_RenderFillRect(r, &shadow);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    set_color(r, COL_FILEMENU_BG);
    SDL_RenderFillRect(r, &rects.menu);
    set_color(r, COL_FILEMENU_BORDER);
    SDL_RenderDrawRect(r, &rects.menu);

    for (int i = 0; i < rects.item_count; ++i) {
        if (state.file_menu_hover == i) {
            set_color(r, COL_FILEMENU_HOVER);
            SDL_RenderFillRect(r, &rects.items[i]);
        }
        int tx = rects.items[i].x + 12;
        int ty = rects.items[i].y + (FILEMENU_ITEM_H - 13) / 2;
        draw_text(r, font, MENU_ITEMS[i], tx, ty, COL_FILEMENU_TEXT);
    }
}

bool filemenu_handle_event(const SDL_Event &e, AppState &state,
                           const FileMenuRects &rects)
{
    if (!state.file_menu_open) return false;

    if (e.type == SDL_MOUSEMOTION) {
        int mx = e.motion.x;
        int my = e.motion.y;
        state.file_menu_hover = -1;
        for (int i = 0; i < rects.item_count; ++i) {
            if (point_in_rect(mx, my, rects.items[i])) {
                state.file_menu_hover = i;
                break;
            }
        }
        if (point_in_rect(mx, my, rects.menu)) return true;
        return false;
    }

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x;
        int my = e.button.y;

        for (int i = 0; i < rects.item_count; ++i) {
            if (point_in_rect(mx, my, rects.items[i])) {
                SDL_Log("File menu: %s", MENU_ITEMS[i]);

                if (i == 0) { /* New */
                    state.blocks.clear();
                    state.top_level_blocks.clear();
                    state.next_block_id = 1;
                    state.active_input = INPUT_NONE;
                    state.input_buffer.clear();
                } else if (i == 1) { /* Open */
                    SDL_Log("Open !");
                } else if (i == 2) { /* Save As */
                    SDL_Log("Saving !");
                }

                state.file_menu_open = false;
                state.file_menu_hover = -1;
                return true;
            }
        }

        state.file_menu_open = false;
        state.file_menu_hover = -1;
        if (point_in_rect(mx, my, rects.menu)) return true;
        return false;
    }

    return false;
}