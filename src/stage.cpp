#include "stage.h"
#include "config.h"
#include "renderer.h"
#include "interpreter.h"

static bool point_in_rect(int px, int py, const SDL_Rect &r)
{
    return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
}

static void set_color(SDL_Renderer *r, Color c)
{
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255);
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
                const StageRects &rects, const Textures &tex)
{
    set_color(r, COL_STAGE_BG);
    SDL_RenderFillRect(r, &rects.panel);
    set_color(r, COL_STAGE_BORDER);
    SDL_RenderDrawRect(r, &rects.stage_area);

    if (state.sprite.visible)
    {
        int cx = rects.stage_area.x + rects.stage_area.w / 2 + state.sprite.x;
        int cy = rects.stage_area.y + rects.stage_area.h / 2 - state.sprite.y;

        int tex_w = 100, tex_h = 100;
        if (tex.scratch_cat)
            SDL_QueryTexture(tex.scratch_cat, NULL, NULL, &tex_w, &tex_h);

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

        int w = (base_w * state.sprite.size) / 100;
        int h = (base_h * state.sprite.size) / 100;
        SDL_Rect dest = {cx - w / 2, cy - h / 2, w, h};

        SDL_RenderSetClipRect(r, const_cast<SDL_Rect *>(&rects.stage_area));

        if (tex.scratch_cat)
        {
            double angle = state.sprite.direction - 90.0;
            SDL_RenderCopyEx(r, tex.scratch_cat, NULL, &dest, angle, NULL, SDL_FLIP_NONE);
        }
        else
        {
            SDL_SetRenderDrawColor(r, 255, 165, 0, 255);
            SDL_RenderFillRect(r, &dest);
        }

        if (!state.sprite.say_text.empty())
        {
            bool should_draw = true;
            if (state.sprite.say_end_time > 0 && SDL_GetTicks() > state.sprite.say_end_time)
                should_draw = false;

            if (should_draw)
            {
                int tw = 0, th = 0;
                TTF_SizeUTF8(font, state.sprite.say_text.c_str(), &tw, &th);

                int bub_w = tw + 24;
                int bub_h = th + 20;
                int bub_x = dest.x + dest.w - 10;
                int bub_y = dest.y - bub_h;

                if (bub_x + bub_w > rects.stage_area.x + rects.stage_area.w)
                    bub_x = dest.x - bub_w + 10;
                if (bub_y < rects.stage_area.y)
                    bub_y = dest.y + dest.h;

                if (state.sprite.is_thinking && tex.cloud)
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

                    if (state.sprite.is_thinking)
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
                SDL_Surface *s = TTF_RenderUTF8_Blended(font, state.sprite.say_text.c_str(), tc);
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
        // ---> FIXED: DRAW VARIABLE MONITORS (SUPPORTS TEXT) <---
        int var_y = rects.stage_area.y + 10;
        for (const std::string &vname : state.variables)
        {
            if (state.variable_visible.count(vname) && state.variable_visible.at(vname))
            {

                // Just grab the string directly!
                std::string s_val = state.variable_values.count(vname) ? state.variable_values.at(vname) : "0";

                int tw1 = 0, th1 = 0;
                TTF_SizeUTF8(font, vname.c_str(), &tw1, &th1);
                int tw2 = 0, th2 = 0;
                TTF_SizeUTF8(font, s_val.c_str(), &tw2, &th2);

                int box_w = tw1 + tw2 + 24;
                SDL_Rect mon = {rects.stage_area.x + 10, var_y, box_w, 24};

                renderer_fill_rounded_rect(r, &mon, 4, 210, 210, 210);
                SDL_SetRenderDrawColor(r, 180, 180, 180, 255);
                SDL_RenderDrawRect(r, &mon);

                SDL_Color tcl = {40, 40, 40, 255};
                SDL_Surface *s1 = TTF_RenderUTF8_Blended(font, vname.c_str(), tcl);
                if (s1)
                {
                    SDL_Texture *t1 = SDL_CreateTextureFromSurface(r, s1);
                    SDL_Rect d1 = {mon.x + 6, mon.y + (24 - s1->h) / 2, s1->w, s1->h};
                    SDL_RenderCopy(r, t1, NULL, &d1);
                    SDL_DestroyTexture(t1);
                    SDL_FreeSurface(s1);
                }

                SDL_Rect val_bg = {mon.x + tw1 + 12, mon.y + 3, tw2 + 8, 18};
                renderer_fill_rounded_rect(r, &val_bg, 4, 255, 140, 26);

                SDL_Color tcv = {255, 255, 255, 255};
                SDL_Surface *s2 = TTF_RenderUTF8_Blended(font, s_val.c_str(), tcv);
                if (s2)
                {
                    SDL_Texture *t2 = SDL_CreateTextureFromSurface(r, s2);
                    SDL_Rect d2 = {val_bg.x + 4, val_bg.y + (18 - s2->h) / 2, s2->w, s2->h};
                    SDL_RenderCopy(r, t2, NULL, &d2);
                    SDL_DestroyTexture(t2);
                    SDL_FreeSurface(s2);
                }
                var_y += 30;
            }
        }
        SDL_RenderSetClipRect(r, NULL);
    }
}

bool stage_handle_event(const SDL_Event &e, AppState &state,
                        const StageRects &rects, const Textures &tex)
{
    int tex_w = 100, tex_h = 100;
    if (tex.scratch_cat)
        SDL_QueryTexture(tex.scratch_cat, NULL, NULL, &tex_w, &tex_h);
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
    int w = (base_w * state.sprite.size) / 100;
    int h = (base_h * state.sprite.size) / 100;

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        int mx = e.button.x, my = e.button.y;
        if (point_in_rect(mx, my, rects.stage_area))
        {
            if (state.sprite.visible)
            {
                int cx = rects.stage_area.x + rects.stage_area.w / 2 + state.sprite.x;
                int cy = rects.stage_area.y + rects.stage_area.h / 2 - state.sprite.y;
                SDL_Rect sprite_rect = {cx - w / 2, cy - h / 2, w, h};

                if (point_in_rect(mx, my, sprite_rect))
                {
                    // ---> FIXED: ENFORCING THE DRAG MODE MEMORY FLAG! <---
                    if (state.sprite.draggable)
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
        if (state.stage_drag_active)
        {
            int cx = e.motion.x - state.stage_drag_off_x;
            int cy = e.motion.y - state.stage_drag_off_y;
            state.sprite.x = cx - (rects.stage_area.x + rects.stage_area.w / 2);
            state.sprite.y = (rects.stage_area.y + rects.stage_area.h / 2) - cy;
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