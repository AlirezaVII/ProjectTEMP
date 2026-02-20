#include "filemenu.h"
#include "config.h"
#include "renderer.h"

static bool point_in_rect(int px, int py, const SDL_Rect &r)
{
    return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
}

void filemenu_layout(FileMenuRects &rects, int file_btn_x)
{
    rects.menu.x = file_btn_x;
    rects.menu.y = NAVBAR_HEIGHT;
    rects.menu.w = 160;
    rects.menu.h = 3 * FILEMENU_ITEM_H;
    for (int i = 0; i < 3; ++i)
    {
        rects.items[i].x = rects.menu.x;
        rects.items[i].y = rects.menu.y + i * FILEMENU_ITEM_H;
        rects.items[i].w = rects.menu.w;
        rects.items[i].h = FILEMENU_ITEM_H;
    }
}

void filemenu_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state, const FileMenuRects &rects)
{
    if (!state.file_menu_open)
        return;
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderFillRect(r, &rects.menu);
    SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
    SDL_RenderDrawRect(r, &rects.menu);

    const char *labels[3] = {"New", "Load", "Save"};
    for (int i = 0; i < 3; ++i)
    {
        if (state.file_menu_hover == i)
        {
            SDL_SetRenderDrawColor(r, 240, 240, 240, 255);
            SDL_RenderFillRect(r, &rects.items[i]);
        }
        SDL_Color tc = {40, 40, 40, 255};
        SDL_Surface *s = TTF_RenderUTF8_Blended(font, labels[i], tc);
        if (s)
        {
            SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
            SDL_Rect td = {rects.items[i].x + 16, rects.items[i].y + (FILEMENU_ITEM_H - s->h) / 2, s->w, s->h};
            SDL_RenderCopy(r, t, NULL, &td);
            SDL_DestroyTexture(t);
            SDL_FreeSurface(s);
        }
    }
}

bool filemenu_handle_event(const SDL_Event &e, AppState &state, const FileMenuRects &rects)
{
    if (!state.file_menu_open)
        return false;
    if (e.type == SDL_MOUSEMOTION)
    {
        state.file_menu_hover = -1;
        for (int i = 0; i < 3; ++i)
        {
            if (point_in_rect(e.motion.x, e.motion.y, rects.items[i]))
            {
                state.file_menu_hover = i;
                break;
            }
        }
        return true;
    }
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        for (int i = 0; i < 3; ++i)
        {
            if (point_in_rect(e.button.x, e.button.y, rects.items[i]))
            {
                if (i == 0)
                { /* New */
                    for (auto &spr : state.sprites)
                    {
                        spr.blocks.clear();
                        spr.top_level_blocks.clear();
                    }
                    state.next_block_id = 1;
                    state.active_input = INPUT_NONE;
                    state.input_buffer.clear();
                }
                else if (i == 1)
                { /* Load */
                }
                else if (i == 2)
                { /* Save */
                }
                state.file_menu_open = false;
                state.file_menu_hover = -1;
                return true;
            }
        }
        state.file_menu_open = false;
        state.file_menu_hover = -1;
        return true;
    }
    return false;
}