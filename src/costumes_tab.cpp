#include "costumes_tab.h"
#include "config.h"
#include "renderer.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

static void draw_text_centered(SDL_Renderer *r, TTF_Font *font, const char *txt, int cx, int cy, Uint8 cr, Uint8 cg, Uint8 cb)
{
    SDL_Color col = {cr, cg, cb, 255};
    SDL_Surface *s = TTF_RenderUTF8_Blended(font, txt, col);
    if (!s)
        return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
    SDL_Rect dst = {cx - s->w / 2, cy - s->h / 2, s->w, s->h};
    SDL_RenderCopy(r, t, NULL, &dst);
    SDL_DestroyTexture(t);
    SDL_FreeSurface(s);
}

static void draw_text_left(SDL_Renderer *r, TTF_Font *font, const char *txt, int x, int y, Uint8 cr, Uint8 cg, Uint8 cb)
{
    SDL_Color col = {cr, cg, cb, 255};
    SDL_Surface *s = TTF_RenderUTF8_Blended(font, txt, col);
    if (!s)
        return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
    SDL_Rect dst = {x, y, s->w, s->h};
    SDL_RenderCopy(r, t, NULL, &dst);
    SDL_DestroyTexture(t);
    SDL_FreeSurface(s);
}

CostumesRects get_costumes_rects(const AppState &state)
{
    CostumesRects res;
    int area_x = 0, area_y = NAVBAR_HEIGHT + TAB_BAR_HEIGHT;
    int total_w = WINDOW_WIDTH - RIGHT_COLUMN_WIDTH, total_h = WINDOW_HEIGHT - area_y;
    int left_w = 120, canvas_x = left_w, canvas_w = total_w - left_w, toolbar_h = 60;

    res.name_box = {canvas_x + 20, area_y + 24, 150, 28};
    int tool_x = res.name_box.x + res.name_box.w + 40, tool_y = area_y + 16;
    for (int i = 0; i < 7; i++)
    {
        res.tools[i] = {tool_x, tool_y, 32, 32};
        tool_x += 40;
    }

    tool_x += 20;
    res.flip_h = {tool_x, tool_y, 32, 32};
    tool_x += 40;
    res.flip_v = {tool_x, tool_y, 32, 32};
    tool_x += 40;
    res.del_tool = {tool_x, tool_y, 32, 32};

    res.canvas = {canvas_x + 40, area_y + toolbar_h + 40, canvas_w - 80, total_h - toolbar_h - 80};

    const std::vector<GraphicItem> *items = state.editing_target_is_stage ? &state.backdrops : (state.selected_sprite >= 0 && state.selected_sprite < (int)state.sprites.size() ? &state.sprites[state.selected_sprite].costumes : nullptr);
    if (items)
    {
        int thumb_y = area_y + 10;
        for (size_t i = 0; i < items->size(); ++i)
        {
            res.thumbs.push_back({area_x + 10, thumb_y, 100, 80});
            res.thumb_dels.push_back({area_x + 10 + 100 - 12, thumb_y - 12, 24, 24});
            thumb_y += 110;
        }
    }

    // ---> FIXED: BIGGER 60x60 Add Button <---
    res.add_btn = {left_w / 2 - 30, WINDOW_HEIGHT - 90, 60, 60};
    return res;
}

void costumes_tab_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state, const Textures &tex)
{
    CostumesRects rects = get_costumes_rects(state);
    int area_x = 0, area_y = NAVBAR_HEIGHT + TAB_BAR_HEIGHT, left_w = 120, canvas_x = left_w, canvas_w = (WINDOW_WIDTH - RIGHT_COLUMN_WIDTH) - left_w, toolbar_h = 60;

    SDL_SetRenderDrawColor(r, 245, 245, 245, 255);
    SDL_Rect left_bg = {area_x, area_y, left_w, WINDOW_HEIGHT - area_y};
    SDL_RenderFillRect(r, &left_bg);
    SDL_SetRenderDrawColor(r, 220, 220, 220, 255);
    SDL_RenderDrawLine(r, left_w - 1, area_y, left_w - 1, WINDOW_HEIGHT);

    const std::vector<GraphicItem> *items = nullptr;
    int selected_idx = 0;
    if (state.editing_target_is_stage)
    {
        items = &state.backdrops;
        selected_idx = state.selected_backdrop;
    }
    else if (state.selected_sprite >= 0 && state.selected_sprite < (int)state.sprites.size())
    {
        items = &state.sprites[state.selected_sprite].costumes;
        selected_idx = state.sprites[state.selected_sprite].selected_costume;
    }

    if (items)
    {
        for (size_t i = 0; i < items->size(); ++i)
        {
            SDL_Rect box = rects.thumbs[i];
            if ((int)i == selected_idx)
            {
                SDL_Rect border = {box.x - 4, box.y - 4, box.w + 8, box.h + 24};
                renderer_fill_rounded_rect(r, &border, 6, 76, 151, 255);
                draw_text_centered(r, font, (*items)[i].name.c_str(), box.x + box.w / 2, box.y + box.h + 8, 255, 255, 255);

                // ---> FIXED: MANUALLY DRAW BEAUTIFUL RED 'X' FOR DELETE <---
                SDL_Rect del_r = rects.thumb_dels[i];
                int cx = del_r.x + del_r.w / 2;
                int cy = del_r.y + del_r.h / 2;
                renderer_fill_circle(r, cx, cy, del_r.w / 2, 255, 60, 60);
                SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
                int d = 4;
                for (int w = -1; w <= 1; w++)
                { // Thick lines
                    SDL_RenderDrawLine(r, cx - d + w, cy - d, cx + d + w, cy + d);
                    SDL_RenderDrawLine(r, cx - d + w, cy + d, cx + d + w, cy - d);
                }
            }
            else
            {
                renderer_fill_rounded_rect(r, &box, 4, 255, 255, 255);
                SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
                SDL_RenderDrawRect(r, &box);
                draw_text_centered(r, font, (*items)[i].name.c_str(), box.x + box.w / 2, box.y + box.h + 8, 80, 80, 80);
            }
            if ((*items)[i].texture)
            {
                SDL_Rect img_r = {box.x + 10, box.y + 10, box.w - 20, box.h - 20};
                renderer_draw_texture_fit(r, (*items)[i].texture, &img_r);
            }
        }
    }

    // ---> FIXED: PERFECT VECTOR PLUS BUTTON (No text font used) <---
    int btn_radius = rects.add_btn.w / 2;
    int btn_cx = rects.add_btn.x + btn_radius;
    int btn_cy = rects.add_btn.y + btn_radius;
    renderer_fill_circle(r, btn_cx, btn_cy, btn_radius, 76, 151, 255);
    int p_w = 24, p_t = 6;
    SDL_Rect p_h = {btn_cx - p_w / 2, btn_cy - p_t / 2, p_w, p_t};
    SDL_Rect p_v = {btn_cx - p_t / 2, btn_cy - p_w / 2, p_t, p_w};
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderFillRect(r, &p_h);
    SDL_RenderFillRect(r, &p_v);

    SDL_Rect right_bg = {canvas_x, area_y, canvas_w, WINDOW_HEIGHT - area_y};
    SDL_SetRenderDrawColor(r, 230, 238, 242, 255);
    SDL_RenderFillRect(r, &right_bg);

    SDL_Rect toolbar_bg = {canvas_x, area_y, canvas_w, toolbar_h};
    SDL_SetRenderDrawColor(r, 245, 245, 245, 255);
    SDL_RenderFillRect(r, &toolbar_bg);
    SDL_SetRenderDrawColor(r, 220, 220, 220, 255);
    SDL_RenderDrawLine(r, canvas_x, area_y + toolbar_h - 1, canvas_x + canvas_w, area_y + toolbar_h - 1);

    std::string active_name = items && selected_idx >= 0 && selected_idx < (int)items->size() ? (*items)[selected_idx].name : "Costume 1";
    draw_text_left(r, font, state.editing_target_is_stage ? "Backdrop" : "Costume", canvas_x + 20, area_y + 8, 120, 120, 120);

    SDL_Rect name_box = rects.name_box;
    renderer_fill_rounded_rect(r, &name_box, 4, 255, 255, 255);
    SDL_SetRenderDrawColor(r, 180, 180, 180, 255);
    SDL_RenderDrawRect(r, &name_box);
    std::string disp_name = (state.active_input == INPUT_COSTUME_NAME) ? state.input_buffer + "|" : active_name;
    draw_text_left(r, font, disp_name.c_str(), name_box.x + 8, name_box.y + 6, 40, 40, 40);

    // ---> FIXED: EXACT ACTIVE COLOR FOR TOOLS (76, 151, 255) <---
    auto draw_tool = [&](SDL_Rect btn, SDL_Texture *icon, bool active, const char *fallback_text)
    {
        if (active)
        {
            renderer_fill_rounded_rect(r, &btn, 4, 76, 151, 255);
        }
        else
        {
            renderer_fill_rounded_rect(r, &btn, 4, 245, 245, 245);
            SDL_SetRenderDrawColor(r, 150, 150, 150, 255);
            SDL_RenderDrawRect(r, &btn);
        }

        if (icon)
        {
            SDL_Rect ic = {btn.x + 4, btn.y + 4, 24, 24};
            SDL_RenderCopy(r, icon, NULL, &ic);
        }
        else
        {
            draw_text_centered(r, font, fallback_text, btn.x + 16, btn.y + 16, active ? 255 : 60, active ? 255 : 60, active ? 255 : 60);
        }
    };

    draw_tool(rects.tools[0], state.active_tool == TOOL_POINTER ? tex.mouse_active : tex.mouse_nonactive, state.active_tool == TOOL_POINTER, "Ptr");
    draw_tool(rects.tools[1], state.active_tool == TOOL_BRUSH ? tex.brush_active : tex.brush_nonactive, state.active_tool == TOOL_BRUSH, "Brsh");
    draw_tool(rects.tools[2], state.active_tool == TOOL_ERASER ? tex.eraser_active : tex.eraser_nonactive, state.active_tool == TOOL_ERASER, "Ers");
    draw_tool(rects.tools[3], state.active_tool == TOOL_TEXT ? tex.text_active : tex.text_nonactive, state.active_tool == TOOL_TEXT, "Txt");
    draw_tool(rects.tools[4], state.active_tool == TOOL_FILL ? tex.fill_active : tex.fill_nonactive, state.active_tool == TOOL_FILL, "Fill");
    draw_tool(rects.tools[5], state.active_tool == TOOL_RECT ? tex.rect_active : tex.rect_nonactive, state.active_tool == TOOL_RECT, "Rect");
    draw_tool(rects.tools[6], state.active_tool == TOOL_CIRCLE ? tex.circ_active : tex.circ_nonactive, state.active_tool == TOOL_CIRCLE, "Circ");

    draw_tool(rects.flip_h, tex.flip_h, false, "FlpH");
    draw_tool(rects.flip_v, tex.flip_v, false, "FlpV");

    SDL_Texture *trash_icon = tex.trash_nonactive ? tex.trash_nonactive : tex.trash_active;
    draw_tool(rects.del_tool, trash_icon, false, "Del");

    SDL_Rect canvas = rects.canvas;
    renderer_fill_rounded_rect(r, &canvas, 8, 255, 255, 255);
    SDL_RenderSetClipRect(r, &canvas);
    for (int y = canvas.y; y < canvas.y + canvas.h; y += 15)
    {
        for (int x = canvas.x; x < canvas.x + canvas.w; x += 15)
        {
            bool even = ((x - canvas.x) / 15 + (y - canvas.y) / 15) % 2 == 0;
            SDL_SetRenderDrawColor(r, even ? 255 : 230, even ? 255 : 230, even ? 255 : 230, 255);
            SDL_Rect sq = {x, y, 15, 15};
            SDL_RenderFillRect(r, &sq);
        }
    }
    SDL_RenderSetClipRect(r, NULL);
    SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
    SDL_RenderDrawRect(r, &canvas);

    if (items && selected_idx >= 0 && selected_idx < (int)items->size() && (*items)[selected_idx].texture)
    {
        SDL_Texture *active_tex = (*items)[selected_idx].texture;
        int tw, th;
        SDL_QueryTexture(active_tex, NULL, NULL, &tw, &th);
        float scale = 1.0f;
        if (tw > canvas.w * 0.8f || th > canvas.h * 0.8f)
        {
            float sx = (canvas.w * 0.8f) / tw;
            float sy = (canvas.h * 0.8f) / th;
            scale = std::min(sx, sy);
        }
        int fw = tw * scale, fh = th * scale;
        SDL_Rect dst = {canvas.x + (canvas.w - fw) / 2, canvas.y + (canvas.h - fh) / 2, fw, fh};
        SDL_RenderCopy(r, active_tex, NULL, &dst);
    }
}

bool costumes_tab_handle_event(const SDL_Event &e, AppState &state)
{
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        CostumesRects rects = get_costumes_rects(state);
        int mx = e.button.x, my = e.button.y;

        auto point_in = [](const SDL_Rect &r, int x, int y)
        { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; };

        const std::vector<GraphicItem> *items = state.editing_target_is_stage ? &state.backdrops : (state.selected_sprite >= 0 && state.selected_sprite < (int)state.sprites.size() ? &state.sprites[state.selected_sprite].costumes : nullptr);
        int selected_idx = state.editing_target_is_stage ? state.selected_backdrop : (state.selected_sprite >= 0 && state.selected_sprite < (int)state.sprites.size() ? state.sprites[state.selected_sprite].selected_costume : 0);

        for (size_t i = 0; i < rects.thumbs.size(); ++i)
        {
            if (point_in(rects.thumb_dels[i], mx, my) && selected_idx == (int)i)
            {
                if (state.editing_target_is_stage)
                {
                    if (state.backdrops.size() > 1)
                    {
                        state.backdrops.erase(state.backdrops.begin() + i);
                        if (state.selected_backdrop >= (int)state.backdrops.size())
                            state.selected_backdrop = state.backdrops.size() - 1;
                    }
                }
                else
                {
                    auto &spr = state.sprites[state.selected_sprite];
                    if (spr.costumes.size() > 1)
                    {
                        spr.costumes.erase(spr.costumes.begin() + i);
                        if (spr.selected_costume >= (int)spr.costumes.size())
                            spr.selected_costume = spr.costumes.size() - 1;
                    }
                }
                return true;
            }
            if (point_in(rects.thumbs[i], mx, my))
            {
                if (state.editing_target_is_stage)
                    state.selected_backdrop = i;
                else
                    state.sprites[state.selected_sprite].selected_costume = i;
                return true;
            }
        }

        if (point_in(rects.name_box, mx, my))
        {
            state.active_input = INPUT_COSTUME_NAME;
            state.input_buffer = "";
            return true;
        }
        for (int i = 0; i < 7; ++i)
            if (point_in(rects.tools[i], mx, my))
            {
                state.active_tool = (EditTool)i;
                return true;
            }

        if (point_in(rects.del_tool, mx, my))
        {
            if (state.editing_target_is_stage && state.selected_backdrop >= 0 && state.selected_backdrop < (int)state.backdrops.size())
            {
                state.backdrops[state.selected_backdrop].shapes.clear();
            }
            else if (!state.editing_target_is_stage && state.selected_sprite >= 0 && state.selected_sprite < (int)state.sprites.size())
            {
                auto &spr = state.sprites[state.selected_sprite];
                if (spr.selected_costume >= 0 && spr.selected_costume < (int)spr.costumes.size())
                {
                    spr.costumes[spr.selected_costume].shapes.clear();
                }
            }
            return true;
        }

        if (point_in(rects.add_btn, mx, my))
            return true;
    }
    return false;
}