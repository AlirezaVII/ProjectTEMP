#include "costumes_tab.h"
#include "config.h"
#include "renderer.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>

static const int LOGICAL_W = 480;
static const int LOGICAL_H = 360;
static const int HIRES_MULT = 4;
static const int RENDER_W = LOGICAL_W * HIRES_MULT;
static const int RENDER_H = LOGICAL_H * HIRES_MULT;

static bool g_is_dragging = false;
static int g_drag_start_x = 0;
static int g_drag_start_y = 0;
static int g_last_mouse_x = 0;
static int g_last_mouse_y = 0;
static int g_resize_handle = 0;
static bool g_is_drawing = false;

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

// Helper to allow dragging rectangles backwards!
static SDL_Rect get_normalized_rect(SDL_Rect r)
{
    if (r.w < 0)
    {
        r.x += r.w;
        r.w = -r.w;
    }
    if (r.h < 0)
    {
        r.y += r.h;
        r.h = -r.h;
    }
    return r;
}

static void fill_circle_local(SDL_Renderer *r, int cx, int cy, int radius, bool erase, SDL_Color color)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    if (erase)
    {
        SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
    }
    else
    {
        SDL_SetRenderDrawColor(r, color.r, color.g, color.b, 255);
    }
    for (int dy = -radius; dy <= radius; ++dy)
    {
        int dx = 0;
        while (dx * dx + dy * dy <= radius * radius)
            ++dx;
        --dx;
        SDL_RenderDrawLine(r, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

static void draw_on_paint_layer(GraphicItem &item, SDL_Renderer *r, int x1, int y1, int x2, int y2, SDL_Color color, int size, bool erase)
{
    if (!item.paint_layer)
    {
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
        item.paint_layer = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, RENDER_W, RENDER_H);
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

        SDL_SetTextureBlendMode(item.paint_layer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderTarget(r, item.paint_layer);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
        SDL_RenderClear(r);
    }
    SDL_SetRenderTarget(r, item.paint_layer);

    int hx1 = x1 * HIRES_MULT, hy1 = y1 * HIRES_MULT;
    int hx2 = x2 * HIRES_MULT, hy2 = y2 * HIRES_MULT;
    int hsize = size * HIRES_MULT;

    int dx = hx2 - hx1, dy = hy2 - hy1;
    int steps = std::max(abs(dx), abs(dy));
    if (steps == 0)
    {
        fill_circle_local(r, hx1, hy1, hsize, erase, color);
    }
    else
    {
        float x_inc = dx / (float)steps;
        float y_inc = dy / (float)steps;
        float cx = hx1, cy = hy1;
        for (int i = 0; i <= steps; i++)
        {
            fill_circle_local(r, (int)cx, (int)cy, hsize, erase, color);
            cx += x_inc;
            cy += y_inc;
        }
    }
    SDL_SetRenderTarget(r, NULL);
}

void get_canvas_bounds(const GraphicItem &item, SDL_Rect canvas_rect, SDL_Rect &dst_out, float &scale_out)
{
    (void)item;
    float scale = std::min(canvas_rect.w / (float)LOGICAL_W, canvas_rect.h / (float)LOGICAL_H);
    int fw = LOGICAL_W * scale, fh = LOGICAL_H * scale;
    dst_out = {canvas_rect.x + (canvas_rect.w - fw) / 2, canvas_rect.y + (canvas_rect.h - fh) / 2, fw, fh};
    scale_out = scale;
}

void update_composed_texture(GraphicItem &item, SDL_Renderer *r, TTF_Font *font)
{
    if (!item.composed_texture)
    {
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
        item.composed_texture = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, RENDER_W, RENDER_H);
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
        SDL_SetTextureBlendMode(item.composed_texture, SDL_BLENDMODE_BLEND);
    }

    SDL_SetRenderTarget(r, item.composed_texture);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
    SDL_RenderClear(r);

    SDL_RendererFlip flip = (SDL_RendererFlip)((item.flip_h ? SDL_FLIP_HORIZONTAL : 0) | (item.flip_v ? SDL_FLIP_VERTICAL : 0));

    // ---> FIXED: READ FROM ORIGINAL_TEXTURE TO PREVENT LOOP GLITCH <---
    if (item.original_texture)
    {
        int img_w, img_h;
        SDL_QueryTexture(item.original_texture, NULL, NULL, &img_w, &img_h);
        float scale_x = (float)RENDER_W / img_w;
        float scale_y = (float)RENDER_H / img_h;
        float scale = std::min(scale_x, scale_y);

        int final_w = (int)(img_w * scale);
        int final_h = (int)(img_h * scale);

        SDL_Rect dst = {(RENDER_W - final_w) / 2, (RENDER_H - final_h) / 2, final_w, final_h};
        SDL_SetTextureBlendMode(item.original_texture, SDL_BLENDMODE_NONE);
        SDL_RenderCopyEx(r, item.original_texture, NULL, &dst, 0, NULL, flip);
        SDL_SetTextureBlendMode(item.original_texture, SDL_BLENDMODE_BLEND);
    }

    if (item.paint_layer)
    {
        SDL_SetTextureBlendMode(item.paint_layer, SDL_BLENDMODE_BLEND);
        SDL_RenderCopyEx(r, item.paint_layer, NULL, NULL, 0, NULL, flip);
    }

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    for (auto &sh : item.shapes)
    {
        // Compute text width naturally to prevent squishing
        if (sh.type == SHAPE_TEXT && !sh.text.empty() && font)
        {
            int tw = 0, th = 0;
            TTF_SizeUTF8(font, sh.text.c_str(), &tw, &th);
            float aspect = (float)tw / (float)th;
            if (sh.rect.h < 0)
                sh.rect.h = -sh.rect.h; // Text can't have negative height
            sh.rect.w = (int)(sh.rect.h * aspect);
        }

        SDL_Rect sr = get_normalized_rect(sh.rect);
        if (item.flip_h)
            sr.x = LOGICAL_W - sr.x - sr.w;
        if (item.flip_v)
            sr.y = LOGICAL_H - sr.y - sr.h;

        sr.x *= HIRES_MULT;
        sr.y *= HIRES_MULT;
        sr.w *= HIRES_MULT;
        sr.h *= HIRES_MULT;

        if (sh.type == SHAPE_RECT)
        {
            SDL_SetRenderDrawColor(r, sh.color.r, sh.color.g, sh.color.b, 255);
            SDL_RenderFillRect(r, &sr);
        }
        else if (sh.type == SHAPE_CIRCLE)
        {
            renderer_fill_circle(r, sr.x + sr.w / 2, sr.y + sr.h / 2, sr.w / 2, sh.color.r, sh.color.g, sh.color.b);
        }
        else if (sh.type == SHAPE_TEXT)
        {
            if (!sh.text.empty() && font)
            {
                SDL_Color sc = sh.color;
                SDL_Surface *s = TTF_RenderUTF8_Blended(font, sh.text.c_str(), sc);
                if (s)
                {
                    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
                    SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
                    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
                    SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
                    SDL_RenderCopy(r, t, NULL, &sr);
                    SDL_DestroyTexture(t);
                    SDL_FreeSurface(s);
                }
            }
        }
    }
    SDL_SetRenderTarget(r, NULL);
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

                SDL_Rect del_r = rects.thumb_dels[i];
                int cx = del_r.x + del_r.w / 2;
                int cy = del_r.y + del_r.h / 2;
                renderer_fill_circle(r, cx, cy, del_r.w / 2, 255, 60, 60);
                SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
                int d = 4;
                for (int w = -1; w <= 1; w++)
                {
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

            SDL_Texture *thumb_tex = (*items)[i].composed_texture ? (*items)[i].composed_texture : (*items)[i].texture;
            if (thumb_tex)
            {
                SDL_Rect img_r = {box.x + 10, box.y + 10, box.w - 20, box.h - 20};
                renderer_draw_texture_fit(r, thumb_tex, &img_r);
            }
        }
    }

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

    if (items && selected_idx >= 0 && selected_idx < (int)items->size())
    {
        GraphicItem &item = const_cast<GraphicItem &>((*items)[selected_idx]);

        if (item.composed_texture)
        {
            SDL_Rect dst;
            float scale;
            get_canvas_bounds(item, canvas, dst, scale);
            SDL_RenderCopy(r, item.composed_texture, NULL, &dst);

            if (state.active_tool == TOOL_POINTER && state.active_shape_index >= 0 && state.active_shape_index < (int)item.shapes.size())
            {
                auto &sh = item.shapes[state.active_shape_index];

                SDL_Rect norm_r = get_normalized_rect(sh.rect);
                int act_x = norm_r.x, act_y = norm_r.y;
                if (item.flip_h)
                    act_x = LOGICAL_W - act_x - norm_r.w;
                if (item.flip_v)
                    act_y = LOGICAL_H - act_y - norm_r.h;

                SDL_Rect sel = {(int)(dst.x + act_x * scale), (int)(dst.y + act_y * scale), (int)(norm_r.w * scale), (int)(norm_r.h * scale)};
                SDL_SetRenderDrawColor(r, 76, 151, 255, 255);
                SDL_RenderDrawRect(r, &sel);
                SDL_Rect hand = {sel.x + sel.w - 4, sel.y + sel.h - 4, 8, 8};
                SDL_RenderFillRect(r, &hand);
            }
        }
    }
}

bool costumes_tab_handle_event(const SDL_Event &e, AppState &state, SDL_Renderer *renderer, TTF_Font *font)
{
    (void)font;
    CostumesRects rects = get_costumes_rects(state);
    auto point_in = [](const SDL_Rect &r, int x, int y)
    { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; };

    GraphicItem *item = nullptr;
    int selected_idx = 0;
    if (state.editing_target_is_stage && state.selected_backdrop >= 0 && state.selected_backdrop < (int)state.backdrops.size())
    {
        item = &state.backdrops[state.selected_backdrop];
        selected_idx = state.selected_backdrop;
    }
    else if (!state.editing_target_is_stage && state.selected_sprite >= 0 && state.selected_sprite < (int)state.sprites.size())
    {
        item = &state.sprites[state.selected_sprite].costumes[state.sprites[state.selected_sprite].selected_costume];
        selected_idx = state.sprites[state.selected_sprite].selected_costume;
    }

    if (e.type == SDL_KEYDOWN && state.active_tool == TOOL_POINTER && state.active_input != INPUT_COSTUME_TEXT)
    {
        if (e.key.keysym.sym == SDLK_DELETE || e.key.keysym.sym == SDLK_BACKSPACE)
        {
            if (item && state.active_shape_index >= 0 && state.active_shape_index < (int)item->shapes.size())
            {
                item->shapes.erase(item->shapes.begin() + state.active_shape_index);
                state.active_shape_index = -1;
                return true;
            }
        }
    }

    if (e.type == SDL_MOUSEMOTION)
    {
        if (g_is_drawing && item && (state.active_tool == TOOL_BRUSH || state.active_tool == TOOL_ERASER))
        {
            SDL_Rect dst;
            float scale;
            get_canvas_bounds(*item, rects.canvas, dst, scale);
            int tx = (e.motion.x - dst.x) / scale, ty = (e.motion.y - dst.y) / scale;
            int real_tx = item->flip_h ? (LOGICAL_W - tx) : tx;
            int real_ty = item->flip_v ? (LOGICAL_H - ty) : ty;

            int brush_rad = std::max(2, (int)(4.0f / scale));

            draw_on_paint_layer(*item, renderer, g_last_mouse_x, g_last_mouse_y, real_tx, real_ty, state.active_color, brush_rad, state.active_tool == TOOL_ERASER);
            g_last_mouse_x = real_tx;
            g_last_mouse_y = real_ty;
            return true;
        }
        if (g_is_dragging && item && state.active_shape_index >= 0 && state.active_shape_index < (int)item->shapes.size())
        {
            SDL_Rect dst;
            float scale;
            get_canvas_bounds(*item, rects.canvas, dst, scale);
            int tx = (e.motion.x - dst.x) / scale, ty = (e.motion.y - dst.y) / scale;
            int real_tx = item->flip_h ? (LOGICAL_W - tx) : tx;
            int real_ty = item->flip_v ? (LOGICAL_H - ty) : ty;
            int dx = real_tx - g_last_mouse_x, dy = real_ty - g_last_mouse_y;

            auto &sh = item->shapes[state.active_shape_index];
            if (g_resize_handle == 0)
            {
                sh.rect.x += item->flip_h ? -dx : dx;
                sh.rect.y += item->flip_v ? -dy : dy;
            }
            else
            {
                if (sh.type == SHAPE_TEXT)
                {
                    sh.rect.h += item->flip_v ? -dy : dy;
                    if (sh.rect.h < 10)
                        sh.rect.h = 10;
                }
                else
                {
                    sh.rect.w += item->flip_h ? -dx : dx;
                    sh.rect.h += item->flip_v ? -dy : dy;
                }
            }
            g_last_mouse_x = real_tx;
            g_last_mouse_y = real_ty;
            return true;
        }
    }
    else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
    {
        g_is_drawing = false;
        g_is_dragging = false;
        return false;
    }
    else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        int mx = e.button.x, my = e.button.y;

        for (size_t i = 0; i < rects.thumbs.size(); ++i)
        {
            if (point_in(rects.thumb_dels[i], mx, my) && selected_idx == (int)i)
            {
                if (state.editing_target_is_stage && state.backdrops.size() > 1)
                {
                    state.backdrops.erase(state.backdrops.begin() + i);
                    if (state.selected_backdrop >= (int)state.backdrops.size())
                        state.selected_backdrop = state.backdrops.size() - 1;
                }
                else if (!state.editing_target_is_stage && state.sprites[state.selected_sprite].costumes.size() > 1)
                {
                    auto &spr = state.sprites[state.selected_sprite];
                    spr.costumes.erase(spr.costumes.begin() + i);
                    if (spr.selected_costume >= (int)spr.costumes.size())
                        spr.selected_costume = spr.costumes.size() - 1;
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

        if (point_in(rects.tools[1], mx, my))
        {
            if (state.active_tool == TOOL_BRUSH)
                state.active_input = INPUT_PEN_COLOR_PICKER;
            else
                state.active_tool = TOOL_BRUSH;
            return true;
        }
        if (point_in(rects.tools[3], mx, my))
        {
            if (state.active_tool == TOOL_TEXT)
                state.active_input = INPUT_PEN_COLOR_PICKER;
            else
                state.active_tool = TOOL_TEXT;
            return true;
        }
        if (point_in(rects.tools[4], mx, my))
        {
            if (state.active_tool == TOOL_FILL)
                state.active_input = INPUT_PEN_COLOR_PICKER;
            else
                state.active_tool = TOOL_FILL;
            return true;
        }

        // Rect and Circle do not trigger the color picker!
        if (point_in(rects.tools[5], mx, my))
        {
            state.active_tool = TOOL_RECT;
            return true;
        }
        if (point_in(rects.tools[6], mx, my))
        {
            state.active_tool = TOOL_CIRCLE;
            return true;
        }
        for (int i = 0; i < 7; ++i)
            if (point_in(rects.tools[i], mx, my))
            {
                state.active_tool = (EditTool)i;
                return true;
            }

        if (point_in(rects.flip_h, mx, my) && item)
        {
            item->flip_h = !item->flip_h;
            return true;
        }
        if (point_in(rects.flip_v, mx, my) && item)
        {
            item->flip_v = !item->flip_v;
            return true;
        }
        if (point_in(rects.del_tool, mx, my) && item)
        {
            item->shapes.clear();
            if (item->paint_layer)
            {
                SDL_DestroyTexture(item->paint_layer);
                item->paint_layer = nullptr;
            }
            return true;
        }
        if (point_in(rects.add_btn, mx, my))
        {
            state.trigger_costume_import = true;
            return true;
        }

        if (point_in(rects.canvas, mx, my) && item)
        {
            if (state.active_input == INPUT_COSTUME_TEXT)
            {
                state.active_input = INPUT_NONE;
            }
            SDL_Rect dst;
            float scale;
            get_canvas_bounds(*item, rects.canvas, dst, scale);

            int tx = (mx - dst.x) / scale;
            int ty = (my - dst.y) / scale;

            int real_tx = item->flip_h ? (LOGICAL_W - tx) : tx;
            int real_ty = item->flip_v ? (LOGICAL_H - ty) : ty;
            int brush_rad = std::max(2, (int)(4.0f / scale));

            if (state.active_tool == TOOL_POINTER)
            {
                state.active_shape_index = -1;
                for (int i = (int)item->shapes.size() - 1; i >= 0; --i)
                {
                    SDL_Rect norm_sr = get_normalized_rect(item->shapes[i].rect);
                    SDL_Rect hand = {norm_sr.x + norm_sr.w - 10, norm_sr.y + norm_sr.h - 10, 20, 20};
                    if (real_tx >= hand.x && real_tx <= hand.x + hand.w && real_ty >= hand.y && real_ty <= hand.y + hand.h)
                    {
                        state.active_shape_index = i;
                        g_resize_handle = 1;
                        g_is_dragging = true;
                        g_last_mouse_x = real_tx;
                        g_last_mouse_y = real_ty;
                        return true;
                    }
                    if (real_tx >= norm_sr.x && real_tx <= norm_sr.x + norm_sr.w && real_ty >= norm_sr.y && real_ty <= norm_sr.y + norm_sr.h)
                    {
                        state.active_shape_index = i;
                        g_resize_handle = 0;
                        g_is_dragging = true;
                        g_last_mouse_x = real_tx;
                        g_last_mouse_y = real_ty;
                        return true;
                    }
                }
            }
            else if (state.active_tool == TOOL_BRUSH || state.active_tool == TOOL_ERASER)
            {
                g_is_drawing = true;
                g_last_mouse_x = real_tx;
                g_last_mouse_y = real_ty;
                draw_on_paint_layer(*item, renderer, real_tx, real_ty, real_tx, real_ty, state.active_color, brush_rad, state.active_tool == TOOL_ERASER);
                return true;
            }
            else if (state.active_tool == TOOL_TEXT)
            {
                int th = std::max(20, (int)(30.0f / scale));
                GraphicShape sh;
                sh.type = SHAPE_TEXT;
                sh.rect = {real_tx, real_ty, 20, th};
                sh.color = state.active_color;
                sh.text = "Text";
                item->shapes.push_back(sh);
                state.active_shape_index = item->shapes.size() - 1;
                state.active_input = INPUT_COSTUME_TEXT;
                state.input_buffer = "Text";
                return true;
            }
            else if (state.active_tool == TOOL_RECT || state.active_tool == TOOL_CIRCLE)
            {
                GraphicShape sh;
                sh.type = (state.active_tool == TOOL_RECT) ? SHAPE_RECT : SHAPE_CIRCLE;
                sh.rect = {real_tx, real_ty, 0, 0};
                sh.color = state.active_color;
                item->shapes.push_back(sh);
                state.active_shape_index = item->shapes.size() - 1;
                g_resize_handle = 1;
                g_is_dragging = true;
                g_last_mouse_x = real_tx;
                g_last_mouse_y = real_ty;
                return true;
            }
            else if (state.active_tool == TOOL_FILL)
            {
                for (int i = (int)item->shapes.size() - 1; i >= 0; --i)
                {
                    SDL_Rect norm_sr = get_normalized_rect(item->shapes[i].rect);
                    if (real_tx >= norm_sr.x && real_tx <= norm_sr.x + norm_sr.w && real_ty >= norm_sr.y && real_ty <= norm_sr.y + norm_sr.h)
                    {
                        item->shapes[i].color = state.active_color;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}