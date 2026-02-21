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
#include "renderer.h"
#include "interpreter.h"
#include "audio.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <ctime>

static std::vector<std::pair<std::string, SDL_Texture *>> global_sprite_lib;
static std::vector<std::pair<std::string, SDL_Texture *>> global_backdrop_lib;

static void load_library_sprites(SDL_Renderer *renderer)
{
    const char *lib_files[] = {"businessman.png", "car.png", "crab.png", "soldier.png", "call_center.png", "celebrater.png", "working.png"};
    const char *lib_names[] = {"businessman", "car", "crab", "soldier", "IT guy", "celebrater", "working"};
    for (int i = 0; i < 7; ++i)
    {
        std::string path = "assets/sprites/" + std::string(lib_files[i]);
        SDL_Texture *t = IMG_LoadTexture(renderer, path.c_str());
        if (t)
            global_sprite_lib.push_back({lib_names[i], t});
    }
}

static void load_library_backdrops(SDL_Renderer *renderer)
{
    const char *lib_files[] = {"cage.png", "city.png", "geometric.png", "sky.png", "traffic.png", "voronoimal.png", "waves.png"};
    const char *lib_names[] = {"cage", "city", "geometric", "sky", "traffic", "voronoimal", "waves"};
    for (int i = 0; i < 7; ++i)
    {
        std::string path = "assets/backdrops/" + std::string(lib_files[i]);
        SDL_Texture *t = IMG_LoadTexture(renderer, path.c_str());
        if (t)
            global_backdrop_lib.push_back({lib_names[i], t});
    }
}

static void render_simple_text(SDL_Renderer *r, TTF_Font *font, const char *text, int x, int y, Color c)
{
    if (!text || text[0] == '\0')
        return;
    SDL_Color sc = {static_cast<Uint8>(c.r), static_cast<Uint8>(c.g), static_cast<Uint8>(c.b), 255};
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, sc);
    if (surf)
    {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
        SDL_Rect dst = {x, y, surf->w, surf->h};
        SDL_RenderCopy(r, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
}

int main(int /*argc*/, char * /*argv*/[])
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        return 1;
    if (TTF_Init() != 0)
    {
        SDL_Quit();
        return 1;
    }
    int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(img_flags) & img_flags) == 0)
    {
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Scratch Clone", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window)
        return 1;

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
        return 1;

    int ww = 0, wh = 0;
    SDL_GetWindowSize(window, &ww, &wh);
    if (ww > 0 && wh > 0)
    {
        WINDOW_WIDTH = ww;
        WINDOW_HEIGHT = wh;
    }
    SDL_RenderSetLogicalSize(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);

    TTF_Font *font = TTF_OpenFont("assets/fonts/NotoSans-Regular.ttf", 13);
    if (!font)
        font = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 13);
    if (!font)
        font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 13);
    if (!font)
        return 1;

    TTF_Font *font_large = TTF_OpenFont("assets/fonts/NotoSans-Regular.ttf", 20);
    if (!font_large)
        font_large = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 20);
    if (!font_large)
        font_large = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 20);
    if (!font_large)
        return 1;

    Textures tex;
    textures_load(tex, renderer);
    if (!audio_init())
        std::fprintf(stderr, "Warning: Audio failed to load. Sounds will be silent.\n");

    load_library_sprites(renderer);
    load_library_backdrops(renderer);
    renderer_init_pen_layer(renderer);
    SDL_Texture *pen_poster = IMG_LoadTexture(renderer, "assets/extensions/pencil.png");

    NavbarRects navbar_rects;
    navbar_layout(navbar_rects);
    FileMenuRects filemenu_rects;
    filemenu_layout(filemenu_rects, navbar_rects.file_btn.x);
    TabBarRects tab_bar_rects;
    tab_bar_layout(tab_bar_rects);
    CategoriesRects cat_rects;
    categories_layout(cat_rects);
    PaletteRects pal_rects;
    palette_layout(pal_rects);
    CanvasRects canvas_rects;
    canvas_layout(canvas_rects);
    DragAreaRects drag_rects;
    drag_area_layout(drag_rects);
    StageRects stage_rects;
    stage_layout(stage_rects);
    SettingsRects settings_rects;
    settings_layout(settings_rects);
    SpritePanelRects sprite_panel_rects;
    sprite_panel_layout(sprite_panel_rects);

    AppState state;
    state.sprites.push_back(Sprite("Sprite1", tex.scratch_cat));

    Mix_Chunk *def_snd = audio_load_sound("assets/sounds/meow.wav");
    if (def_snd)
        state.sprites[0].sounds.push_back(SoundData("meow", def_snd));
    state.backdrops.push_back(Backdrop("backdrop1", nullptr));

    SDL_StartTextInput();
    bool quit = false;

    while (!quit)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                quit = true;
                break;
            }

            if (state.ask_active)
            {
                if (e.type == SDL_KEYDOWN)
                {
                    if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER)
                    {
                        state.global_answer = state.ask_reply;
                        state.ask_active = false;
                    }
                    else if (e.key.keysym.sym == SDLK_BACKSPACE)
                    {
                        if (!state.ask_reply.empty())
                            state.ask_reply.pop_back();
                    }
                }
                else if (e.type == SDL_TEXTINPUT)
                {
                    state.ask_reply += e.text.text;
                }
                continue;
            }

            if (state.var_modal_active)
            {
                if (e.type == SDL_KEYDOWN)
                {
                    if (e.key.keysym.sym == SDLK_ESCAPE)
                    {
                        state.var_modal_active = false;
                        state.active_input = INPUT_NONE;
                        state.input_buffer.clear();
                    }
                    else if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER)
                    {
                        bool unique = true;
                        for (const auto &v : state.variables)
                            if (v == state.input_buffer)
                            {
                                unique = false;
                                break;
                            }
                        if (unique && !state.input_buffer.empty())
                        {
                            state.variables.push_back(state.input_buffer);
                            state.variable_values[state.input_buffer] = "0";
                            state.variable_visible[state.input_buffer] = true;
                        }
                        state.var_modal_active = false;
                        state.active_input = INPUT_NONE;
                        state.input_buffer.clear();
                    }
                    else if (e.key.keysym.sym == SDLK_BACKSPACE)
                    {
                        if (!state.input_buffer.empty())
                            state.input_buffer.pop_back();
                    }
                }
                else if (e.type == SDL_TEXTINPUT)
                {
                    state.input_buffer += e.text.text;
                }
                else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
                {
                    int mw = 400, mh = 200, mx = WINDOW_WIDTH / 2 - mw / 2, my = WINDOW_HEIGHT / 2 - mh / 2;
                    SDL_Rect submit_btn = {mx + mw - 120, my + mh - 60, 90, 40};
                    SDL_Rect cancel_btn = {mx + mw - 220, my + mh - 60, 90, 40};
                    auto in_rect = [](int px, int py, const SDL_Rect &r)
                    { return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h; };
                    if (in_rect(e.button.x, e.button.y, submit_btn))
                    {
                        bool unique = true;
                        for (const auto &v : state.variables)
                            if (v == state.input_buffer)
                            {
                                unique = false;
                                break;
                            }
                        if (unique && !state.input_buffer.empty())
                        {
                            state.variables.push_back(state.input_buffer);
                            state.variable_values[state.input_buffer] = "0";
                            state.variable_visible[state.input_buffer] = true;
                        }
                        state.var_modal_active = false;
                        state.active_input = INPUT_NONE;
                        state.input_buffer.clear();
                    }
                    else if (in_rect(e.button.x, e.button.y, cancel_btn))
                    {
                        state.var_modal_active = false;
                        state.active_input = INPUT_NONE;
                        state.input_buffer.clear();
                    }
                }
                continue;
            }

            if (state.active_input == INPUT_PEN_COLOR_PICKER)
            {
                if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
                {
                    int mx = e.button.x, my = e.button.y;
                    int mw = 300, mh = 250, mod_x = WINDOW_WIDTH / 2 - mw / 2, mod_y = WINDOW_HEIGHT / 2 - mh / 2;
                    bool clicked_color = false;
                    Color palette[9] = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 0}, {0, 255, 255}, {255, 0, 255}, {0, 0, 0}, {128, 128, 128}, {255, 255, 255}};
                    for (int i = 0; i < 9; i++)
                    {
                        SDL_Rect c_rect = {mod_x + 30 + (i % 3) * 80, mod_y + 80 + (i / 3) * 50, 70, 40};
                        if (mx >= c_rect.x && mx < c_rect.x + c_rect.w && my >= c_rect.y && my < c_rect.y + c_rect.h)
                        {
                            if (state.current_tab == TAB_COSTUMES)
                            {
                                state.active_color = {(Uint8)palette[i].r, (Uint8)palette[i].g, (Uint8)palette[i].b, 255};
                            }
                            else if (state.selected_sprite >= 0 && state.selected_sprite < (int)state.sprites.size())
                            {
                                state.sprites[state.selected_sprite].pen_color = {(Uint8)palette[i].r, (Uint8)palette[i].g, (Uint8)palette[i].b, 255};
                            }
                            state.active_input = INPUT_NONE;
                            clicked_color = true;
                            break;
                        }
                    }
                    if (!clicked_color && (mx < mod_x || mx > mod_x + mw || my < mod_y || my > mod_y + mh))
                        state.active_input = INPUT_NONE;
                }
                continue;
            }

            if (state.active_input == INPUT_COSTUME_TEXT)
            {
                bool is_key = false;
                if (e.type == SDL_KEYDOWN)
                {
                    if (e.key.keysym.sym == SDLK_BACKSPACE && !state.input_buffer.empty())
                        state.input_buffer.pop_back();
                    else if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_ESCAPE)
                    {
                        state.active_input = INPUT_NONE;
                    }
                    is_key = true;
                }
                else if (e.type == SDL_TEXTINPUT)
                {
                    state.input_buffer += e.text.text;
                    is_key = true;
                }

                if (is_key)
                {
                    GraphicItem *item = nullptr;
                    if (state.editing_target_is_stage && state.selected_backdrop >= 0 && state.selected_backdrop < (int)state.backdrops.size())
                        item = &state.backdrops[state.selected_backdrop];
                    else if (!state.editing_target_is_stage && state.selected_sprite >= 0 && state.selected_sprite < (int)state.sprites.size())
                    {
                        auto &spr = state.sprites[state.selected_sprite];
                        if (spr.selected_costume >= 0 && spr.selected_costume < (int)spr.costumes.size())
                            item = &spr.costumes[spr.selected_costume];
                    }
                    if (item && state.active_shape_index >= 0 && state.active_shape_index < (int)item->shapes.size())
                    {
                        item->shapes[state.active_shape_index].text = state.input_buffer;
                    }
                    continue;
                }
            }
            else if (state.active_input == INPUT_SOUND_NAME || state.active_input == INPUT_SOUND_VOLUME || state.active_input == INPUT_COSTUME_NAME)
            {
                if (e.type == SDL_KEYDOWN)
                {
                    if (e.key.keysym.sym == SDLK_BACKSPACE)
                    {
                        if (!state.input_buffer.empty())
                            state.input_buffer.pop_back();
                        continue;
                    }
                    else if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER || e.key.keysym.sym == SDLK_ESCAPE)
                    {
                        if (state.active_input == INPUT_COSTUME_NAME && !state.input_buffer.empty())
                        {
                            if (state.editing_target_is_stage)
                            {
                                if (state.selected_backdrop >= 0 && state.selected_backdrop < (int)state.backdrops.size())
                                    state.backdrops[state.selected_backdrop].name = state.input_buffer;
                            }
                            else
                            {
                                if (state.selected_sprite >= 0 && state.selected_sprite < (int)state.sprites.size())
                                {
                                    auto &spr = state.sprites[state.selected_sprite];
                                    if (spr.selected_costume >= 0 && spr.selected_costume < (int)spr.costumes.size())
                                        spr.costumes[spr.selected_costume].name = state.input_buffer;
                                }
                            }
                        }
                        else if (state.active_input == INPUT_SOUND_NAME && !state.input_buffer.empty())
                        {
                            if (state.selected_sprite >= 0 && state.selected_sprite < (int)state.sprites.size())
                            {
                                auto &spr = state.sprites[state.selected_sprite];
                                if (spr.selected_sound >= 0 && spr.selected_sound < (int)spr.sounds.size())
                                    spr.sounds[spr.selected_sound].name = state.input_buffer;
                            }
                        }
                        else if (state.active_input == INPUT_SOUND_VOLUME && !state.input_buffer.empty())
                        {
                            if (state.selected_sprite >= 0 && state.selected_sprite < (int)state.sprites.size())
                            {
                                auto &spr = state.sprites[state.selected_sprite];
                                if (spr.selected_sound >= 0 && spr.selected_sound < (int)spr.sounds.size())
                                {
                                    int v = std::max(0, std::min(std::atoi(state.input_buffer.c_str()), 100));
                                    spr.sounds[spr.selected_sound].volume = v;
                                    if (v > 0)
                                        spr.sounds[spr.selected_sound].prev_volume = v;
                                    audio_set_volume(v);
                                }
                            }
                        }
                        state.active_input = INPUT_NONE;
                        state.input_buffer.clear();
                        continue;
                    }
                }
                else if (e.type == SDL_TEXTINPUT)
                {
                    state.input_buffer += e.text.text;
                    continue;
                }
            }

            if (state.mode == MODE_EXTENSION_LIBRARY)
            {
                if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
                {
                    int mx = e.button.x, my = e.button.y;
                    if (my < NAVBAR_HEIGHT)
                    {
                        state.mode = MODE_EDITOR;
                    }
                    int box_w = WINDOW_WIDTH / 3, box_h = 320, box_x = 60, box_y = NAVBAR_HEIGHT + 40;
                    if (mx >= box_x && mx < box_x + box_w && my >= box_y && my < box_y + box_h)
                    {
                        state.pen_extension_enabled = true;
                        state.selected_category = 9;
                        state.mode = MODE_EDITOR;
                    }
                }
                continue;
            }

            if (state.mode == MODE_SPRITE_LIBRARY)
            {
                if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
                {
                    int mx = e.button.x, my = e.button.y;
                    if (mx >= 10 && mx <= 120 && my >= 10 && my <= 60)
                        state.mode = MODE_EDITOR;
                    for (size_t i = 0; i < global_sprite_lib.size(); i++)
                    {
                        int x = 50 + i * 180, y = 150;
                        if (mx >= x && mx <= x + 160 && my >= y && my <= y + 160)
                        {
                            std::string new_name = global_sprite_lib[i].first;
                            int count = 1;
                            for (auto &s : state.sprites)
                                if (s.name.find(new_name) != std::string::npos)
                                    count++;
                            if (count > 1)
                                new_name += std::to_string(count);
                            state.sprites.push_back(Sprite(new_name, global_sprite_lib[i].second));
                            Mix_Chunk *ds = audio_load_sound("assets/sounds/meow.wav");
                            if (ds)
                                state.sprites.back().sounds.push_back(SoundData("meow", ds));
                            state.selected_sprite = state.sprites.size() - 1;
                            state.editing_target_is_stage = false;
                            state.mode = MODE_EDITOR;
                            break;
                        }
                    }
                }
                continue;
            }

            if (state.mode == MODE_BACKDROP_LIBRARY)
            {
                if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
                {
                    int mx = e.button.x, my = e.button.y;
                    if (mx >= 10 && mx <= 120 && my >= 10 && my <= 60)
                        state.mode = MODE_EDITOR;
                    for (size_t i = 0; i < global_backdrop_lib.size(); i++)
                    {
                        int x = 50 + i * 180, y = 150;
                        if (mx >= x && mx <= x + 160 && my >= y && my <= y + 160)
                        {
                            state.backdrops.push_back(Backdrop(global_backdrop_lib[i].first, global_backdrop_lib[i].second));
                            state.selected_backdrop = state.backdrops.size() - 1;
                            state.editing_target_is_stage = true;
                            state.mode = MODE_EDITOR;
                            break;
                        }
                    }
                }
                continue;
            }

            if (e.type == SDL_KEYDOWN && state.active_input == INPUT_NONE && !state.file_menu_open && !state.var_modal_active)
                interpreter_trigger_key(state, e.key.keysym.sym);
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE && state.active_input == INPUT_NONE && !state.file_menu_open)
            {
                quit = true;
                break;
            }

            if (state.sprite_menu_open && e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
            {
                int mx = e.button.x, my = e.button.y;
                bool consumed_menu = false;
                if (mx >= sprite_panel_rects.sprite_menu_items[0].x && mx <= sprite_panel_rects.sprite_menu_items[0].x + 36 && my >= sprite_panel_rects.sprite_menu_items[0].y && my <= sprite_panel_rects.sprite_menu_items[0].y + 36)
                {
                    char buffer[1024];
                    std::string result = "";
                    FILE *pipe = popen("osascript -e 'POSIX path of (choose file with prompt \"Choose Sprite\" of type {\"png\", \"jpg\", \"jpeg\"})'", "r");
                    if (pipe)
                    {
                        while (fgets(buffer, sizeof(buffer), pipe) != NULL)
                            result += buffer;
                        pclose(pipe);
                    }
                    if (!result.empty() && result.back() == '\n')
                        result.pop_back();
                    if (!result.empty())
                    {
                        SDL_Texture *t = IMG_LoadTexture(renderer, result.c_str());
                        if (t)
                        {
                            size_t slash = result.find_last_of('/');
                            std::string fname = (slash == std::string::npos) ? result : result.substr(slash + 1);
                            size_t dot = fname.find_last_of('.');
                            if (dot != std::string::npos)
                                fname = fname.substr(0, dot);
                            state.sprites.push_back(Sprite(fname, t));
                            Mix_Chunk *ds = audio_load_sound("assets/sounds/meow.wav");
                            if (ds)
                                state.sprites.back().sounds.push_back(SoundData("meow", ds));
                            state.selected_sprite = state.sprites.size() - 1;
                            state.editing_target_is_stage = false;
                        }
                    }
                    state.sprite_menu_open = false;
                    consumed_menu = true;
                }
                else if (mx >= sprite_panel_rects.sprite_menu_items[1].x && mx <= sprite_panel_rects.sprite_menu_items[1].x + 36 && my >= sprite_panel_rects.sprite_menu_items[1].y && my <= sprite_panel_rects.sprite_menu_items[1].y + 36)
                {
                    if (!global_sprite_lib.empty())
                    {
                        int r_idx = std::rand() % global_sprite_lib.size();
                        std::string new_name = global_sprite_lib[r_idx].first;
                        int count = 1;
                        for (auto &s : state.sprites)
                            if (s.name.find(new_name) != std::string::npos)
                                count++;
                        if (count > 1)
                            new_name += std::to_string(count);
                        state.sprites.push_back(Sprite(new_name, global_sprite_lib[r_idx].second));
                        Mix_Chunk *ds = audio_load_sound("assets/sounds/meow.wav");
                        if (ds)
                            state.sprites.back().sounds.push_back(SoundData("meow", ds));
                        state.selected_sprite = state.sprites.size() - 1;
                        state.editing_target_is_stage = false;
                    }
                    state.sprite_menu_open = false;
                    consumed_menu = true;
                }
                else if (mx >= sprite_panel_rects.sprite_menu_items[2].x && mx <= sprite_panel_rects.sprite_menu_items[2].x + 36 && my >= sprite_panel_rects.sprite_menu_items[2].y && my <= sprite_panel_rects.sprite_menu_items[2].y + 36)
                {
                    state.mode = MODE_SPRITE_LIBRARY;
                    state.sprite_menu_open = false;
                    consumed_menu = true;
                }
                if (consumed_menu)
                    continue;
            }

            if (state.backdrop_menu_open && e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
            {
                int mx = e.button.x, my = e.button.y;
                bool consumed_menu = false;
                if (mx >= sprite_panel_rects.backdrop_menu_items[0].x && mx <= sprite_panel_rects.backdrop_menu_items[0].x + 36 && my >= sprite_panel_rects.backdrop_menu_items[0].y && my <= sprite_panel_rects.backdrop_menu_items[0].y + 36)
                {
                    char buffer[1024];
                    std::string result = "";
                    FILE *pipe = popen("osascript -e 'POSIX path of (choose file with prompt \"Choose Backdrop\" of type {\"png\", \"jpg\", \"jpeg\"})'", "r");
                    if (pipe)
                    {
                        while (fgets(buffer, sizeof(buffer), pipe) != NULL)
                            result += buffer;
                        pclose(pipe);
                    }
                    if (!result.empty() && result.back() == '\n')
                        result.pop_back();
                    if (!result.empty())
                    {
                        SDL_Texture *t = IMG_LoadTexture(renderer, result.c_str());
                        if (t)
                        {
                            size_t slash = result.find_last_of('/');
                            std::string fname = (slash == std::string::npos) ? result : result.substr(slash + 1);
                            size_t dot = fname.find_last_of('.');
                            if (dot != std::string::npos)
                                fname = fname.substr(0, dot);
                            state.backdrops.push_back(Backdrop(fname, t));
                            state.selected_backdrop = state.backdrops.size() - 1;
                            state.editing_target_is_stage = true;
                        }
                    }
                    state.backdrop_menu_open = false;
                    consumed_menu = true;
                }
                else if (mx >= sprite_panel_rects.backdrop_menu_items[1].x && mx <= sprite_panel_rects.backdrop_menu_items[1].x + 36 && my >= sprite_panel_rects.backdrop_menu_items[1].y && my <= sprite_panel_rects.backdrop_menu_items[1].y + 36)
                {
                    if (!global_backdrop_lib.empty())
                    {
                        int r_idx = std::rand() % global_backdrop_lib.size();
                        state.backdrops.push_back(Backdrop(global_backdrop_lib[r_idx].first, global_backdrop_lib[r_idx].second));
                        state.selected_backdrop = state.backdrops.size() - 1;
                        state.editing_target_is_stage = true;
                    }
                    state.backdrop_menu_open = false;
                    consumed_menu = true;
                }
                else if (mx >= sprite_panel_rects.backdrop_menu_items[2].x && mx <= sprite_panel_rects.backdrop_menu_items[2].x + 36 && my >= sprite_panel_rects.backdrop_menu_items[2].y && my <= sprite_panel_rects.backdrop_menu_items[2].y + 36)
                {
                    state.mode = MODE_BACKDROP_LIBRARY;
                    state.backdrop_menu_open = false;
                    consumed_menu = true;
                }
                if (consumed_menu)
                    continue;
            }

            // ---> FIXED: GLOBAL UNFOCUS ENGINE (Saves inputs if you click ANYWHERE else) <---
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
            {
                if (state.active_input != INPUT_NONE && state.active_input != INPUT_PROJECT_NAME && state.active_input != INPUT_COSTUME_TEXT)
                {
                    if (state.selected_sprite >= 0 && state.selected_sprite < (int)state.sprites.size())
                    {
                        Sprite &spr = state.sprites[state.selected_sprite];
                        if (state.active_input == INPUT_SPRITE_NAME)
                        {
                            if (!state.input_buffer.empty())
                                spr.name = state.input_buffer;
                        }
                        else if (state.active_input == INPUT_X)
                        {
                            if (!state.input_buffer.empty() && state.input_buffer != "-")
                                spr.x = std::atoi(state.input_buffer.c_str());
                        }
                        else if (state.active_input == INPUT_Y)
                        {
                            if (!state.input_buffer.empty() && state.input_buffer != "-")
                                spr.y = std::atoi(state.input_buffer.c_str());
                        }
                        else if (state.active_input == INPUT_DIRECTION)
                        {
                            if (!state.input_buffer.empty() && state.input_buffer != "-")
                                spr.direction = std::atoi(state.input_buffer.c_str());
                        }
                        else if (state.active_input == INPUT_SIZE)
                        {
                            if (!state.input_buffer.empty())
                            {
                                int s = std::atoi(state.input_buffer.c_str());
                                spr.size = std::max(0, std::min(s, 999));
                            }
                        }
                        else if (state.active_input == INPUT_SOUND_NAME)
                        {
                            if (!state.input_buffer.empty() && spr.selected_sound >= 0 && spr.selected_sound < (int)spr.sounds.size())
                                spr.sounds[spr.selected_sound].name = state.input_buffer;
                        }
                        else if (state.active_input == INPUT_SOUND_VOLUME)
                        {
                            if (!state.input_buffer.empty() && spr.selected_sound >= 0 && spr.selected_sound < (int)spr.sounds.size())
                            {
                                int v = std::max(0, std::min(std::atoi(state.input_buffer.c_str()), 100));
                                spr.sounds[spr.selected_sound].volume = v;
                                if (v > 0)
                                    spr.sounds[spr.selected_sound].prev_volume = v;
                                audio_set_volume(v);
                            }
                        }
                        else if (state.active_input == INPUT_COSTUME_NAME)
                        {
                            if (!state.input_buffer.empty())
                            {
                                if (state.editing_target_is_stage)
                                {
                                    if (state.selected_backdrop >= 0 && state.selected_backdrop < (int)state.backdrops.size())
                                        state.backdrops[state.selected_backdrop].name = state.input_buffer;
                                }
                                else
                                {
                                    if (spr.selected_costume >= 0 && spr.selected_costume < (int)spr.costumes.size())
                                        spr.costumes[spr.selected_costume].name = state.input_buffer;
                                }
                            }
                        }
                    }
                    if (state.active_input == INPUT_BLOCK_FIELD)
                        workspace_commit_active_input(state);

                    state.active_input = INPUT_NONE;
                    state.input_buffer.clear();
                }
            }

            if (filemenu_handle_event(e, state, filemenu_rects))
                continue;
            if (navbar_handle_event(e, state, navbar_rects))
                continue;
            if (tab_bar_handle_event(e, state, tab_bar_rects))
                continue;
            if (settings_handle_event(e, state, settings_rects))
                continue;
            if (stage_handle_event(e, state, stage_rects, tex))
                continue;
            if (sprite_panel_handle_event(e, state, sprite_panel_rects))
                continue;

            if (state.current_tab == TAB_CODE)
            {
                if (categories_handle_event(e, state, cat_rects))
                    continue;
                if (palette_handle_event(e, state, pal_rects, font))
                    continue;
                if (workspace_handle_event(e, state, canvas_rects.panel, pal_rects.panel, font))
                    continue;
                if (canvas_handle_event(e, state, canvas_rects, pal_rects, font))
                    continue;
            }
            else if (state.current_tab == TAB_COSTUMES)
            {
                if (costumes_tab_handle_event(e, state, renderer, font))
                    continue;
            }
            else if (state.current_tab == TAB_SOUNDS)
            {
                if (sounds_tab_handle_event(e, state))
                    continue;
            }
        }

        if (state.trigger_costume_import)
        {
            state.trigger_costume_import = false;
            char buffer[2048];
            std::string result = "";
            FILE *pipe = popen("osascript -e 'set f to choose file with prompt \"Choose\" of type {\"png\", \"jpg\", \"jpeg\"} multiple selections allowed true' -e 'set p to \"\"' -e 'repeat with i in f' -e 'set p to p & POSIX path of i & \"\\n\"' -e 'end repeat' -e 'p'", "r");
            if (pipe)
            {
                while (fgets(buffer, sizeof(buffer), pipe) != NULL)
                    result += buffer;
                pclose(pipe);
            }
            size_t pos = 0;
            while ((pos = result.find('\n')) != std::string::npos)
            {
                std::string path = result.substr(0, pos);
                result.erase(0, pos + 1);
                if (!path.empty())
                {
                    SDL_Texture *t = IMG_LoadTexture(renderer, path.c_str());
                    if (t)
                    {
                        size_t slash = path.find_last_of('/');
                        std::string fname = (slash == std::string::npos) ? path : path.substr(slash + 1);
                        size_t dot = fname.find_last_of('.');
                        if (dot != std::string::npos)
                            fname = fname.substr(0, dot);
                        if (state.editing_target_is_stage)
                        {
                            state.backdrops.push_back(Backdrop(fname, t));
                            state.selected_backdrop = state.backdrops.size() - 1;
                        }
                        else
                        {
                            if (state.selected_sprite >= 0 && state.selected_sprite < (int)state.sprites.size())
                            {
                                auto &spr = state.sprites[state.selected_sprite];
                                spr.costumes.push_back(Costume(fname, t));
                                spr.selected_costume = spr.costumes.size() - 1;
                            }
                        }
                    }
                }
            }
        }

        interpreter_tick(state);

        for (auto &spr : state.sprites)
        {
            if (!spr.costumes.empty() && spr.selected_costume >= 0 && spr.selected_costume < (int)spr.costumes.size())
            {
                update_composed_texture(spr.costumes[spr.selected_costume], renderer, font);
                spr.texture = spr.costumes[spr.selected_costume].composed_texture;
            }
        }
        if (!state.backdrops.empty() && state.selected_backdrop >= 0 && state.selected_backdrop < (int)state.backdrops.size())
        {
            update_composed_texture(state.backdrops[state.selected_backdrop], renderer, font);
            state.backdrops[state.selected_backdrop].texture = state.backdrops[state.selected_backdrop].composed_texture;
        }

        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        SDL_RenderClear(renderer);

        if (state.mode == MODE_EXTENSION_LIBRARY)
        {
            SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
            SDL_RenderFillRect(renderer, NULL);
            SDL_Rect nav_bg = {0, 0, WINDOW_WIDTH, NAVBAR_HEIGHT};
            SDL_SetRenderDrawColor(renderer, 76, 151, 255, 255);
            SDL_RenderFillRect(renderer, &nav_bg);
            render_simple_text(renderer, font_large, "< Back", 30, (NAVBAR_HEIGHT - 24) / 2, {255, 255, 255});
            int tw = 0;
            TTF_SizeUTF8(font_large, "Choose an Extension", &tw, NULL);
            render_simple_text(renderer, font_large, "Choose an Extension", (WINDOW_WIDTH - tw) / 2, (NAVBAR_HEIGHT - 24) / 2, {255, 255, 255});

            int box_w = WINDOW_WIDTH / 3, box_h = 320, box_x = 60, box_y = NAVBAR_HEIGHT + 40;
            SDL_Rect box_rect = {box_x, box_y, box_w, box_h};
            renderer_fill_rounded_rect(renderer, &box_rect, 10, 255, 255, 255);
            SDL_SetRenderDrawColor(renderer, 210, 210, 210, 255);
            SDL_RenderDrawRect(renderer, &box_rect);

            int img_h = 210;
            if (pen_poster)
            {
                SDL_Rect img_r = {box_x + 1, box_y + 1, box_w - 2, img_h};
                SDL_RenderCopy(renderer, pen_poster, NULL, &img_r);
                SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
                SDL_RenderDrawLine(renderer, box_x, box_y + img_h + 2, box_x + box_w, box_y + img_h + 2);
            }
            int title_w = 0;
            TTF_SizeUTF8(font_large, "Pen", &title_w, NULL);
            render_simple_text(renderer, font_large, "Pen", box_x + (box_w - title_w) / 2, box_y + img_h + 20, {80, 80, 80});
            int desc_w = 0;
            TTF_SizeUTF8(font, "Draw with your sprites.", &desc_w, NULL);
            render_simple_text(renderer, font, "Draw with your sprites.", box_x + (box_w - desc_w) / 2, box_y + img_h + 60, {120, 120, 120});
        }
        else if (state.mode == MODE_SPRITE_LIBRARY)
        {
            SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
            SDL_RenderFillRect(renderer, NULL);
            SDL_Rect nav_bg = {0, 0, WINDOW_WIDTH, NAVBAR_HEIGHT + 20};
            SDL_SetRenderDrawColor(renderer, 76, 151, 255, 255);
            SDL_RenderFillRect(renderer, &nav_bg);
            render_simple_text(renderer, font_large, "< Back", 30, 20, {255, 255, 255});
            for (size_t i = 0; i < global_sprite_lib.size(); i++)
            {
                int x = 50 + i * 180, y = 150;
                SDL_Rect box = {x, y, 160, 160};
                renderer_fill_rounded_rect(renderer, &box, 12, 255, 255, 255);
                SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
                SDL_RenderDrawRect(renderer, &box);
                if (global_sprite_lib[i].second)
                {
                    SDL_Rect img_dst = {x + 30, y + 20, 100, 100};
                    SDL_RenderCopy(renderer, global_sprite_lib[i].second, NULL, &img_dst);
                }
                int tw = 0;
                TTF_SizeUTF8(font_large, global_sprite_lib[i].first.c_str(), &tw, NULL);
                render_simple_text(renderer, font_large, global_sprite_lib[i].first.c_str(), x + 80 - tw / 2, y + 125, {40, 40, 40});
            }
        }
        else if (state.mode == MODE_BACKDROP_LIBRARY)
        {
            SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
            SDL_RenderFillRect(renderer, NULL);
            SDL_Rect nav_bg = {0, 0, WINDOW_WIDTH, NAVBAR_HEIGHT + 20};
            SDL_SetRenderDrawColor(renderer, 76, 151, 255, 255);
            SDL_RenderFillRect(renderer, &nav_bg);
            render_simple_text(renderer, font_large, "< Back", 30, 20, {255, 255, 255});
            for (size_t i = 0; i < global_backdrop_lib.size(); i++)
            {
                int x = 50 + i * 180, y = 150;
                SDL_Rect box = {x, y, 160, 160};
                renderer_fill_rounded_rect(renderer, &box, 12, 255, 255, 255);
                SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
                SDL_RenderDrawRect(renderer, &box);
                if (global_backdrop_lib[i].second)
                {
                    SDL_Rect img_dst = {x + 10, y + 20, 140, 100};
                    SDL_RenderCopy(renderer, global_backdrop_lib[i].second, NULL, &img_dst);
                }
                int tw = 0;
                TTF_SizeUTF8(font_large, global_backdrop_lib[i].first.c_str(), &tw, NULL);
                render_simple_text(renderer, font_large, global_backdrop_lib[i].first.c_str(), x + 80 - tw / 2, y + 125, {40, 40, 40});
            }
        }
        else
        {
            if (state.current_tab == TAB_CODE)
            {
                drag_area_draw(renderer, font, state, drag_rects);
                canvas_draw(renderer, font, state, canvas_rects, tex);
                categories_draw(renderer, font, state, cat_rects);
                palette_draw(renderer, font, state, pal_rects, tex);
            }
            else if (state.current_tab == TAB_COSTUMES)
            {
                costumes_tab_draw(renderer, font, state, tex);
            }
            else if (state.current_tab == TAB_SOUNDS)
            {
                sounds_tab_draw(renderer, font, state, tex);
            }

            stage_draw(renderer, font, state, stage_rects, tex);
            settings_draw(renderer, font, state, settings_rects, tex);
            sprite_panel_draw(renderer, font, state, tex, sprite_panel_rects);
            tab_bar_draw(renderer, font, state, tab_bar_rects, tex);
            navbar_draw(renderer, font, state, navbar_rects, tex);
            filemenu_draw(renderer, font, state, filemenu_rects);
        }

        if (state.var_modal_active)
        {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
            SDL_Rect screen_rect = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
            SDL_RenderFillRect(renderer, &screen_rect);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            int mw = 400, mh = 200, mx = WINDOW_WIDTH / 2 - mw / 2, my = WINDOW_HEIGHT / 2 - mh / 2;
            SDL_Rect modal_rect = {mx, my, mw, mh};
            renderer_fill_rounded_rect(renderer, &modal_rect, 8, 255, 255, 255);
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            SDL_RenderDrawRect(renderer, &modal_rect);
            Color textCol = {40, 40, 40};
            render_simple_text(renderer, font, "New variable name:", modal_rect.x + 30, modal_rect.y + 30, textCol);
            SDL_Rect input_rect = {modal_rect.x + 30, modal_rect.y + 70, mw - 60, 40};
            renderer_fill_rounded_rect(renderer, &input_rect, 4, 240, 240, 240);
            SDL_SetRenderDrawColor(renderer, 76, 151, 255, 255);
            SDL_RenderDrawRect(renderer, &input_rect);
            render_simple_text(renderer, font, state.input_buffer.c_str(), input_rect.x + 10, input_rect.y + 12, textCol);
            if ((SDL_GetTicks() / 500) % 2 == 0)
            {
                int tw = 0;
                TTF_SizeUTF8(font, state.input_buffer.c_str(), &tw, NULL);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderDrawLine(renderer, input_rect.x + 10 + tw + 2, input_rect.y + 10, input_rect.x + 10 + tw + 2, input_rect.y + 30);
            }
            SDL_Rect submit_btn = {mx + mw - 120, my + mh - 60, 90, 40};
            renderer_fill_rounded_rect(renderer, &submit_btn, 4, 76, 151, 255);
            render_simple_text(renderer, font, "OK", submit_btn.x + 35, submit_btn.y + 12, (Color){255, 255, 255});
            SDL_Rect cancel_btn = {mx + mw - 220, my + mh - 60, 90, 40};
            renderer_fill_rounded_rect(renderer, &cancel_btn, 4, 220, 220, 220);
            render_simple_text(renderer, font, "Cancel", cancel_btn.x + 22, cancel_btn.y + 12, (Color){40, 40, 40});
        }

        if (state.active_input == INPUT_PEN_COLOR_PICKER)
        {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
            SDL_Rect screen_rect = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
            SDL_RenderFillRect(renderer, &screen_rect);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            int mw = 300, mh = 250, mx = WINDOW_WIDTH / 2 - mw / 2, my = WINDOW_HEIGHT / 2 - mh / 2;
            SDL_Rect modal_rect = {mx, my, mw, mh};
            renderer_fill_rounded_rect(renderer, &modal_rect, 8, 255, 255, 255);
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            SDL_RenderDrawRect(renderer, &modal_rect);
            render_simple_text(renderer, font_large, "Pick Color", modal_rect.x + 100, modal_rect.y + 20, {40, 40, 40});
            Color palette[9] = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 0}, {0, 255, 255}, {255, 0, 255}, {0, 0, 0}, {128, 128, 128}, {255, 255, 255}};
            for (int i = 0; i < 9; i++)
            {
                SDL_Rect c_rect = {mx + 30 + (i % 3) * 80, my + 80 + (i / 3) * 50, 70, 40};
                renderer_fill_rounded_rect(renderer, &c_rect, 4, palette[i].r, palette[i].g, palette[i].b);
                SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
                SDL_RenderDrawRect(renderer, &c_rect);
            }
        }
        SDL_RenderPresent(renderer);
    }

    if (pen_poster)
        SDL_DestroyTexture(pen_poster);
    textures_free(tex);
    TTF_CloseFont(font);
    TTF_CloseFont(font_large);
    SDL_StopTextInput();
    audio_quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    return 0;
}