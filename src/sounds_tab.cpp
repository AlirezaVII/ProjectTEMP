#include "sounds_tab.h"
#include "config.h"
#include "renderer.h"
#include "audio.h"
#include <cstdio>
#include <cstdlib>
#include <string>

static void draw_text(SDL_Renderer *r, TTF_Font *f, const char *txt, int x, int y, Uint8 cr, Uint8 cg, Uint8 cb)
{
    SDL_Color sc = {cr, cg, cb, 255};
    SDL_Surface *surf = TTF_RenderUTF8_Blended(f, txt, sc);
    if (!surf)
        return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(r, surf);
    SDL_Rect dst = {x, y, surf->w, surf->h};
    SDL_RenderCopy(r, t, NULL, &dst);
    SDL_DestroyTexture(t);
    SDL_FreeSurface(surf);
}

static void fill_rounded(SDL_Renderer *r, SDL_Rect *rect, int radius, Uint8 cr, Uint8 cg, Uint8 cb)
{
    SDL_SetRenderDrawColor(r, cr, cg, cb, 255);
    SDL_RenderFillRect(r, rect);
}

static bool point_in(const SDL_Rect &r, int x, int y) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; }

void sounds_tab_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state, const Textures &tex)
{
    int top_y = NAVBAR_HEIGHT + TAB_BAR_HEIGHT;
    int left_w = 250;
    int panel_h = WINDOW_HEIGHT - top_y;

    SDL_Rect left_panel = {0, top_y, left_w, panel_h};
    SDL_SetRenderDrawColor(r, 235, 245, 255, 255);
    SDL_RenderFillRect(r, &left_panel);
    SDL_SetRenderDrawColor(r, 200, 220, 240, 255);
    SDL_RenderDrawLine(r, left_w, top_y, left_w, WINDOW_HEIGHT);

    SDL_Rect right_panel = {left_w + 1, top_y, WINDOW_WIDTH - left_w, panel_h};
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderFillRect(r, &right_panel);

    if (state.selected_sprite < 0 || state.selected_sprite >= (int)state.sprites.size())
        return;
    const Sprite &spr = state.sprites[state.selected_sprite];

    int sy = top_y + 10;
    for (size_t i = 0; i < spr.sounds.size(); i++)
    {
        SDL_Rect box = {10, sy, left_w - 20, 60};

        if ((int)i == spr.selected_sound)
        {
            SDL_Rect border = {box.x - 2, box.y - 2, box.w + 4, box.h + 4};
            fill_rounded(r, &border, 8, 77, 151, 255);
            fill_rounded(r, &box, 6, 255, 255, 255);
        }
        else
        {
            fill_rounded(r, &box, 6, 255, 255, 255);
            SDL_SetRenderDrawColor(r, 180, 180, 180, 255);
            SDL_RenderDrawRect(r, &box);
        }

        SDL_Texture *ic = tex.vol_up;
        if (spr.sounds[i].volume == 0)
            ic = tex.vol_mute;
        else if (spr.sounds[i].volume < 50)
            ic = tex.vol_down;

        if (ic)
        {
            SDL_Rect ic_r = {box.x + 10, box.y + 14, 32, 32};
            SDL_RenderCopy(r, ic, NULL, &ic_r);
        }

        if ((int)i == spr.selected_sound)
            draw_text(r, font, spr.sounds[i].name.c_str(), box.x + 55, box.y + 20, 0, 0, 0);
        else
            draw_text(r, font, spr.sounds[i].name.c_str(), box.x + 55, box.y + 20, 80, 80, 80);

        if ((int)i == spr.selected_sound && tex.delete_sprite)
        {
            SDL_Rect del_r = {box.x + box.w - 20, box.y - 8, 24, 24};
            SDL_RenderCopy(r, tex.delete_sprite, NULL, &del_r);
        }
        sy += 70;
    }

    SDL_Rect up_btn = {10, WINDOW_HEIGHT - 60, left_w - 20, 40};
    fill_rounded(r, &up_btn, 8, 77, 151, 255);
    draw_text(r, font, "Upload Sound", up_btn.x + 60, up_btn.y + 10, 255, 255, 255);

    if (spr.selected_sound >= 0 && spr.selected_sound < (int)spr.sounds.size())
    {
        const auto &snd = spr.sounds[spr.selected_sound];
        int rx = left_w + 40;
        int ry = top_y + 40;

        draw_text(r, font, "Sound Name:", rx, ry + 5, 40, 40, 40);
        SDL_Rect name_box = {rx + 110, ry, 200, 30};
        fill_rounded(r, &name_box, 4, 255, 255, 255);
        SDL_SetRenderDrawColor(r, state.active_input == INPUT_SOUND_NAME ? 77 : 120, state.active_input == INPUT_SOUND_NAME ? 151 : 120, state.active_input == INPUT_SOUND_NAME ? 255 : 120, 255);
        SDL_RenderDrawRect(r, &name_box);
        std::string n_txt = (state.active_input == INPUT_SOUND_NAME) ? state.input_buffer + "|" : snd.name;
        draw_text(r, font, n_txt.c_str(), name_box.x + 10, name_box.y + 6, 0, 0, 0);

        ry += 50;

        draw_text(r, font, "Volume %:", rx, ry + 5, 40, 40, 40);
        SDL_Rect vol_box = {rx + 110, ry, 60, 30};
        fill_rounded(r, &vol_box, 4, 255, 255, 255);
        SDL_SetRenderDrawColor(r, state.active_input == INPUT_SOUND_VOLUME ? 77 : 120, state.active_input == INPUT_SOUND_VOLUME ? 151 : 120, state.active_input == INPUT_SOUND_VOLUME ? 255 : 120, 255);
        SDL_RenderDrawRect(r, &vol_box);
        char vb[32];
        std::snprintf(vb, sizeof(vb), "%d", snd.volume);
        std::string v_txt = (state.active_input == INPUT_SOUND_VOLUME) ? state.input_buffer + "|" : vb;
        draw_text(r, font, v_txt.c_str(), vol_box.x + 10, vol_box.y + 6, 0, 0, 0);

        ry += 60;

        bool is_playing = audio_is_playing();
        SDL_Rect play_btn = {rx, ry, 120, 50};
        fill_rounded(r, &play_btn, 8, 153, 102, 255);
        if (tex.play_icon && !is_playing)
        {
            SDL_Rect ic_r = {play_btn.x + 15, play_btn.y + 9, 32, 32};
            SDL_RenderCopy(r, tex.play_icon, NULL, &ic_r);
        }
        draw_text(r, font, is_playing ? "Pause" : "Play", play_btn.x + (is_playing ? 35 : 55), play_btn.y + 15, 255, 255, 255);

        SDL_Rect mute_btn = {rx + 140, ry, 120, 50};
        bool is_muted = (snd.volume == 0);
        if (is_muted)
            fill_rounded(r, &mute_btn, 8, 76, 200, 76);
        else
            fill_rounded(r, &mute_btn, 8, 200, 200, 200);

        if (tex.vol_mute)
        {
            SDL_Rect ic_r = {mute_btn.x + 15, mute_btn.y + 9, 32, 32};
            SDL_RenderCopy(r, tex.vol_mute, NULL, &ic_r);
        }
        draw_text(r, font, is_muted ? "Unmute" : "Mute", mute_btn.x + 55, mute_btn.y + 15, is_muted ? 255 : 40, is_muted ? 255 : 40, is_muted ? 255 : 40);
    }
}

bool sounds_tab_handle_event(const SDL_Event &e, AppState &state)
{
    if (state.current_tab != TAB_SOUNDS)
        return false;
    if (e.type != SDL_MOUSEBUTTONDOWN || e.button.button != SDL_BUTTON_LEFT)
        return false;

    int mx = e.button.x, my = e.button.y;
    int top_y = NAVBAR_HEIGHT + TAB_BAR_HEIGHT;
    int left_w = 250;

    if (state.selected_sprite < 0 || state.selected_sprite >= (int)state.sprites.size())
        return false;
    Sprite &spr = state.sprites[state.selected_sprite];

    SDL_Rect up_btn = {10, WINDOW_HEIGHT - 60, left_w - 20, 40};
    if (point_in(up_btn, mx, my))
    {
        char buffer[1024];
        std::string result = "";
        FILE *pipe = popen("osascript -e 'POSIX path of (choose file with prompt \"Choose Sound\" of type {\"public.audio\"})'", "r");
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
            Mix_Chunk *c = audio_load_sound(result);
            if (c)
            {
                size_t slash = result.find_last_of('/');
                std::string fname = (slash == std::string::npos) ? result : result.substr(slash + 1);
                size_t dot = fname.find_last_of('.');
                if (dot != std::string::npos)
                    fname = fname.substr(0, dot);
                spr.sounds.push_back(SoundData(fname, c));
                spr.selected_sound = spr.sounds.size() - 1;
            }
        }
        return true;
    }

    if (mx < left_w)
    {
        int sy = top_y + 10;
        for (size_t i = 0; i < spr.sounds.size(); i++)
        {
            SDL_Rect box = {10, sy, left_w - 20, 60};
            SDL_Rect del_r = {box.x + box.w - 20, box.y - 8, 24, 24};

            if ((int)i == spr.selected_sound && point_in(del_r, mx, my))
            {
                audio_stop_all();
                spr.sounds.erase(spr.sounds.begin() + i);
                if (spr.selected_sound >= (int)spr.sounds.size())
                    spr.selected_sound = spr.sounds.size() - 1;
                if (spr.selected_sound < 0)
                    spr.selected_sound = 0;
                return true;
            }
            if (point_in(box, mx, my))
            {
                spr.selected_sound = i;
                return true;
            }
            sy += 70;
        }
    }

    if (spr.selected_sound >= 0 && spr.selected_sound < (int)spr.sounds.size())
    {
        auto &snd = spr.sounds[spr.selected_sound];
        int rx = left_w + 40;
        int ry = top_y + 40;

        SDL_Rect name_box = {rx + 110, ry, 200, 30};
        if (point_in(name_box, mx, my))
        {
            state.active_input = INPUT_SOUND_NAME;
            state.input_buffer = snd.name; 
            return true;
        }

        ry += 50;
        SDL_Rect vol_box = {rx + 110, ry, 60, 30};
        if (point_in(vol_box, mx, my))
        {
            state.active_input = INPUT_SOUND_VOLUME;
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%d", snd.volume);
            state.input_buffer = buf; 
            return true;
        }

        ry += 60;
        SDL_Rect play_btn = {rx, ry, 120, 50};
        if (point_in(play_btn, mx, my))
        {
            if (audio_is_playing()) audio_stop_all();
            else audio_play_chunk(snd.chunk, snd.volume);
            return true;
        }

        SDL_Rect mute_btn = {rx + 140, ry, 120, 50};
        if (point_in(mute_btn, mx, my))
        {
            if (snd.volume > 0) { snd.prev_volume = snd.volume; snd.volume = 0; }
            else { snd.volume = (snd.prev_volume > 0) ? snd.prev_volume : 100; }
            audio_set_volume(snd.volume);
            return true;
        }
    }

    return false;
}