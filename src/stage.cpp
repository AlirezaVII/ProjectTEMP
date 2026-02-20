#include "stage.h"
#include "config.h"
#include "renderer.h"
#include "interpreter.h"

static bool point_in_rect(int px, int py, const SDL_Rect &r) { return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h; }
static void set_color(SDL_Renderer *r, Color c) { SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255); }

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

void stage_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state, const StageRects &rects, const Textures &tex)
{
    set_color(r, COL_STAGE_BG);
    SDL_RenderFillRect(r, &rects.panel);
    set_color(r, COL_STAGE_BORDER);
    SDL_RenderDrawRect(r, &rects.stage_area);
    SDL_RenderSetClipRect(r, const_cast<SDL_Rect *>(&rects.stage_area));

    // ---> NEW: DRAW ALL SPRITES <---
    for (const auto &spr : state.sprites)
    {
        if (spr.visible)
        {
            int cx = rects.stage_area.x + rects.stage_area.w / 2 + spr.x;
            int cy = rects.stage_area.y + rects.stage_area.h / 2 - spr.y;
            int tex_w = 100, tex_h = 100;
            if (spr.texture)
                SDL_QueryTexture(spr.texture, NULL, NULL, &tex_w, &tex_h);
            int base_w = tex_w;
            int base_h = tex_h;
            int MAX_DEFAULT = 120;
            if (base_w > MAX_DEFAULT || base_h > MAX_DEFAULT)
            {
                if (base_w > base_h)
                {
                    base_h = (base_h * MAX_DEFAULT) / base_w;
                    base_w = MAX_DEFAULT;
                }
                else
                {
                    base_w = (base_w * MAX_DEFAULT) / base_h;
                    base_h = MAX_DEFAULT;
                }
            }
            int w = (base_w * spr.size) / 100;
            int h = (base_h * spr.size) / 100;
            SDL_Rect dest = {cx - w / 2, cy - h / 2, w, h};

            if (spr.texture)
            {
                double angle = spr.direction - 90.0;
                SDL_RenderCopyEx(r, spr.texture, NULL, &dest, angle, NULL, SDL_FLIP_NONE);
            }
            else
            {
                SDL_SetRenderDrawColor(r, 255, 165, 0, 255);
                SDL_RenderFillRect(r, &dest);
            }

            if (!spr.say_text.empty())
            {
                bool should_draw = true;
                if (spr.say_end_time > 0 && SDL_GetTicks() > spr.say_end_time)
                    should_draw = false;
                if (should_draw)
                {
                    int tw = 0, th = 0;
                    TTF_SizeUTF8(font, spr.say_text.c_str(), &tw, &th);
                    int bub_w = tw + 24;
                    int bub_h = th + 20;
                    int bub_x = dest.x + dest.w - 10;
                    int bub_y = dest.y - bub_h;
                    if (bub_x + bub_w > rects.stage_area.x + rects.stage_area.w)
                        bub_x = dest.x - bub_w + 10;
                    if (bub_y < rects.stage_area.y)
                        bub_y = dest.y + dest.h;
                    if (spr.is_thinking && tex.cloud)
                    {
                        SDL_Rect cloud_r = {bub_x - 10, bub_y - 10, bub_w + 20, bub_h + 30};
                        SDL_RenderCopy(r, tex.cloud, NULL, &cloud_r);
                    }
                    else
                    {
                        SDL_Rect bub_r = {bub_x, bub_y, bub_w, bub_h};
                        SDL_SetRenderDrawColor(r, 160, 160, 160, 255);
                        renderer_fill_rounded_rect(r, &bub_r, 12, 160, 160, 160);
                        SDL_Rect inner_r = {bub_x + 2, bub_y + 2, bub_w - 4, bub_h - 4};
                        renderer_fill_rounded_rect(r, &inner_r, 10, 255, 255, 255);
                        if (spr.is_thinking)
                        {
                            renderer_fill_circle(r, bub_x - 5, bub_y + bub_h + 5, 4, 255, 255, 255);
                            renderer_fill_circle(r, bub_x - 15, bub_y + bub_h + 15, 2, 255, 255, 255);
                        }
                        else
                        {
                            SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
                            SDL_RenderDrawLine(r, bub_x + 10, bub_y + bub_h - 2, dest.x + dest.w / 2, dest.y + dest.h / 4);
                            SDL_RenderDrawLine(r, bub_x + 20, bub_y + bub_h - 2, dest.x + dest.w / 2, dest.y + dest.h / 4);
                        }
                    }
                    SDL_Color tc = {0, 0, 0, 255};
                    SDL_Surface *s = TTF_RenderUTF8_Blended(font, spr.say_text.c_str(), tc);
                    if (s)
                    {
                        SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
                        SDL_Rect td = {bub_x + 12, bub_y + 10, s->w, s->h};
                        SDL_RenderCopy(r, t, NULL, &td);
                        SDL_DestroyTexture(t);
                        SDL_FreeSurface(s);
                    }
                }
            }
        }
    }

    if (state.ask_active)
    {
        int ask_h = 60;
        SDL_Rect ask_bg = {rects.stage_area.x, rects.stage_area.y + rects.stage_area.h - ask_h, rects.stage_area.w, ask_h};
        SDL_SetRenderDrawColor(r, 230, 240, 255, 255);
        SDL_RenderFillRect(r, &ask_bg);
        SDL_SetRenderDrawColor(r, 0, 160, 255, 255);
        SDL_RenderDrawRect(r, &ask_bg);
        SDL_Color tc = {40, 40, 40, 255};
        SDL_Surface *ms = TTF_RenderUTF8_Blended(font, state.ask_msg.c_str(), tc);
        if (ms)
        {
            SDL_Texture *mt = SDL_CreateTextureFromSurface(r, ms);
            SDL_Rect md = {ask_bg.x + 10, ask_bg.y + 10, ms->w, ms->h};
            SDL_RenderCopy(r, mt, NULL, &md);
            SDL_DestroyTexture(mt);
            SDL_FreeSurface(ms);
        }
        SDL_Rect inp_r = {ask_bg.x + 10, ask_bg.y + 30, ask_bg.w - 20, 24};
        renderer_fill_rounded_rect(r, &inp_r, 4, 255, 255, 255);
        SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
        SDL_RenderDrawRect(r, &inp_r);
        SDL_Surface *rs = TTF_RenderUTF8_Blended(font, state.ask_reply.c_str(), tc);
        if (rs)
        {
            SDL_Texture *rt = SDL_CreateTextureFromSurface(r, rs);
            SDL_Rect rd = {inp_r.x + 6, inp_r.y + 4, rs->w, rs->h};
            SDL_RenderCopy(r, rt, NULL, &rd);
            SDL_DestroyTexture(rt);
            SDL_FreeSurface(rs);
        }
        if ((SDL_GetTicks() / 500) % 2 == 0)
        {
            int tw = 0;
            TTF_SizeUTF8(font, state.ask_reply.c_str(), &tw, NULL);
            SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
            SDL_RenderDrawLine(r, inp_r.x + 6 + tw, inp_r.y + 4, inp_r.x + 6 + tw, inp_r.y + 20);
        }
    }
    SDL_RenderSetClipRect(r, NULL);
}

bool stage_handle_event(const SDL_Event &e, AppState &state, const StageRects &rects, const Textures &tex)
{
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        int mx = e.button.x, my = e.button.y;
        if (point_in_rect(mx, my, rects.stage_area))
        {
            // Hit test backwards (top to bottom)
            for (int i = (int)state.sprites.size() - 1; i >= 0; i--)
            {
                auto &spr = state.sprites[i];
                if (!spr.visible)
                    continue;
                int cx = rects.stage_area.x + rects.stage_area.w / 2 + spr.x;
                int cy = rects.stage_area.y + rects.stage_area.h / 2 - spr.y;
                int tex_w = 100, tex_h = 100;
                if (spr.texture)
                    SDL_QueryTexture(spr.texture, NULL, NULL, &tex_w, &tex_h);
                int base_w = tex_w;
                int base_h = tex_h;
                int MAX_DEFAULT = 120;
                if (base_w > MAX_DEFAULT || base_h > MAX_DEFAULT)
                {
                    if (base_w > base_h)
                    {
                        base_h = (base_h * MAX_DEFAULT) / base_w;
                        base_w = MAX_DEFAULT;
                    }
                    else
                    {
                        base_w = (base_w * MAX_DEFAULT) / base_h;
                        base_h = MAX_DEFAULT;
                    }
                }
                int w = (base_w * spr.size) / 100;
                int h = (base_h * spr.size) / 100;
                SDL_Rect sprite_rect = {cx - w / 2, cy - h / 2, w, h};
                if (point_in_rect(mx, my, sprite_rect))
                {
                    state.selected_sprite = i; // Select the clicked sprite!
                    if (spr.draggable)
                    {
                        state.stage_drag_active = true;
                        state.stage_drag_off_x = mx - cx;
                        state.stage_drag_off_y = my - cy;
                    }
                    interpreter_trigger_sprite_click(state);
                    return true;
                }
            }
        }
    }
    else if (e.type == SDL_MOUSEMOTION)
    {
        if (state.stage_drag_active && state.selected_sprite >= 0 && state.selected_sprite < (int)state.sprites.size())
        {
            int cx = e.motion.x - state.stage_drag_off_x;
            int cy = e.motion.y - state.stage_drag_off_y;
            state.sprites[state.selected_sprite].x = cx - (rects.stage_area.x + rects.stage_area.w / 2);
            state.sprites[state.selected_sprite].y = (rects.stage_area.y + rects.stage_area.h / 2) - cy;
            return true;
        }
    }
    else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
    {
        if (state.stage_drag_active)
        {
            state.stage_drag_active = false;
            return true;
        }
    }
    return false;
}