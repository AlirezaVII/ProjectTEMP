#include "navbar.h"
#include "config.h"
#include "renderer.h"
#include <cstring>
#include <string>

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

void navbar_layout(NavbarRects &rects)
{
    rects.bar.x = 0;
    rects.bar.y = 0;
    rects.bar.w = WINDOW_WIDTH;
    rects.bar.h = NAVBAR_HEIGHT;

    int x = NAVBAR_LOGO_MARGIN;

    rects.logo_rect.x = x;
    rects.logo_rect.y = (NAVBAR_HEIGHT - NAVBAR_LOGO_SIZE) / 2;
    rects.logo_rect.w = NAVBAR_LOGO_WIDTH;
    rects.logo_rect.h = NAVBAR_LOGO_HEIGHT;
    x += NAVBAR_LOGO_WIDTH + 10;

    rects.file_btn.x = x;
    rects.file_btn.y = 0;
    rects.file_btn.w = FILE_BTN_WIDTH;
    rects.file_btn.h = NAVBAR_HEIGHT;
    x += FILE_BTN_WIDTH + 10;

    rects.project_input.x = x;
    rects.project_input.y = (NAVBAR_HEIGHT - PROJECT_INPUT_HEIGHT) / 2;
    rects.project_input.w = PROJECT_INPUT_WIDTH;
    rects.project_input.h = PROJECT_INPUT_HEIGHT;
}

void navbar_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state,
                 const NavbarRects &rects, const Textures &tex)
{
    /* background */
    set_color(r, COL_NAVBAR_BG);
    SDL_RenderFillRect(r, &rects.bar);

    /* logo */
    if (tex.logo) {
        renderer_draw_texture_fit(r, tex.logo, &rects.logo_rect);
    } else {
        /* fallback circle */
        int cx = rects.logo_rect.x + rects.logo_rect.w / 2;
        int cy = rects.logo_rect.y + rects.logo_rect.h / 2;
        renderer_fill_circle(r, cx, cy, NAVBAR_LOGO_SIZE / 2,
                             255, 165, 0);
    }

    /* file button */
    if (state.file_menu_open) {
        set_color(r, COL_NAVBAR_FILE_HOVER);
        SDL_RenderFillRect(r, &rects.file_btn);
    }
    {
        int tx = rects.file_btn.x + (rects.file_btn.w - 22) / 2;
        int ty = (NAVBAR_HEIGHT - 13) / 2;
        draw_text(r, font, "File", tx, ty, COL_NAVBAR_TEXT);
    }

    /* project name input */
    set_color(r, COL_NAVBAR_INPUT_BG);
    SDL_RenderFillRect(r, &rects.project_input);
    if (state.active_input == INPUT_PROJECT_NAME) {
        set_color(r, COL_SETTINGS_INPUT_ACTIVE);
    } else {
        set_color(r, COL_NAVBAR_INPUT_BORDER);
    }
    SDL_RenderDrawRect(r, &rects.project_input);

    {
        std::string txt;
        if (state.active_input == INPUT_PROJECT_NAME) {
            txt = state.input_buffer + "|";
        } else {
            txt = state.project_name;
        }
        int tx = rects.project_input.x + 6;
        int ty = rects.project_input.y + (rects.project_input.h - 13) / 2;
        draw_text(r, font, txt.c_str(), tx, ty, COL_NAVBAR_INPUT_TEXT);
    }
}

static void commit_project_name(AppState &state)
{
    if (!state.input_buffer.empty()) {
        state.project_name = state.input_buffer;
    }
    state.active_input = INPUT_NONE;
    state.input_buffer.clear();
}

bool navbar_handle_event(const SDL_Event &e, AppState &state,
                         const NavbarRects &rects)
{
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x;
        int my = e.button.y;

        /* file button click */
        if (point_in_rect(mx, my, rects.file_btn)) {
            /* commit any active input */
            if (state.active_input == INPUT_PROJECT_NAME) {
                commit_project_name(state);
            }
            state.file_menu_open = !state.file_menu_open;
            state.file_menu_hover = -1;
            return true;
        }

        /* project name input click */
        if (point_in_rect(mx, my, rects.project_input)) {
            state.file_menu_open = false;
            state.active_input = INPUT_PROJECT_NAME;
            state.input_buffer = state.project_name;
            return true;
        }

        /* click on navbar elsewhere */
        if (point_in_rect(mx, my, rects.bar)) {
            if (state.active_input == INPUT_PROJECT_NAME) {
                commit_project_name(state);
            }
            state.file_menu_open = false;
            return true;
        }
    }

    /* keyboard handling for project name */
    if (state.active_input == INPUT_PROJECT_NAME) {
        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_RETURN ||
                e.key.keysym.sym == SDLK_KP_ENTER) {
                commit_project_name(state);
                return true;
            }
            if (e.key.keysym.sym == SDLK_ESCAPE) {
                state.active_input = INPUT_NONE;
                state.input_buffer.clear();
                return true;
            }
            if (e.key.keysym.sym == SDLK_BACKSPACE) {
                if (!state.input_buffer.empty()) {
                    state.input_buffer.erase(state.input_buffer.size() - 1);
                }
                return true;
            }
        }
        if (e.type == SDL_TEXTINPUT) {
            state.input_buffer += e.text.text;
            return true;
        }
    }

    return false;
}
