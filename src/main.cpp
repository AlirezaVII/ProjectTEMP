#include "SDL.h"
#include "SDL_ttf.h"
#include "SDL_image.h"

#include "config.h"
#include "types.h"
#include "textures.h"
#include "navbar.h"
#include "filemenu.h"
#include "tab_bar.h"
#include "categories.h"
#include "palette.h"
#include "canvas.h"
#include "drag_area.h"
#include "stage.h"
#include "settings.h"
#include "workspace.h"
#include "costumes_tab.h"
#include "sounds_tab.h"
#include "sprite_panel.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>

int main(int /*argc*/, char* /*argv*/[])
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() != 0) {
        std::fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    int img_flags = IMG_INIT_PNG;
    if ((IMG_Init(img_flags) & img_flags) == 0) {
        std::fprintf(stderr, "IMG_Init: %s\n", IMG_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Scratch Clone",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!window) {
        std::fprintf(stderr, "Window: %s\n", SDL_GetError());
        IMG_Quit(); TTF_Quit(); SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::fprintf(stderr, "Renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit(); TTF_Quit(); SDL_Quit();
        return 1;
    }

    {
        int ww = 0, wh = 0;
        SDL_GetWindowSize(window, &ww, &wh);
        if (ww > 0 && wh > 0) {
            WINDOW_WIDTH = ww;
            WINDOW_HEIGHT = wh;
        }
        SDL_RenderSetLogicalSize(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);
    }

    TTF_Font *font = TTF_OpenFont("assets/fonts/NotoSans-Regular.ttf", 13);
    if (!font) {
        font = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 13);
    }
    if (!font) {
        font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 13);
    }
    if (!font) {
        std::fprintf(stderr, "Font load failed: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit(); TTF_Quit(); SDL_Quit();
        return 1;
    }

    Textures tex;
    textures_load(tex, renderer);

    NavbarRects     navbar_rects;      navbar_layout(navbar_rects);
    FileMenuRects   filemenu_rects;    filemenu_layout(filemenu_rects, navbar_rects.file_btn.x);
    TabBarRects     tab_bar_rects;     tab_bar_layout(tab_bar_rects);
    CategoriesRects cat_rects;         categories_layout(cat_rects);
    PaletteRects    pal_rects;         palette_layout(pal_rects);
    CanvasRects     canvas_rects;      canvas_layout(canvas_rects);
    DragAreaRects   drag_rects;        drag_area_layout(drag_rects);
    StageRects      stage_rects;       stage_layout(stage_rects);
    SettingsRects   settings_rects;    settings_layout(settings_rects);
    SpritePanelRects sprite_panel_rects; sprite_panel_layout(sprite_panel_rects);

    AppState state;
    SDL_StartTextInput();

    bool quit = false;
    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
                break;
            }
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE &&
                state.active_input == INPUT_NONE && !state.file_menu_open) {
                quit = true;
                break;
            }

/* --- always-active UI (regardless of tab) --- */
            if (filemenu_handle_event(e, state, filemenu_rects)) continue;
            if (navbar_handle_event(e, state, navbar_rects)) continue;
            if (tab_bar_handle_event(e, state, tab_bar_rects)) continue;
            if (settings_handle_event(e, state, settings_rects)) continue;
            
            // Pass tex into stage_handle_event!
            if (stage_handle_event(e, state, stage_rects, tex)) continue;
            
            if (sprite_panel_handle_event(e, state, sprite_panel_rects)) continue;
            /* --- tab-specific event handling --- */
            if (state.current_tab == TAB_CODE)
            {
                if (categories_handle_event(e, state, cat_rects)) continue;
                if (palette_handle_event(e, state, pal_rects, font)) continue;
                if (workspace_handle_event(e, state,
                               canvas_rects.panel,
                               pal_rects.panel,
                               font)) continue;
                if (canvas_handle_event(e, state, canvas_rects, pal_rects, font)) continue;
            }
            else if (state.current_tab == TAB_COSTUMES)
            {
                if (costumes_tab_handle_event(e, state)) continue;
            }
            else if (state.current_tab == TAB_SOUNDS)
            {
                if (sounds_tab_handle_event(e, state)) continue;
            }

            /* Fallback: commit active input on click elsewhere */
            /* Fallback: commit active input on click elsewhere */
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (state.active_input != INPUT_NONE &&
                    state.active_input != INPUT_PROJECT_NAME) {

                    if (state.active_input == INPUT_SPRITE_NAME) {
                        if (!state.input_buffer.empty())
                            state.sprite.name = state.input_buffer;
                    } else if (state.active_input == INPUT_X) {
                        if (!state.input_buffer.empty() && state.input_buffer != "-")
                            state.sprite.x = std::atoi(state.input_buffer.c_str());
                    } else if (state.active_input == INPUT_Y) {
                        if (!state.input_buffer.empty() && state.input_buffer != "-")
                            state.sprite.y = std::atoi(state.input_buffer.c_str());
                    } else if (state.active_input == INPUT_DIRECTION) {
                        if (!state.input_buffer.empty() && state.input_buffer != "-")
                            state.sprite.direction = std::atoi(state.input_buffer.c_str());
                    } else if (state.active_input == INPUT_SIZE) { 
                        if (!state.input_buffer.empty()) {
                            int s = std::atoi(state.input_buffer.c_str());
                            if (s < 0) s = 0;
                            if (s > 999) s = 999;
                            state.sprite.size = s;
                        }
                    }
                    else if (state.active_input == INPUT_BLOCK_FIELD) {
                        workspace_commit_active_input(state);
                    }

                    state.active_input = INPUT_NONE;
                    state.input_buffer.clear();
                }
            }
        }

        /* ===================== DRAW ===================== */
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        SDL_RenderClear(renderer);

        /* --- tab-specific drawing --- */
        if (state.current_tab == TAB_CODE)
        {
            drag_area_draw(renderer, font, state, drag_rects);
            canvas_draw(renderer, font, state, canvas_rects, tex);
            categories_draw(renderer, font, state, cat_rects);
            palette_draw(renderer, font, state, pal_rects, tex);
        }
        else if (state.current_tab == TAB_COSTUMES)
        {
            costumes_tab_draw(renderer, font, state);
        }
        else if (state.current_tab == TAB_SOUNDS)
        {
            sounds_tab_draw(renderer, font, state);
        }

        /* --- always-visible UI (right column + top bars) --- */
    stage_draw(renderer, font, state, stage_rects, tex);
        
        settings_draw(renderer, font, state, settings_rects, tex);
        sprite_panel_draw(renderer, font, state, tex, sprite_panel_rects);
        tab_bar_draw(renderer, font, state, tab_bar_rects, tex);
        navbar_draw(renderer, font, state, navbar_rects, tex);
        filemenu_draw(renderer, font, state, filemenu_rects);
        SDL_RenderPresent(renderer);
    }

    textures_free(tex);
    TTF_CloseFont(font);
    SDL_StopTextInput();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    return 0;
}