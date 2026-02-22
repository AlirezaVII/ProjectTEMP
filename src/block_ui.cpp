#include "block_ui.h"
#include "renderer.h"
#include "SDL_ttf.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

static void draw_reporter_shape(SDL_Renderer *r, const SDL_Rect &br, Color col, bool ghost);

/* ---------- small text helpers ---------- */

static int text_w(TTF_Font *f, const char *txt)
{
    int w = 0, h = 0;
    if (TTF_SizeUTF8(f, txt, &w, &h) != 0)
        return 0;
    return w;
}

static Color shade(Color c, float k)
{
    auto clamp255 = [](int v)
    { return v < 0 ? 0 : (v > 255 ? 255 : v); };
    Color o;
    o.r = clamp255((int)(c.r * k));
    o.g = clamp255((int)(c.g * k));
    o.b = clamp255((int)(c.b * k));
    return o;
}

static void draw_text(SDL_Renderer *r, TTF_Font *f, const char *txt,
                      int x, int y, Color c)
{
    if (!r || !f || !txt || !txt[0])
        return;

    SDL_Color sc{(Uint8)c.r, (Uint8)c.g, (Uint8)c.b, 255};
    SDL_Surface *surf = TTF_RenderUTF8_Blended(f, txt, sc);
    if (!surf)
        return;

    SDL_Texture *t = SDL_CreateTextureFromSurface(r, surf);
    if (!t)
    {
        SDL_FreeSurface(surf);
        return;
    }

    SDL_Rect dst{x, y, surf->w, surf->h};
    SDL_RenderCopy(r, t, nullptr, &dst);

    SDL_DestroyTexture(t);
    SDL_FreeSurface(surf);
}

/* ---------- caret ---------- */

static void draw_caret(SDL_Renderer *r, int cx, int cy, Color c)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 200);
    SDL_RenderDrawLine(r, cx - 4, cy - 2, cx, cy + 2);
    SDL_RenderDrawLine(r, cx, cy + 2, cx + 4, cy - 2);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

/* ---------- base stack shape ---------- */

MotionBlockMetrics motion_block_metrics()
{
    MotionBlockMetrics m;
    m.h = 40;
    m.radius = 8;
    m.notch_w = 32;
    m.notch_h = 8;
    m.notch_x = 18;
    m.notch_y = 0;
    m.overlap = 6;
    return m;
}

static SDL_Rect input_capsule_rect(int x, int y, int w, int h)
{
    SDL_Rect r{x, y, w, h};
    return r;
}

static void draw_input_capsule(SDL_Renderer *r, const SDL_Rect &rc,
                               bool selected)
{
    renderer_fill_rounded_rect(r, &rc, rc.h / 2, 255, 255, 255);

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, selected ? 80 : 45);
    SDL_RenderDrawRect(r, &rc);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

static void draw_dropdown_capsule(SDL_Renderer *r, const SDL_Rect &rc, Color base)
{
    renderer_fill_rounded_rect(r, &rc, rc.h / 2, base.r, base.g, base.b);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 35);
    SDL_RenderDrawRect(r, &rc);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

static void draw_stack_shape(SDL_Renderer *r, const SDL_Rect &br,
                             Color col, Color panel_bg,
                             bool ghost)
{
    MotionBlockMetrics m = motion_block_metrics();

    float gk = ghost ? 0.75f : 1.0f;

    Color border = shade(col, 0.80f * gk);
    Color fill = shade(col, 1.00f * gk);

    renderer_fill_rounded_rect(r, &br, m.radius, border.r, border.g, border.b);

    SDL_Rect inner{br.x + 1, br.y + 1, br.w - 2, br.h - 2};
    renderer_fill_rounded_rect(r, &inner, std::max(0, m.radius - 1), fill.r, fill.g, fill.b);

    SDL_Rect topCut{
        br.x + m.notch_x,
        br.y,
        m.notch_w,
        m.notch_h};
    renderer_fill_rounded_rect(r, &topCut, m.notch_h / 2, panel_bg.r, panel_bg.g, panel_bg.b);

    SDL_Rect botCut{
        br.x + m.notch_x,
        br.y + br.h - m.notch_h,
        m.notch_w,
        m.notch_h};
    renderer_fill_rounded_rect(r, &botCut, m.notch_h / 2, panel_bg.r, panel_bg.g, panel_bg.b);

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, ghost ? 18 : 22);
    SDL_Rect sh{topCut.x, topCut.y + 1, topCut.w, 2};
    SDL_RenderFillRect(r, &sh);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

static void draw_stack_shape_custom(SDL_Renderer *r, const SDL_Rect &br,
                                    Color col, Color panel_bg,
                                    bool ghost,
                                    bool top_notch,
                                    bool bottom_notch)
{
    MotionBlockMetrics m = motion_block_metrics();

    float gk = ghost ? 0.75f : 1.0f;
    Color border = shade(col, 0.80f * gk);
    Color fill = shade(col, 1.00f * gk);

    renderer_fill_rounded_rect(r, &br, m.radius, border.r, border.g, border.b);

    SDL_Rect inner{br.x + 1, br.y + 1, br.w - 2, br.h - 2};
    renderer_fill_rounded_rect(r, &inner, std::max(0, m.radius - 1), fill.r, fill.g, fill.b);

    if (top_notch)
    {
        SDL_Rect topCut{
            br.x + m.notch_x,
            br.y,
            m.notch_w,
            m.notch_h};
        renderer_fill_rounded_rect(r, &topCut, m.notch_h / 2, panel_bg.r, panel_bg.g, panel_bg.b);

        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, 0, 0, 0, ghost ? 18 : 22);
        SDL_Rect sh{topCut.x, topCut.y + 1, topCut.w, 2};
        SDL_RenderFillRect(r, &sh);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    }

    if (bottom_notch)
    {
        SDL_Rect botCut{
            br.x + m.notch_x,
            br.y + br.h - m.notch_h,
            m.notch_w,
            m.notch_h};
        renderer_fill_rounded_rect(r, &botCut, m.notch_h / 2, panel_bg.r, panel_bg.g, panel_bg.b);
    }
}

/* ---------------- Motion ---------------- */

int motion_block_width(MotionBlockType type)
{
    switch (type)
    {
    case MB_MOVE_STEPS:
        return 230;
    case MB_TURN_RIGHT_DEG:
        return 230;
    case MB_TURN_LEFT_DEG:
        return 230;
    case MB_GO_TO_TARGET:
        return 250;
    case MB_GO_TO_XY:
        return 260;
    case MB_CHANGE_X_BY:
        return 220;
    case MB_CHANGE_Y_BY:
        return 220;
    case MB_POINT_IN_DIR:
        return 250;
    default:
        return 230;
    }
}

SDL_Rect motion_block_rect(MotionBlockType type, int x, int y)
{
    MotionBlockMetrics m = motion_block_metrics();
    SDL_Rect r{x, y, motion_block_width(type), m.h};
    return r;
}

void motion_block_draw(SDL_Renderer *r, TTF_Font *font, const Textures &tex,
                       MotionBlockType type, int x, int y,
                       int a, int b, GoToTarget target,
                       bool ghost, Color panel_bg,
                       int selected_field)
{
    motion_block_draw(r, font, tex, type, x, y,
                      a, b, target, ghost, panel_bg,
                      selected_field, nullptr, nullptr);
}

void motion_block_draw(SDL_Renderer *r, TTF_Font *font, const Textures &tex,
                       MotionBlockType type, int x, int y,
                       int a, int b, GoToTarget target,
                       bool ghost, Color panel_bg,
                       int selected_field,
                       const char *override_field0_text,
                       const char *override_field1_text)
{
    Color motion_col = {76, 151, 255};
    SDL_Rect br = motion_block_rect(type, x, y);
    draw_stack_shape(r, br, motion_col, panel_bg, ghost);

    int padding_x = 12;
    int cy = br.y + (br.h - 16) / 2;

    Color txt_col = {255, 255, 255};

    char bufA[32];
    std::snprintf(bufA, sizeof(bufA), "%d", a);
    char bufB[32];
    std::snprintf(bufB, sizeof(bufB), "%d", b);

    int cap_h = 22;
    int cap_y = br.y + (br.h - cap_h) / 2;

    int cur_x = br.x + padding_x;

    auto draw_word = [&](const char *w)
    {
        draw_text(r, font, w, cur_x, cy, txt_col);
        cur_x += text_w(font, w) + 6;
    };

    auto draw_num_caps = [&](const char *num_default,
                             const char *override_txt,
                             int field_index,
                             int cap_w)
    {
        const char *show = num_default;
        if (override_txt)
            show = override_txt;

        SDL_Rect cap = input_capsule_rect(cur_x, cap_y, cap_w, cap_h);
        draw_input_capsule(r, cap, selected_field == field_index);

        int tw = text_w(font, show);
        int tx = (tw <= cap.w - 10) ? (cap.x + (cap.w - tw) / 2) : (cap.x + 6);
        int ty = cap.y + (cap.h - 16) / 2;
        draw_text(r, font, show, tx, ty, (Color){40, 40, 40});

        cur_x += cap.w + 6;
    };

    switch (type)
    {
    case MB_MOVE_STEPS:
    {
        draw_word("move");
        draw_num_caps(bufA, override_field0_text, 0, 40);
        draw_word("steps");
    }
    break;

    case MB_TURN_RIGHT_DEG:
    {
        draw_word("turn");
        SDL_Rect ic{cur_x, br.y + 10, 18, 18};
        if (tex.rotate_right)
            renderer_draw_texture_fit(r, tex.rotate_right, &ic);
        cur_x += 18 + 6;
        draw_num_caps(bufA, override_field0_text, 0, 40);
        draw_word("degrees");
    }
    break;

    case MB_TURN_LEFT_DEG:
    {
        draw_word("turn");
        SDL_Rect ic{cur_x, br.y + 10, 18, 18};
        if (tex.rotate_left)
            renderer_draw_texture_fit(r, tex.rotate_left, &ic);
        cur_x += 18 + 6;
        draw_num_caps(bufA, override_field0_text, 0, 40);
        draw_word("degrees");
    }
    break;

    case MB_GO_TO_XY:
    {
        draw_word("go");
        draw_word("to");
        draw_word("x");
        draw_num_caps(bufA, override_field0_text, 0, 48);
        draw_word("y");
        draw_num_caps(bufB, override_field1_text, 1, 48);
    }
    break;

    case MB_CHANGE_X_BY:
    {
        draw_word("change");
        draw_word("x");
        draw_word("by");
        draw_num_caps(bufA, override_field0_text, 0, 48);
    }
    break;

    case MB_CHANGE_Y_BY:
    {
        draw_word("change");
        draw_word("y");
        draw_word("by");
        draw_num_caps(bufA, override_field0_text, 0, 48);
    }
    break;

    case MB_POINT_IN_DIR:
    {
        draw_word("point");
        draw_word("in");
        draw_word("direction");
        draw_num_caps(bufA, override_field0_text, 0, 48);
    }
    break;

    case MB_GO_TO_TARGET:
    {
        draw_word("go");
        draw_word("to");

        const char *opt = (target == TARGET_MOUSE_POINTER) ? "mouse-pointer" : "random position";
        int opt_w = text_w(font, opt);
        int cap_w = std::max(120, opt_w + 24);

        SDL_Rect dd{cur_x, cap_y, cap_w, cap_h};
        draw_dropdown_capsule(r, dd, (Color){59, 120, 220});

        int tx = dd.x + 10;
        int ty = dd.y + (dd.h - 16) / 2;
        draw_text(r, font, opt, tx, ty, (Color){255, 255, 255});

        draw_caret(r, dd.x + dd.w - 12, dd.y + dd.h / 2, (Color){255, 255, 255});
    }
    break;

    default:
    {
        draw_word("motion");
    }
    break;
    }
}

int motion_block_hittest_field(TTF_Font *font, MotionBlockType type,
                               int x, int y,
                               int a, int b, GoToTarget target,
                               int px, int py)
{
    (void)a;
    (void)b;
    (void)target;

    SDL_Rect br = motion_block_rect(type, x, y);
    if (!(px >= br.x && px < br.x + br.w && py >= br.y && py < br.y + br.h))
        return -1;

    int padding_x = 12;
    int cap_h = 22;
    int cap_y = br.y + (br.h - cap_h) / 2;
    int cur_x = br.x + padding_x;

    auto adv_word = [&](const char *w)
    { cur_x += text_w(font, w) + 6; };
    auto cap_rect = [&](int w)
    {
        SDL_Rect rc{cur_x, cap_y, w, cap_h};
        cur_x += w + 6;
        return rc;
    };
    auto in = [&](const SDL_Rect &rc)
    {
        return px >= rc.x && px < rc.x + rc.w && py >= rc.y && py < rc.y + rc.h;
    };

    switch (type)
    {
    case MB_MOVE_STEPS:
        adv_word("move");
        if (in(cap_rect(40)))
            return 0;
        return -1;

    case MB_TURN_RIGHT_DEG:
        adv_word("turn");
        cur_x += 18 + 6;
        if (in(cap_rect(40)))
            return 0;
        return -1;

    case MB_TURN_LEFT_DEG:
        adv_word("turn");
        cur_x += 18 + 6;
        if (in(cap_rect(40)))
            return 0;
        return -1;

    case MB_GO_TO_XY:
        adv_word("go");
        adv_word("to");
        adv_word("x");
        if (in(cap_rect(48)))
            return 0;
        adv_word("y");
        if (in(cap_rect(48)))
            return 1;
        return -1;

    case MB_CHANGE_X_BY:
        adv_word("change");
        adv_word("x");
        adv_word("by");
        if (in(cap_rect(48)))
            return 0;
        return -1;

    case MB_CHANGE_Y_BY:
        adv_word("change");
        adv_word("y");
        adv_word("by");
        if (in(cap_rect(48)))
            return 0;
        return -1;

    case MB_POINT_IN_DIR:
        adv_word("point");
        adv_word("in");
        adv_word("direction");
        if (in(cap_rect(48)))
            return 0;
        return -1;

    case MB_GO_TO_TARGET:
    {
        adv_word("go");
        adv_word("to");
        SDL_Rect dd = cap_rect(140);
        if (in(dd))
            return -2;
        return -1;
    }

    default:
        return -1;
    }
}

/* ---------------- Looks (NO reporters) ---------------- */

int looks_block_width(LooksBlockType type)
{
    switch (type)
    {
    case LB_SAY_FOR:
        return 300;
    case LB_SAY:
        return 250;
    case LB_THINK_FOR:
        return 320;
    case LB_THINK:
        return 260;
    case LB_SWITCH_COSTUME_TO:
        return 300;
    case LB_NEXT_COSTUME:
        return 220;
    case LB_SWITCH_BACKDROP_TO:
        return 310;
    case LB_NEXT_BACKDROP:
        return 220;
    case LB_CHANGE_SIZE_BY:
        return 270;
    case LB_SET_SIZE_TO:
        return 270;
    case LB_SHOW:
        return 160;
    case LB_HIDE:
        return 160;
    case LB_GO_TO_LAYER:
        return 260;
    case LB_GO_LAYERS:
        return 310;
    case LB_SIZE:
        return 80;
    case LB_BACKDROP_NUM_NAME:
        return 170;
    case LB_COSTUME_NUM_NAME:
        return 170;
    default:
        return 260;
    }
}

SDL_Rect looks_block_rect(LooksBlockType type, int x, int y)
{
    MotionBlockMetrics m = motion_block_metrics();
    SDL_Rect r{x, y, looks_block_width(type), m.h};
    return r;
}

void looks_block_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state, LooksBlockType type, int x, int y, const std::string &text, int a, int b, int opt, bool ghost, Color panel_bg, int selected_field, const char *override_field0_text, const char *override_field1_text)
{
    (void)b;
    Color looks_col = {153, 102, 255};
    SDL_Rect br = looks_block_rect(type, x, y);
    bool is_reporter = (type == LB_SIZE || type == LB_BACKDROP_NUM_NAME || type == LB_COSTUME_NUM_NAME);
    if (is_reporter)
        draw_reporter_shape(r, br, looks_col, ghost);
    else
        draw_stack_shape(r, br, looks_col, panel_bg, ghost);

    int padding_x = 12;
    int cy = br.y + (br.h - 16) / 2;
    Color txt_col = {255, 255, 255};
    char bufA[32];
    std::snprintf(bufA, sizeof(bufA), "%d", a);
    char bufB[32];
    std::snprintf(bufB, sizeof(bufB), "%d", b);
    int cap_h = 22;
    int cap_y = br.y + (br.h - cap_h) / 2;
    int cur_x = br.x + padding_x;
    auto draw_word = [&](const char *w)
    { draw_text(r, font, w, cur_x, cy, txt_col); cur_x += text_w(font, w) + 6; };
    auto draw_text_caps = [&](const char *default_txt, const char *override_txt, int field_index, int cap_w)
    { const char *show = default_txt; if (override_txt && selected_field == field_index) show = override_txt; SDL_Rect cap = input_capsule_rect(cur_x, cap_y, cap_w, cap_h); draw_input_capsule(r, cap, selected_field == field_index); int tw = text_w(font, show); int tx = (tw <= cap.w - 10) ? (cap.x + (cap.w - tw) / 2) : (cap.x + 6); int ty = cap.y + (cap.h - 16) / 2; draw_text(r, font, show, tx, ty, (Color){40, 40, 40}); cur_x += cap.w + 6; };
    auto draw_num_caps = [&](const char *num_default, const char *override_txt, int field_index, int cap_w)
    { draw_text_caps(num_default, override_txt, field_index, cap_w); };
    auto draw_dd = [&](const char *txt)
    { int tw = text_w(font, txt); int cap_w = std::max(90, tw + 26); SDL_Rect dd{cur_x, cap_y, cap_w, cap_h}; draw_dropdown_capsule(r, dd, (Color){120, 70, 220}); int tx = dd.x + 10; int ty = dd.y + (dd.h - 16) / 2; draw_text(r, font, txt, tx, ty, (Color){255, 255, 255}); draw_caret(r, dd.x + dd.w - 12, dd.y + dd.h / 2, (Color){255, 255, 255}); cur_x += cap_w + 6; };

    switch (type)
    {
    case LB_SAY_FOR:
        draw_word("say");
        draw_text_caps(text.c_str(), override_field0_text, 0, 110);
        draw_word("for");
        draw_num_caps(bufB, override_field1_text, 1, 48);
        draw_word("seconds");
        break;
    case LB_SAY:
        draw_word("say");
        draw_text_caps(text.c_str(), override_field0_text, 0, 120);
        break;
    case LB_THINK_FOR:
        draw_word("think");
        draw_text_caps(text.c_str(), override_field0_text, 0, 120);
        draw_word("for");
        draw_num_caps(bufB, override_field1_text, 1, 48);
        draw_word("seconds");
        break;
    case LB_THINK:
        draw_word("think");
        draw_text_caps(text.c_str(), override_field0_text, 0, 140);
        break;
    case LB_SWITCH_COSTUME_TO:
        draw_word("switch");
        draw_word("costume");
        draw_word("to");
        draw_dd((opt == 0) ? "costume1" : (opt == 1) ? "costume2"
                                                     : "costume3");
        break;
    case LB_NEXT_COSTUME:
        draw_word("next");
        draw_word("costume");
        break;
    case LB_SWITCH_BACKDROP_TO:
    {
        draw_word("switch");
        draw_word("backdrop");
        draw_word("to");
        std::string b_txt = (opt >= 0 && opt < (int)state.backdrops.size()) ? state.backdrops[opt].name : "backdrop1";
        draw_dd(b_txt.c_str());
    }
    break;
    case LB_NEXT_BACKDROP:
        draw_word("next");
        draw_word("backdrop");
        break;
    case LB_CHANGE_SIZE_BY:
        draw_word("change");
        draw_word("size");
        draw_word("by");
        draw_num_caps(bufA, override_field0_text, 0, 48);
        break;
    case LB_SET_SIZE_TO:
        draw_word("set");
        draw_word("size");
        draw_word("to");
        draw_num_caps(bufA, override_field0_text, 0, 48);
        draw_word("%");
        break;
    case LB_SHOW:
        draw_word("show");
        break;
    case LB_HIDE:
        draw_word("hide");
        break;
    case LB_GO_TO_LAYER:
        draw_word("go");
        draw_word("to");
        draw_dd(opt == 0 ? "front" : "back");
        draw_word("layer");
        break;
    case LB_GO_LAYERS:
        draw_word("go");
        draw_dd(opt == 0 ? "forward" : "backward");
        draw_num_caps(bufA, override_field0_text, 0, 48);
        draw_word("layers");
        break;
    case LB_SIZE:
        draw_word("size");
        break;
    case LB_BACKDROP_NUM_NAME:
        draw_word("backdrop");
        draw_dd(opt == 0 ? "number" : "name");
        break;
    case LB_COSTUME_NUM_NAME:
        draw_word("costume");
        draw_dd(opt == 0 ? "number" : "name");
        break;
    default:
        draw_word("looks");
        break;
    }
}

int looks_block_hittest_field(TTF_Font *font, const AppState &state, LooksBlockType type, int x, int y, const std::string &text, int a, int b, int opt, int px, int py)
{
    (void)text;
    (void)a;
    (void)b;
    (void)opt;
    SDL_Rect br = looks_block_rect(type, x, y);
    if (!(px >= br.x && px < br.x + br.w && py >= br.y && py < br.y + br.h))
        return -1;
    int padding_x = 12;
    int cap_h = 22;
    int cap_y = br.y + (br.h - cap_h) / 2;
    int cur_x = br.x + padding_x;
    auto adv_word = [&](const char *w)
    { cur_x += text_w(font, w) + 6; };
    auto cap_rect = [&](int w)
    { SDL_Rect rc{cur_x, cap_y, w, cap_h}; cur_x += w + 6; return rc; };
    auto in = [&](const SDL_Rect &rc)
    { return px >= rc.x && px < rc.x + rc.w && py >= rc.y && py < rc.y + rc.h; };
    auto dd_w = [&](const char *txt)
    { return std::max(90, text_w(font, txt) + 26); };
    switch (type)
    {
    case LB_SAY_FOR:
        adv_word("say");
        if (in(cap_rect(110)))
            return 0;
        adv_word("for");
        if (in(cap_rect(48)))
            return 1;
        return -1;
    case LB_SAY:
        adv_word("say");
        if (in(cap_rect(120)))
            return 0;
        return -1;
    case LB_THINK_FOR:
        adv_word("think");
        if (in(cap_rect(120)))
            return 0;
        adv_word("for");
        if (in(cap_rect(48)))
            return 1;
        return -1;
    case LB_THINK:
        adv_word("think");
        if (in(cap_rect(140)))
            return 0;
        return -1;
    case LB_SWITCH_COSTUME_TO:
        adv_word("switch");
        adv_word("costume");
        adv_word("to");
        if (in(cap_rect(120)))
            return -2;
        return -1;
    case LB_NEXT_COSTUME:
        return -1;
    case LB_SWITCH_BACKDROP_TO:
    {
        adv_word("switch");
        adv_word("backdrop");
        adv_word("to");
        std::string b_txt = (opt >= 0 && opt < (int)state.backdrops.size()) ? state.backdrops[opt].name : "backdrop1";
        if (in(cap_rect(dd_w(b_txt.c_str()))))
            return -2;
    }
        return -1;
    case LB_NEXT_BACKDROP:
        return -1;
    case LB_CHANGE_SIZE_BY:
        adv_word("change");
        adv_word("size");
        adv_word("by");
        if (in(cap_rect(48)))
            return 0;
        return -1;
    case LB_SET_SIZE_TO:
        adv_word("set");
        adv_word("size");
        adv_word("to");
        if (in(cap_rect(48)))
            return 0;
        return -1;
    case LB_SHOW:
    case LB_HIDE:
        return -1;
    case LB_GO_TO_LAYER:
        adv_word("go");
        adv_word("to");
        if (in(cap_rect(dd_w(opt == 0 ? "front" : "back"))))
            return -2;
        return -1;
    case LB_GO_LAYERS:
        adv_word("go");
        if (in(cap_rect(dd_w(opt == 0 ? "forward" : "backward"))))
            return -2;
        if (in(cap_rect(48)))
            return 0;
        return -1;
    case LB_SIZE:
        return -1;
    case LB_BACKDROP_NUM_NAME:
        adv_word("backdrop");
        if (in(cap_rect(dd_w(opt == 0 ? "number" : "name"))))
            return -2;
        return -1;
    case LB_COSTUME_NUM_NAME:
        adv_word("costume");
        if (in(cap_rect(dd_w(opt == 0 ? "number" : "name"))))
            return -2;
        return -1;
    default:
        return -1;
    }
}

/* ---------------- Sound ---------------- */

int sound_block_width(SoundBlockType type)
{
    switch (type)
    {
    case SB_CHANGE_VOLUME_BY:
        return 300;
    case SB_SET_VOLUME_TO:
        return 280;
    case SB_STOP_ALL_SOUNDS:
        return 230;
    case SB_START_SOUND:
        return 260;
    case SB_PLAY_SOUND_UNTIL_DONE:
        return 330;
    default:
        return 260;
    }
}

SDL_Rect sound_block_rect(SoundBlockType type, int x, int y, int a, int opt)
{
    (void)a;
    (void)opt;
    MotionBlockMetrics m = motion_block_metrics();
    return {x, y, sound_block_width(type), m.h};
}

void sound_block_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state, SoundBlockType type, int x, int y, int a, int opt, bool ghost, Color panel_bg, int selected_field, const char *override_field0_text)
{
    Color sound_col = {207, 99, 207};
    SDL_Rect br = sound_block_rect(type, x, y, a, opt);
    draw_stack_shape(r, br, sound_col, panel_bg, ghost);

    int padding_x = 12;
    int cy = br.y + (br.h - 16) / 2;
    Color txt_col = {255, 255, 255};
    char bufA[32];
    std::snprintf(bufA, sizeof(bufA), "%d", a);
    int cap_h = 22;
    int cap_y = br.y + (br.h - cap_h) / 2;
    int cur_x = br.x + padding_x;

    auto draw_word = [&](const char *w)
    { draw_text(r, font, w, cur_x, cy, txt_col); cur_x += text_w(font, w) + 6; };
    auto draw_num_caps = [&](const char *num_default, int field_index, int cap_w)
    {
        const char *show = num_default;
        if (override_field0_text && selected_field == field_index)
            show = override_field0_text;
        SDL_Rect cap = input_capsule_rect(cur_x, cap_y, cap_w, cap_h);
        draw_input_capsule(r, cap, selected_field == field_index);
        int tw = text_w(font, show);
        int tx = (tw <= cap.w - 10) ? (cap.x + (cap.w - tw) / 2) : (cap.x + 6);
        draw_text(r, font, show, tx, cap.y + (cap.h - 16) / 2, (Color){40, 40, 40});
        cur_x += cap.w + 6;
    };
    auto draw_dd = [&](const char *txt)
    {
        int cap_w = std::max(90, text_w(font, txt) + 26);
        SDL_Rect dd{cur_x, cap_y, cap_w, cap_h};
        draw_dropdown_capsule(r, dd, (Color){165, 70, 165});
        draw_text(r, font, txt, dd.x + 10, dd.y + (dd.h - 16) / 2, (Color){255, 255, 255});
        draw_caret(r, dd.x + dd.w - 12, dd.y + dd.h / 2, (Color){255, 255, 255});
        cur_x += cap_w + 6;
    };

    switch (type)
    {
    case SB_PLAY_SOUND_UNTIL_DONE:
    case SB_START_SOUND:
    {
        draw_word(type == SB_PLAY_SOUND_UNTIL_DONE ? "play" : "start");
        draw_word("sound");
        std::string s_txt = "Meow";
        if (state.selected_sprite >= 0 && state.selected_sprite < (int)state.sprites.size())
        {
            const auto &spr = state.sprites[state.selected_sprite];
            if (opt >= 0 && opt < (int)spr.sounds.size())
                s_txt = spr.sounds[opt].name;
        }
        draw_dd(s_txt.c_str());
        if (type == SB_PLAY_SOUND_UNTIL_DONE)
        {
            draw_word("until");
            draw_word("done");
        }
        break;
    }
    case SB_STOP_ALL_SOUNDS:
        draw_word("stop");
        draw_word("all");
        draw_word("sounds");
        break;
    case SB_CHANGE_VOLUME_BY:
        draw_word("change");
        draw_word("volume");
        draw_word("by");
        draw_num_caps(bufA, 0, 52);
        break;
    case SB_SET_VOLUME_TO:
        draw_word("set");
        draw_word("volume");
        draw_word("to");
        draw_num_caps(bufA, 0, 52);
        draw_word("%");
        break;
    }
}

int sound_block_hittest_field(TTF_Font *font, const AppState &state, SoundBlockType type, int x, int y, int a, int opt, int px, int py)
{
    SDL_Rect br = sound_block_rect(type, x, y, a, opt);
    if (!(px >= br.x && px < br.x + br.w && py >= br.y && py < br.y + br.h))
        return -1;
    int cur_x = br.x + 12;
    int cap_y = br.y + (br.h - 22) / 2;
    auto adv_word = [&](const char *w)
    { cur_x += text_w(font, w) + 6; };
    auto cap_rect = [&](int w)
    { SDL_Rect rc{cur_x, cap_y, w, 22}; cur_x += w + 6; return rc; };
    auto in = [&](const SDL_Rect &rc)
    { return px >= rc.x && px < rc.x + rc.w && py >= rc.y && py < rc.y + rc.h; };

    switch (type)
    {
    case SB_PLAY_SOUND_UNTIL_DONE:
    case SB_START_SOUND:
    {
        adv_word(type == SB_PLAY_SOUND_UNTIL_DONE ? "play" : "start");
        adv_word("sound");
        std::string s_txt = "Meow";
        if (state.selected_sprite >= 0 && state.selected_sprite < (int)state.sprites.size())
        {
            const auto &spr = state.sprites[state.selected_sprite];
            if (opt >= 0 && opt < (int)spr.sounds.size())
                s_txt = spr.sounds[opt].name;
        }
        if (in(cap_rect(std::max(90, text_w(font, s_txt.c_str()) + 26))))
            return -2;
        return -1;
    }
    case SB_CHANGE_VOLUME_BY:
        adv_word("change");
        adv_word("volume");
        adv_word("by");
        if (in(cap_rect(52)))
            return 0;
        return -1;
    case SB_SET_VOLUME_TO:
        adv_word("set");
        adv_word("volume");
        adv_word("to");
        if (in(cap_rect(52)))
            return 0;
        return -1;
    default:
        return -1;
    }
}

/* ---------------- Events ---------------- */

/* ---------------- Events ---------------- */

static const char *events_key_label(int opt)
{
    if (opt == 0) return "space";
    if (opt == 1) return "up arrow";
    if (opt == 2) return "down arrow";
    if (opt == 3) return "left arrow";
    if (opt == 4) return "right arrow";
    if (opt >= 5 && opt <= 30) {
        static char buf[2] = {0};
        buf[0] = 'a' + (opt - 5);
        return buf;
    }
    if (opt >= 31 && opt <= 40) {
        static char buf[2] = {0};
        buf[0] = '0' + (opt - 31);
        return buf;
    }
    return "any";
}

static const char *events_message_label(const AppState &state, int opt)
{
    if (state.messages.empty()) return "message1";
    if (opt >= 0 && opt < (int)state.messages.size()) return state.messages[opt].c_str();
    return state.messages[0].c_str();
}

int events_block_width(EventsBlockType type)
{
    switch (type)
    {
    case EB_WHEN_FLAG_CLICKED: return 260;
    case EB_WHEN_KEY_PRESSED: return 330;
    case EB_WHEN_SPRITE_CLICKED: return 280;
    case EB_WHEN_I_RECEIVE: return 320;
    case EB_BROADCAST: return 260;
    default: return 260;
    }
}

SDL_Rect events_block_rect(EventsBlockType type, int x, int y, int opt)
{
    (void)opt;
    int w = events_block_width(type);
    
    // ---> FIXED: Broadcast is a normal block height, others are Hats <---
    if (type == EB_BROADCAST) return {x, y, w, 40};
    return {x, y, w, 48};
}

static void draw_events_hat(SDL_Renderer *r, const SDL_Rect &br, Color col, Color panel_bg, bool ghost)
{
    (void)panel_bg;
    int cap_h = 22;
    int cap_w = std::min(170, br.w - 40);
    SDL_Rect cap{br.x, br.y - (cap_h / 2), cap_w, cap_h};

    float gk = ghost ? 0.75f : 1.0f;
    Color border = shade(col, 0.80f * gk);
    Color fill = shade(col, 1.00f * gk);

    renderer_fill_rounded_rect(r, &cap, cap.h / 2, border.r, border.g, border.b);
    SDL_Rect inner{cap.x + 1, cap.y + 1, cap.w - 2, cap.h - 2};
    renderer_fill_rounded_rect(r, &inner, std::max(0, inner.h / 2), fill.r, fill.g, fill.b);
}

void events_block_draw(SDL_Renderer *r, TTF_Font *font, const Textures &tex, const AppState &state,
                       EventsBlockType type, int x, int y, int opt,
                       bool ghost, Color panel_bg, int selected_field)
{
    (void)selected_field;
    SDL_Rect br = events_block_rect(type, x, y, opt);
    Color col = {255, 191, 38};

    // Draw the main stack body for ALL blocks
    draw_stack_shape(r, br, col, panel_bg, ghost);

    // If it is NOT a broadcast block, draw the Hat cap on top
    if (type != EB_BROADCAST) {
        draw_events_hat(r, br, col, panel_bg, ghost);
    }

    int padding_x = 12;
    int cy = br.y + (br.h - 16) / 2;
    int cap_h = 22;
    int cap_y = br.y + (br.h - cap_h) / 2;
    
    // Shift text down slightly for Hat blocks so it centers nicely
    if (type != EB_BROADCAST) {
        cy += 4;
        cap_y += 4;
    }

    int cur_x = br.x + padding_x;
    Color txt_col = {255, 255, 255};

    auto draw_word = [&](const char *w) {
        draw_text(r, font, w, cur_x, cy, txt_col);
        cur_x += text_w(font, w) + 6;
    };
    
    auto draw_dd = [&](const char *lbl) {
        int tw = text_w(font, lbl);
        int cap_w = std::max(90, tw + 26);
        SDL_Rect dd{cur_x, cap_y, cap_w, cap_h};
        draw_dropdown_capsule(r, dd, (Color){214, 152, 15});
        draw_text(r, font, lbl, dd.x + 10, dd.y + (dd.h - 16) / 2, txt_col);
        draw_caret(r, dd.x + dd.w - 12, dd.y + dd.h / 2, txt_col);
        cur_x += cap_w + 6;
    };

    switch (type)
    {
    case EB_WHEN_FLAG_CLICKED:
    {
        draw_word("when");
        SDL_Rect ic{cur_x, br.y + 12, 24, 24};
        if (tex.green_flag) renderer_draw_texture_fit(r, tex.green_flag, &ic);
        cur_x += 24 + 6;
        draw_word("clicked");
    }
    break;
    case EB_WHEN_KEY_PRESSED:
        draw_word("when"); draw_dd(events_key_label(opt)); draw_word("key"); draw_word("pressed");
        break;
    case EB_WHEN_SPRITE_CLICKED:
        draw_word("when"); draw_word("this"); draw_word("sprite"); draw_word("clicked");
        break;
    case EB_WHEN_I_RECEIVE:
        draw_word("when"); draw_word("I"); draw_word("receive"); draw_dd(events_message_label(state, opt)); 
        break;
    case EB_BROADCAST:
        draw_word("broadcast"); draw_dd(events_message_label(state, opt));
        break;
    default:
        draw_word("events");
        break;
    }
}

int events_block_hittest_field(TTF_Font *font, const AppState &state, EventsBlockType type, int x, int y, int opt, int px, int py)
{
    SDL_Rect br = events_block_rect(type, x, y, opt);
    if (!(px >= br.x && px < br.x + br.w && py >= br.y && py < br.y + br.h)) return -1;
    
    int padding_x = 12;
    int cur_x = br.x + padding_x;
    int cap_h = 22;
    int cap_y = br.y + (br.h - cap_h) / 2;
    if (type != EB_BROADCAST) cap_y += 4;

    auto adv_word = [&](const char *w) { cur_x += text_w(font, w) + 6; };
    auto cap_rect = [&](int w) {
        SDL_Rect rc{cur_x, cap_y, w, cap_h};
        cur_x += w + 6;
        return rc;
    };
    auto dd_w = [&](const char *txt) { return std::max(90, text_w(font, txt) + 26); };
    auto in = [&](const SDL_Rect &rc) { return px >= rc.x && px < rc.x + rc.w && py >= rc.y && py < rc.y + rc.h; };

    switch (type)
    {
    case EB_WHEN_KEY_PRESSED:
        adv_word("when");
        if (in(cap_rect(dd_w(events_key_label(opt))))) return -2;
        return -1;
    case EB_WHEN_I_RECEIVE:
        adv_word("when"); adv_word("I"); adv_word("receive");
        if (in(cap_rect(dd_w(events_message_label(state, opt))))) return -2;
        return -1;
    case EB_BROADCAST:
        adv_word("broadcast");
        if (in(cap_rect(dd_w(events_message_label(state, opt))))) return -2;
        return -1;
    default: return -1;
    }
}
/* ================================================================ */
/* ================       S E N S I N G       ==================== */
/* ================================================================ */

static const char *sensing_touching_label(int opt)
{
    switch (opt)
    {
    case TOUCHING_MOUSE_POINTER:
        return "mouse-pointer";
    case TOUCHING_EDGE:
        return "edge";
    case TOUCHING_SPRITE:
        return "sprite";
    default:
        return "mouse-pointer";
    }
}

static const char *sensing_key_label(int opt)
{
    static char buf[8];
    if (opt == 0)
        return "space";
    if (opt == 1)
        return "up arrow";
    if (opt == 2)
        return "down arrow";
    if (opt == 3)
        return "left arrow";
    if (opt == 4)
        return "right arrow";
    if (opt >= 5 && opt <= 30)
    {
        char c = (char)('a' + (opt - 5));
        buf[0] = c;
        buf[1] = 0;
        return buf;
    }
    if (opt >= 31 && opt <= 40)
    {
        char c = (char)('0' + (opt - 31));
        buf[0] = c;
        buf[1] = 0;
        return buf;
    }
    return "space";
}

static const char *sensing_drag_mode_label(int opt) { return (opt == DRAG_NOT_DRAGGABLE) ? "not draggable" : "draggable"; }

static int clamp_color(int v)
{
    if (v < 0)
        return 0;
    if (v > 255)
        return 255;
    return v;
}

static void draw_boolean_shape(SDL_Renderer *r, const SDL_Rect &br, Color col, bool ghost)
{
    float gk = ghost ? 0.75f : 1.0f;
    Color border = shade(col, 0.80f * gk);
    Color fill = shade(col, 1.00f * gk);
    int point_w = br.h / 2;
    SDL_Rect center{br.x + point_w, br.y, br.w - 2 * point_w, br.h};
    SDL_Rect center_b{center.x - 1, center.y, center.w + 2, center.h};
    SDL_SetRenderDrawColor(r, border.r, border.g, border.b, 255);
    SDL_RenderFillRect(r, &center_b);
    SDL_Rect center_f{center.x, center.y + 1, center.w, center.h - 2};
    SDL_SetRenderDrawColor(r, fill.r, fill.g, fill.b, 255);
    SDL_RenderFillRect(r, &center_f);
    int mid_y = br.y + br.h / 2;
    int left_x = br.x;
    int right_of_left = br.x + point_w;
    for (int row = 0; row < br.h; row++)
    {
        int y_pos = br.y + row;
        int dist = (row <= br.h / 2) ? row : (br.h - 1 - row);
        float ratio = (float)dist / (float)(br.h / 2);
        int x_start = right_of_left - (int)(ratio * point_w);
        SDL_SetRenderDrawColor(r, border.r, border.g, border.b, 255);
        SDL_RenderDrawLine(r, x_start, y_pos, right_of_left, y_pos);
        if (row > 0 && row < br.h - 1)
        {
            SDL_SetRenderDrawColor(r, fill.r, fill.g, fill.b, 255);
            SDL_RenderDrawLine(r, x_start + 1, y_pos, right_of_left, y_pos);
        }
    }
    int left_of_right = br.x + br.w - point_w;
    int right_x = br.x + br.w;
    for (int row = 0; row < br.h; row++)
    {
        int y_pos = br.y + row;
        int dist = (row <= br.h / 2) ? row : (br.h - 1 - row);
        float ratio = (float)dist / (float)(br.h / 2);
        int x_end = left_of_right + (int)(ratio * point_w);
        SDL_SetRenderDrawColor(r, border.r, border.g, border.b, 255);
        SDL_RenderDrawLine(r, left_of_right, y_pos, x_end, y_pos);
        if (row > 0 && row < br.h - 1)
        {
            SDL_SetRenderDrawColor(r, fill.r, fill.g, fill.b, 255);
            SDL_RenderDrawLine(r, left_of_right, y_pos, x_end - 1, y_pos);
        }
    }
    (void)mid_y;
    (void)left_x;
    (void)right_x;
}

int sensing_block_width(SensingBlockType type)
{
    switch (type)
    {
    case SENSB_TOUCHING:
        return 310;
    case SENSB_ASK_AND_WAIT:
        return 350;
    case SENSB_KEY_PRESSED:
        return 300;
    case SENSB_MOUSE_DOWN:
        return 220;
    case SENSB_SET_DRAG_MODE:
        return 340;
    default:
        return 260;
    }
}

SDL_Rect sensing_block_rect(SensingBlockType type, int x, int y)
{
    MotionBlockMetrics m = motion_block_metrics();
    return {x, y, sensing_block_width(type), m.h};
}

void sensing_block_draw(SDL_Renderer *r, TTF_Font *font,
                        SensingBlockType type, int x, int y,
                        int a, int b, int opt,
                        int r1, int g1, int b1, int r2, int g2, int b2,
                        const std::string &text, bool ghost, Color panel_bg,
                        int selected_field, const char *override_field0_text)
{
    (void)b;
    Color sens_col = {90, 188, 216};
    SDL_Rect br = sensing_block_rect(type, x, y);

    bool is_hex = (type == SENSB_TOUCHING || type == SENSB_KEY_PRESSED || type == SENSB_MOUSE_DOWN);
    if (is_hex)
        draw_boolean_shape(r, br, sens_col, ghost);
    else
        draw_stack_shape(r, br, sens_col, panel_bg, ghost);

    int padding_x = is_hex ? (br.h / 2 + 6) : 12;
    int cy = br.y + (br.h - 16) / 2;
    int cap_h = 22;
    int cap_y = br.y + (br.h - cap_h) / 2;
    int cur_x = br.x + padding_x;
    Color txt_col = {255, 255, 255};

    auto draw_word = [&](const char *w)
    { draw_text(r, font, w, cur_x, cy, txt_col); cur_x += text_w(font, w) + 6; };
    auto draw_num_caps = [&](const char *num_default, const char *override_txt, int field_index, int cap_w)
    {
        const char *show = num_default;
        if (override_txt && field_index == 0)
            show = override_txt;
        SDL_Rect cap = input_capsule_rect(cur_x, cap_y, cap_w, cap_h);
        draw_input_capsule(r, cap, selected_field == field_index);
        int tw = text_w(font, show);
        int tx = (tw <= cap.w - 10) ? (cap.x + (cap.w - tw) / 2) : (cap.x + 6);
        int ty = cap.y + (cap.h - 16) / 2;
        draw_text(r, font, show, tx, ty, (Color){40, 40, 40});
        cur_x += cap.w + 6;
    };
    auto draw_dd = [&](const char *txt)
    {
        int tw = text_w(font, txt);
        int cap_w = std::max(90, tw + 26);
        SDL_Rect dd{cur_x, cap_y, cap_w, cap_h};
        draw_dropdown_capsule(r, dd, (Color){60, 150, 180});
        int tx = dd.x + 10;
        int ty = dd.y + (dd.h - 16) / 2;
        draw_text(r, font, txt, tx, ty, (Color){255, 255, 255});
        draw_caret(r, dd.x + dd.w - 12, dd.y + dd.h / 2, (Color){255, 255, 255});
        cur_x += cap_w + 6;
    };

    switch (type)
    {
    case SENSB_TOUCHING:
        draw_word("touching");
        draw_dd(sensing_touching_label(opt));
        draw_word("?");
        break;
    case SENSB_ASK_AND_WAIT:
    {
        const char *show_text = text.empty() ? "What's your name?" : text.c_str();
        if (override_field0_text)
            show_text = override_field0_text;
        draw_word("ask");
        int cap_w = 180;
        SDL_Rect cap = input_capsule_rect(cur_x, cap_y, cap_w, cap_h);
        draw_input_capsule(r, cap, selected_field == 0);
        int tw = text_w(font, show_text);
        int tx = (tw <= cap.w - 10) ? (cap.x + (cap.w - tw) / 2) : (cap.x + 6);
        int ty = cap.y + (cap.h - 16) / 2;
        draw_text(r, font, show_text, tx, ty, (Color){40, 40, 40});
        cur_x += cap_w + 6;
        draw_word("and");
        draw_word("wait");
    }
    break;
    case SENSB_KEY_PRESSED:
        draw_word("key");
        draw_dd(sensing_key_label(opt));
        draw_word("pressed?");
        break;
    case SENSB_MOUSE_DOWN:
        draw_word("mouse");
        draw_word("down?");
        break;
    case SENSB_SET_DRAG_MODE:
        draw_word("set");
        draw_word("drag");
        draw_word("mode");
        draw_dd(sensing_drag_mode_label(opt));
        break;
    default:
        draw_word("sensing");
        break;
    }
}

int sensing_block_hittest_field(TTF_Font *font, SensingBlockType type, int x, int y, int opt, int px, int py)
{
    (void)opt;
    SDL_Rect br = sensing_block_rect(type, x, y);
    if (!(px >= br.x && px < br.x + br.w && py >= br.y && py < br.y + br.h))
        return -1;
    bool is_hex = (type == SENSB_TOUCHING || type == SENSB_KEY_PRESSED || type == SENSB_MOUSE_DOWN);
    int padding_x = is_hex ? (br.h / 2 + 6) : 12;
    int cap_h = 22;
    int cap_y = br.y + (br.h - cap_h) / 2;
    int cur_x = br.x + padding_x;
    auto adv_word = [&](const char *w)
    { cur_x += text_w(font, w) + 6; };
    auto cap_rect = [&](int w)
    { SDL_Rect rc{cur_x, cap_y, w, cap_h}; cur_x += w + 6; return rc; };
    auto in = [&](const SDL_Rect &rc)
    { return px >= rc.x && px < rc.x + rc.w && py >= rc.y && py < rc.y + rc.h; };
    auto dd_w = [&](const char *txt)
    { int tw = text_w(font, txt); return std::max(90, tw + 26); };

    switch (type)
    {
    case SENSB_TOUCHING:
        adv_word("touching");
        if (in(cap_rect(dd_w(sensing_touching_label(opt)))))
            return -2;
        return -1;
    case SENSB_ASK_AND_WAIT:
        adv_word("ask");
        if (in(cap_rect(180)))
            return 0;
        return -1;
    case SENSB_KEY_PRESSED:
        adv_word("key");
        if (in(cap_rect(dd_w(sensing_key_label(opt)))))
            return -2;
        return -1;
    case SENSB_MOUSE_DOWN:
        return -1;
    case SENSB_SET_DRAG_MODE:
        adv_word("set");
        adv_word("drag");
        adv_word("mode");
        if (in(cap_rect(dd_w(sensing_drag_mode_label(opt)))))
            return -2;
        return -1;
    default:
        return -1;
    }
}

static const char *sensing_drag_label(int opt) { return (opt == 0) ? "draggable" : "not draggable"; }

static void draw_boolean_shape(SDL_Renderer *r, const SDL_Rect &br, Color col, Color panel_bg, bool ghost)
{
    (void)panel_bg;
    float gk = ghost ? 0.75f : 1.0f;
    Color border = shade(col, 0.80f * gk);
    Color fill = shade(col, 1.00f * gk);
    int arrow = br.h / 2;

    SDL_Rect mid_b = {br.x + arrow, br.y, br.w - 2 * arrow, br.h};
    SDL_SetRenderDrawColor(r, border.r, border.g, border.b, 255);
    SDL_RenderFillRect(r, &mid_b);

    int cx_l = br.x + arrow;
    int cy_m = br.y + br.h / 2;
    for (int row = 0; row < br.h; ++row)
    {
        int y = br.y + row;
        int half = (row <= br.h / 2) ? row : (br.h - 1 - row);
        int x0 = cx_l - half;
        SDL_RenderDrawLine(r, x0, y, cx_l, y);
    }
    int cx_r = br.x + br.w - arrow;
    for (int row = 0; row < br.h; ++row)
    {
        int y = br.y + row;
        int half = (row <= br.h / 2) ? row : (br.h - 1 - row);
        int x1 = cx_r + half;
        SDL_RenderDrawLine(r, cx_r, y, x1, y);
    }

    SDL_Rect mid_f = {br.x + arrow + 1, br.y + 1, br.w - 2 * arrow - 2, br.h - 2};
    SDL_SetRenderDrawColor(r, fill.r, fill.g, fill.b, 255);
    SDL_RenderFillRect(r, &mid_f);

    for (int row = 1; row < br.h - 1; ++row)
    {
        int y = br.y + row;
        int half = (row <= br.h / 2) ? row : (br.h - 1 - row);
        int x0 = cx_l - half + 1;
        SDL_SetRenderDrawColor(r, fill.r, fill.g, fill.b, 255);
        SDL_RenderDrawLine(r, x0, y, cx_l, y);
    }
    for (int row = 1; row < br.h - 1; ++row)
    {
        int y = br.y + row;
        int half = (row <= br.h / 2) ? row : (br.h - 1 - row);
        int x1 = cx_r + half - 1;
        SDL_SetRenderDrawColor(r, fill.r, fill.g, fill.b, 255);
        SDL_RenderDrawLine(r, cx_r, y, x1, y);
    }
}

int sensing_boolean_block_width(SensingBlockType type)
{
    switch (type)
    {
    case SENSB_TOUCHING: return 280;
    case SENSB_KEY_PRESSED: return 280;
    case SENSB_MOUSE_DOWN: return 240;
    case SENSB_TOUCHING_COLOR: return 180;
    case SENSB_COLOR_IS_TOUCHING_COLOR: return 240;
    default: return 260;
    }
}

SDL_Rect sensing_boolean_block_rect(SensingBlockType type, int x, int y, int opt)
{
    (void)opt;
    return {x, y, sensing_boolean_block_width(type), 36};
}

void sensing_boolean_block_draw(SDL_Renderer *r, TTF_Font *font,
                                SensingBlockType type, int x, int y,
                                int opt, int r1, int g1, int b1, int r2, int g2, int b2,
                                bool ghost, Color panel_bg, int selected_field, const char *override_field0_text)
{
    Color sensing_col = {74, 189, 211};
    SDL_Rect br = sensing_boolean_block_rect(type, x, y, opt);
    draw_boolean_shape(r, br, sensing_col, panel_bg, ghost);

    int arrow = br.h / 2;
    int padding_x = arrow + 8;
    int cy = br.y + (br.h - 16) / 2;
    Color txt_col = {255, 255, 255};
    int cap_h = 20;
    int cap_y = br.y + (br.h - cap_h) / 2;
    int cur_x = br.x + padding_x;

    auto draw_word = [&](const char *w)
    { draw_text(r, font, w, cur_x, cy, txt_col); cur_x += text_w(font, w) + 6; };
    auto draw_dd = [&](const char *txt)
    {
        int tw = text_w(font, txt);
        int cap_w = std::max(90, tw + 26);
        SDL_Rect dd = {cur_x, cap_y, cap_w, cap_h};
        draw_dropdown_capsule(r, dd, (Color){50, 155, 180});
        int tx = dd.x + 10;
        int ty = dd.y + (dd.h - 16) / 2;
        draw_text(r, font, txt, tx, ty, (Color){255, 255, 255});
        draw_caret(r, dd.x + dd.w - 12, dd.y + dd.h / 2, (Color){255, 255, 255});
        cur_x += cap_w + 6;
    };

    auto draw_color_picker = [&](int col_r, int col_g, int col_b) {
        SDL_Rect cap = input_capsule_rect(cur_x, cap_y, 32, cap_h);
        renderer_fill_rounded_rect(r, &cap, cap.h / 2, col_r, col_g, col_b);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, 0, 0, 0, 50);
        SDL_RenderDrawRect(r, &cap);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        cur_x += 38;
    };

    switch (type)
    {
    case SENSB_TOUCHING:
        draw_word("touching"); draw_dd(sensing_touching_label(opt)); draw_word("?");
        break;
    case SENSB_KEY_PRESSED:
        draw_word("key"); draw_dd(sensing_key_label(opt)); draw_word("pressed?");
        break;
    case SENSB_MOUSE_DOWN:
        draw_word("mouse"); draw_word("down?");
        break;
    
    // ---> FIXED: COLOR BLOCK RENDERING <---
    case SENSB_TOUCHING_COLOR:
        draw_word("touching color"); draw_color_picker(r1, g1, b1); draw_word("?");
        break;
    case SENSB_COLOR_IS_TOUCHING_COLOR:
        draw_word("color"); draw_color_picker(r1, g1, b1); 
        draw_word("is touching"); draw_color_picker(r2, g2, b2); draw_word("?");
        break;
    default:
        draw_word("sensing");
        break;
    }
}

int sensing_boolean_block_hittest_field(TTF_Font *font, SensingBlockType type, int x, int y,int opt, int a, int b, int c, int d, int e, int f, int px, int py){
    SDL_Rect br = sensing_boolean_block_rect(type, x, y, opt);
    if (!(px >= br.x && px < br.x + br.w && py >= br.y && py < br.y + br.h))
        return -1;
    int arrow = br.h / 2;
    int padding_x = arrow + 8;
    int cap_h = 20;
    int cap_y = br.y + (br.h - cap_h) / 2;
    int cur_x = br.x + padding_x;
    auto adv_word = [&](const char *w)
    { cur_x += text_w(font, w) + 6; };
    auto cap_rect = [&](int w)
    { SDL_Rect rc = {cur_x, cap_y, w, cap_h}; cur_x += w + 6; return rc; };
    auto in = [&](const SDL_Rect &rc)
    { return px >= rc.x && px < rc.x + rc.w && py >= rc.y && py < rc.y + rc.h; };
    auto dd_w = [&](const char *txt)
    { return std::max(90, text_w(font, txt) + 26); };

    switch (type)
    {
    case SENSB_TOUCHING:
        adv_word("touching");
        if (in(cap_rect(dd_w(sensing_touching_label(opt))))) return -2;
        return -1;
    case SENSB_KEY_PRESSED:
        adv_word("key");
        if (in(cap_rect(dd_w(sensing_key_label(opt))))) return -2;
        return -1;
    case SENSB_MOUSE_DOWN:
        return -1;
    
    // ---> FIXED: COLOR BLOCK HIT ZONES <---
    case SENSB_TOUCHING_COLOR:
        adv_word("touching color");
        if (in(cap_rect(32))) return -5; // Magic number -5 for color picker 1
        return -1;
    case SENSB_COLOR_IS_TOUCHING_COLOR:
        adv_word("color");
        if (in(cap_rect(32))) return -5; // Magic number -5 for color picker 1
        adv_word("is touching");
        if (in(cap_rect(32))) return -6; // Magic number -6 for color picker 2
        return -1;
    default:
        return -1;
    }
}

int sensing_stack_block_width(SensingBlockType type)
{
    switch (type)
    {
    case SENSB_ASK_AND_WAIT:
        return 320;
    case SENSB_SET_DRAG_MODE:
        return 300;
    default:
        return 280;
    }
}

SDL_Rect sensing_stack_block_rect(SensingBlockType type, int x, int y, int opt)
{
    (void)opt;
    MotionBlockMetrics m = motion_block_metrics();
    return {x, y, sensing_stack_block_width(type), m.h};
}

void sensing_stack_block_draw(SDL_Renderer *r, TTF_Font *font, SensingBlockType type, int x, int y,
                              const std::string &text, int opt, bool ghost, Color panel_bg,
                              int selected_field, const char *override_field0_text)
{
    Color sensing_col = {74, 189, 211};
    SDL_Rect br = sensing_stack_block_rect(type, x, y, opt);
    draw_stack_shape(r, br, sensing_col, panel_bg, ghost);

    int padding_x = 12;
    int cy = br.y + (br.h - 16) / 2;
    Color txt_col = {255, 255, 255};
    int cap_h = 22;
    int cap_y = br.y + (br.h - cap_h) / 2;
    int cur_x = br.x + padding_x;

    auto draw_word = [&](const char *w)
    { draw_text(r, font, w, cur_x, cy, txt_col); cur_x += text_w(font, w) + 6; };
    auto draw_text_caps = [&](const char *default_txt, const char *override_txt, int field_index, int cap_w)
    {
        const char *show = default_txt;
        if (override_txt && selected_field == field_index)
            show = override_txt;
        SDL_Rect cap = input_capsule_rect(cur_x, cap_y, cap_w, cap_h);
        draw_input_capsule(r, cap, selected_field == field_index);
        int tw = text_w(font, show);
        int tx = (tw <= cap.w - 10) ? (cap.x + (cap.w - tw) / 2) : (cap.x + 6);
        int ty = cap.y + (cap.h - 16) / 2;
        draw_text(r, font, show, tx, ty, (Color){40, 40, 40});
        cur_x += cap.w + 6;
    };
    auto draw_dd = [&](const char *txt)
    {
        int tw = text_w(font, txt);
        int cap_w = std::max(110, tw + 26);
        SDL_Rect dd = {cur_x, cap_y, cap_w, cap_h};
        draw_dropdown_capsule(r, dd, (Color){50, 155, 180});
        int tx = dd.x + 10;
        int ty = dd.y + (dd.h - 16) / 2;
        draw_text(r, font, txt, tx, ty, (Color){255, 255, 255});
        draw_caret(r, dd.x + dd.w - 12, dd.y + dd.h / 2, (Color){255, 255, 255});
        cur_x += cap_w + 6;
    };

    switch (type)
    {
    case SENSB_ASK_AND_WAIT:
        draw_word("ask");
        draw_text_caps(text.c_str(), override_field0_text, 0, 140);
        draw_word("and");
        draw_word("wait");
        break;
    case SENSB_SET_DRAG_MODE:
        draw_word("set");
        draw_word("drag");
        draw_word("mode");
        draw_dd(sensing_drag_label(opt));
        break;
    default:
        draw_word("sensing");
        break;
    }
}

int sensing_stack_block_hittest_field(TTF_Font *font, SensingBlockType type, int x, int y,
                                      const std::string &text, int opt, int px, int py)
{
    SDL_Rect br = sensing_stack_block_rect(type, x, y, opt);
    if (!(px >= br.x && px < br.x + br.w && py >= br.y && py < br.y + br.h))
        return -1;
    int padding_x = 12;
    int cap_h = 22;
    int cap_y = br.y + (br.h - cap_h) / 2;
    int cur_x = br.x + padding_x;
    auto adv_word = [&](const char *w)
    { cur_x += text_w(font, w) + 6; };
    auto cap_rect = [&](int w)
    { SDL_Rect rc = {cur_x, cap_y, w, cap_h}; cur_x += w + 6; return rc; };
    auto in = [&](const SDL_Rect &rc)
    { return px >= rc.x && px < rc.x + rc.w && py >= rc.y && py < rc.y + rc.h; };
    auto dd_w = [&](const char *txt)
    { return std::max(110, text_w(font, txt) + 26); };

    switch (type)
    {
    case SENSB_ASK_AND_WAIT:
        adv_word("ask");
        if (in(cap_rect(140)))
            return 0;
        return -1;
    case SENSB_SET_DRAG_MODE:
        adv_word("set");
        adv_word("drag");
        adv_word("mode");
        if (in(cap_rect(dd_w(sensing_drag_label(opt)))))
            return -2;
        return -1;
    default:
        return -1;
    }
}

/* ================================================================ */
/* ================        C O N T R O L        =================== */
/* ================================================================ */

static void draw_c_shape_native(SDL_Renderer *r, int x, int y, int w, int top_h, int inner_h, int bottom_h, Color col, Color bg, bool ghost, bool has_top_notch, bool has_bottom_notch)
{
    float gk = ghost ? 0.75f : 1.0f;
    Color border = shade(col, 0.80f * gk);
    Color fill = shade(col, 1.00f * gk);
    int spine_w = 16;
    MotionBlockMetrics m = motion_block_metrics();

    SDL_Rect top = {x, y, w, top_h};
    renderer_fill_rounded_rect(r, &top, 4, border.r, border.g, border.b);
    SDL_Rect topF = {x + 1, y + 1, w - 2, top_h - 2};
    renderer_fill_rounded_rect(r, &topF, 4, fill.r, fill.g, fill.b);

    SDL_Rect spine = {x, y + top_h - 4, spine_w, inner_h + 8};
    SDL_SetRenderDrawColor(r, fill.r, fill.g, fill.b, 255);
    SDL_RenderFillRect(r, &spine);
    SDL_SetRenderDrawColor(r, border.r, border.g, border.b, 255);
    SDL_RenderDrawLine(r, x, spine.y, x, spine.y + spine.h);

    SDL_Rect bot = {x, y + top_h + inner_h, w, bottom_h};
    renderer_fill_rounded_rect(r, &bot, 4, border.r, border.g, border.b);
    SDL_Rect botF = {x + 1, y + top_h + inner_h + 1, w - 2, bottom_h - 2};
    renderer_fill_rounded_rect(r, &botF, 4, fill.r, fill.g, fill.b);

    if (has_top_notch)
    {
        SDL_Rect tc = {x + m.notch_x, y, m.notch_w, m.notch_h};
        renderer_fill_rounded_rect(r, &tc, m.notch_h / 2, bg.r, bg.g, bg.b);
    }
    if (has_bottom_notch)
    {
        SDL_Rect bc = {x + m.notch_x, bot.y + bot.h - m.notch_h, m.notch_w, m.notch_h};
        renderer_fill_rounded_rect(r, &bc, m.notch_h / 2, bg.r, bg.g, bg.b);
    }
    SDL_Rect ic = {x + spine_w + m.notch_x, y + top_h - m.notch_h, m.notch_w, m.notch_h};
    renderer_fill_rounded_rect(r, &ic, m.notch_h / 2, fill.r, fill.g, fill.b);
}

int control_block_width(ControlBlockType type)
{
    switch (type)
    {
    case CB_WAIT:
        return 200;
    case CB_REPEAT:
        return 220;
    case CB_FOREVER:
        return 180;
    case CB_IF:
        return 240;
    case CB_IF_ELSE:
        return 240;
    case CB_WAIT_UNTIL:
        return 240;
    default:
        return 200;
    }
}

SDL_Rect control_block_rect(ControlBlockType type, int x, int y, int inner1_h, int inner2_h)
{
    int w = control_block_width(type);
    int h = 40;
    if (type == CB_REPEAT || type == CB_IF)
        h = 40 + std::max(24, inner1_h) + 24;
    else if (type == CB_FOREVER)
        h = 40 + std::max(24, inner1_h) + 20;
    else if (type == CB_IF_ELSE)
        h = 40 + std::max(24, inner1_h) + 32 + std::max(24, inner2_h) + 24;
    return {x, y, w, h};
}

static void draw_hex_slot(SDL_Renderer *r, int cx, int cy, Color base_col, bool filled)
{
    if (filled)
        return;
    int w = 50, h = 24;
    SDL_Rect br = {cx, cy - h / 2, w, h};
    Color hole_col = shade(base_col, 0.70f);
    int arrow = br.h / 2;
    SDL_Rect mid = {br.x + arrow, br.y, br.w - 2 * arrow, br.h};
    SDL_SetRenderDrawColor(r, hole_col.r, hole_col.g, hole_col.b, 255);
    SDL_RenderFillRect(r, &mid);
    int cx_l = br.x + arrow;
    for (int row = 0; row < br.h; ++row)
    {
        int y = br.y + row;
        int half = (row <= br.h / 2) ? row : (br.h - 1 - row);
        int x0 = cx_l - half;
        SDL_RenderDrawLine(r, x0, y, cx_l, y);
    }
    int cx_r = br.x + br.w - arrow;
    for (int row = 0; row < br.h; ++row)
    {
        int y = br.y + row;
        int half = (row <= br.h / 2) ? row : (br.h - 1 - row);
        int x1 = cx_r + half;
        SDL_RenderDrawLine(r, cx_r, y, x1, y);
    }
    SDL_SetRenderDrawColor(r, 0, 0, 0, 40);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_RenderDrawLine(r, mid.x - 5, mid.y, mid.x + mid.w + 5, mid.y);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void control_block_draw(SDL_Renderer *r, TTF_Font *font, ControlBlockType type, int x, int y, int inner1_h, int inner2_h, int a, bool has_condition, bool ghost, Color panel_bg, int selected_field, const char *override_field0_text)
{
    Color col = {255, 171, 25};
    SDL_Rect br = control_block_rect(type, x, y, inner1_h, inner2_h);
    int min_inner = 24;
    int h1 = std::max(min_inner, inner1_h);
    int h2 = std::max(min_inner, inner2_h);

    if (type == CB_WAIT || type == CB_WAIT_UNTIL)
        draw_stack_shape_custom(r, br, col, panel_bg, ghost, true, true);
    else if (type == CB_REPEAT || type == CB_IF)
        draw_c_shape_native(r, x, y, br.w, 40, h1, 24, col, panel_bg, ghost, true, true);
    else if (type == CB_FOREVER)
        draw_c_shape_native(r, x, y, br.w, 40, h1, 20, col, panel_bg, ghost, true, false);
    else if (type == CB_IF_ELSE)
    {
        draw_c_shape_native(r, x, y, br.w, 40, h1, 32, col, panel_bg, ghost, true, false);
        draw_c_shape_native(r, x, y + 40 + h1, br.w, 32, h2, 24, col, panel_bg, ghost, false, true);
    }

    int cur_x = br.x + 12;
    int cy = br.y + (40 - 16) / 2;
    Color txt_col = {255, 255, 255};
    char bufA[32];
    std::snprintf(bufA, sizeof(bufA), "%d", a);

    auto draw_word = [&](const char *w, int y_pos)
    { draw_text(r, font, w, cur_x, y_pos, txt_col); cur_x += text_w(font, w) + 6; };
    auto draw_num = [&](const char *txt)
    {
        SDL_Rect cap = input_capsule_rect(cur_x, br.y + 9, 40, 22);
        draw_input_capsule(r, cap, selected_field == 0);
        draw_text(r, font, override_field0_text ? override_field0_text : txt, cap.x + 6, cap.y + 3, {40, 40, 40});
        cur_x += 46;
    };

    if (type == CB_WAIT)
    {
        draw_word("wait", cy);
        draw_num(bufA);
        draw_word("seconds", cy);
    }
    else if (type == CB_REPEAT)
    {
        draw_word("repeat", cy);
        draw_num(bufA);
    }
    else if (type == CB_FOREVER)
    {
        draw_word("forever", cy);
    }
    else if (type == CB_IF)
    {
        draw_word("if", cy);
        draw_hex_slot(r, cur_x, cy + 8, col, has_condition);
        cur_x += 56;
        draw_word("then", cy);
    }
    else if (type == CB_IF_ELSE)
    {
        draw_word("if", cy);
        draw_hex_slot(r, cur_x, cy + 8, col, has_condition);
        cur_x += 56;
        draw_word("then", cy);
        cur_x = br.x + 12;
        draw_word("else", br.y + 40 + h1 + 8);
    }
    else if (type == CB_WAIT_UNTIL)
    {
        draw_word("wait", cy);
        draw_word("until", cy);
        draw_hex_slot(r, cur_x, cy + 8, col, has_condition);
    }
}

int control_block_hittest_field(TTF_Font *font, ControlBlockType type, int x, int y, int inner1_h, int inner2_h, int a, int px, int py)
{
    SDL_Rect br = control_block_rect(type, x, y, inner1_h, inner2_h);
    if (!(px >= br.x && px < br.x + br.w && py >= br.y && py < br.y + br.h))
        return -1;
    int padding_x = 12;
    int cur_x = br.x + padding_x;

    if (type == CB_WAIT || type == CB_REPEAT)
    {
        cur_x += text_w(font, type == CB_WAIT ? "wait" : "repeat") + 6;
        if (px >= cur_x && px < cur_x + 40 && py >= br.y + 9 && py < br.y + 31)
            return 0;
    }
    else if (type == CB_IF || type == CB_IF_ELSE || type == CB_WAIT_UNTIL)
    {
        cur_x += text_w(font, type == CB_WAIT_UNTIL ? "wait until" : "if") + 12;
        if (px >= cur_x && px < cur_x + 50 && py >= br.y + 8 && py < br.y + 32)
            return -3;
    }
    return -1;
}

/* ================================================================ */
/* ===============       O P E R A T O R S      =================== */
/* ================================================================ */

static void draw_reporter_shape(SDL_Renderer *r, const SDL_Rect &br, Color col, bool ghost)
{
    float gk = ghost ? 0.75f : 1.0f;
    Color border = shade(col, 0.80f * gk);
    Color fill = shade(col, 1.00f * gk);
    renderer_fill_rounded_rect(r, &br, br.h / 2, border.r, border.g, border.b);
    SDL_Rect inner{br.x + 1, br.y + 1, br.w - 2, br.h - 2};
    renderer_fill_rounded_rect(r, &inner, std::max(0, inner.h / 2), fill.r, fill.g, fill.b);
}

int operators_block_width(OperatorsBlockType type)
{
    switch (type)
    {
    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
        return 140;
    case OP_GT:
    case OP_LT:
    case OP_EQ:
        return 160;
    case OP_AND:
    case OP_OR:
        return 190;
    case OP_NOT:
        return 140;
    case OP_JOIN:
        return 190;
    case OP_LETTER_OF:
        return 200;
    case OP_LENGTH_OF:
        return 160;
    default:
        return 140;
    }
}

SDL_Rect operators_block_rect(OperatorsBlockType type, int x, int y)
{
    return {x, y, operators_block_width(type), 40};
}

void operators_block_draw(SDL_Renderer *r, TTF_Font *font, OperatorsBlockType type, int x, int y, const std::string &str_a, const std::string &str_b, int a, int b, bool ghost, Color panel_bg, int selected_field, const char *override_field0_text, const char *override_field1_text)
{
    (void)a;
    (void)b;
    Color op_col = {89, 192, 89};
    SDL_Rect br = operators_block_rect(type, x, y);
    bool is_bool = (type == OP_GT || type == OP_LT || type == OP_EQ || type == OP_AND || type == OP_OR || type == OP_NOT);
    if (is_bool)
        draw_boolean_shape(r, br, op_col, panel_bg, ghost);
    else
        draw_reporter_shape(r, br, op_col, ghost);

    int cy = br.y + (br.h - 16) / 2;
    int cap_h = 24;
    int cap_y = br.y + (br.h - cap_h) / 2;

    auto get_word_w = [&](const char *w)
    { return text_w(font, w) + 6; };
    auto get_cap_w = [&](int cw)
    { return cw + 6; };
    auto get_hex_w = [&]()
    { return 50 + 6; };

    int content_w = 0;
    switch (type)
    {
    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
        content_w = get_cap_w(36) + get_word_w("+") + get_cap_w(36);
        break;
    case OP_GT:
    case OP_LT:
    case OP_EQ:
        content_w = get_cap_w(36) + get_word_w("=") + get_cap_w(36);
        break;
    case OP_AND:
    case OP_OR:
        content_w = get_hex_w() + get_word_w("and") + get_hex_w();
        break;
    case OP_NOT:
        content_w = get_word_w("not") + get_hex_w();
        break;
    case OP_JOIN:
        content_w = get_word_w("join") + get_cap_w(46) + get_cap_w(56);
        break;
    case OP_LETTER_OF:
        content_w = get_word_w("letter") + get_cap_w(26) + get_word_w("of") + get_cap_w(56);
        break;
    case OP_LENGTH_OF:
        content_w = get_word_w("length of") + get_cap_w(56);
        break;
    }
    if (content_w > 0)
        content_w -= 6;

    int cur_x = br.x + (br.w - content_w) / 2;
    Color txt_col = {255, 255, 255};

    auto draw_word = [&](const char *w)
    { draw_text(r, font, w, cur_x, cy, txt_col); cur_x += text_w(font, w) + 6; };
    auto draw_capsule = [&](const char *txt, const char *override_txt, int field_index, int cw)
    {
        const char *show = (override_txt && selected_field == field_index) ? override_txt : txt;
        SDL_Rect cap = input_capsule_rect(cur_x, cap_y, cw, cap_h);
        draw_input_capsule(r, cap, selected_field == field_index);
        int tw = text_w(font, show);
        int tx = (tw <= cap.w - 10) ? (cap.x + (cap.w - tw) / 2) : (cap.x + 6);
        int ty = cap.y + (cap.h - 16) / 2;
        draw_text(r, font, show, tx, ty, (Color){40, 40, 40});
        cur_x += cap.w + 6;
    };
    auto draw_empty_hex = [&]()
    { draw_hex_slot(r, cur_x, cy + 8, op_col, false); cur_x += 50 + 6; };

    switch (type)
    {
    case OP_ADD:
        draw_capsule(str_a.c_str(), override_field0_text, 0, 36);
        draw_word("+");
        draw_capsule(str_b.c_str(), override_field1_text, 1, 36);
        break;
    case OP_SUB:
        draw_capsule(str_a.c_str(), override_field0_text, 0, 36);
        draw_word("-");
        draw_capsule(str_b.c_str(), override_field1_text, 1, 36);
        break;
    case OP_MUL:
        draw_capsule(str_a.c_str(), override_field0_text, 0, 36);
        draw_word("*");
        draw_capsule(str_b.c_str(), override_field1_text, 1, 36);
        break;
    case OP_DIV:
        draw_capsule(str_a.c_str(), override_field0_text, 0, 36);
        draw_word("/");
        draw_capsule(str_b.c_str(), override_field1_text, 1, 36);
        break;
    case OP_GT:
        draw_capsule(str_a.c_str(), override_field0_text, 0, 36);
        draw_word(">");
        draw_capsule(str_b.c_str(), override_field1_text, 1, 36);
        break;
    case OP_LT:
        draw_capsule(str_a.c_str(), override_field0_text, 0, 36);
        draw_word("<");
        draw_capsule(str_b.c_str(), override_field1_text, 1, 36);
        break;
    case OP_EQ:
        draw_capsule(str_a.c_str(), override_field0_text, 0, 36);
        draw_word("=");
        draw_capsule(str_b.c_str(), override_field1_text, 1, 36);
        break;
    case OP_AND:
        draw_empty_hex();
        draw_word("and");
        draw_empty_hex();
        break;
    case OP_OR:
        draw_empty_hex();
        draw_word("or");
        draw_empty_hex();
        break;
    case OP_NOT:
        draw_word("not");
        draw_empty_hex();
        break;
    case OP_JOIN:
        draw_word("join");
        draw_capsule(str_a.c_str(), override_field0_text, 0, 46);
        draw_capsule(str_b.c_str(), override_field1_text, 1, 56);
        break;
    case OP_LETTER_OF:
        draw_word("letter");
        draw_capsule(str_a.c_str(), override_field0_text, 0, 26);
        draw_word("of");
        draw_capsule(str_b.c_str(), override_field1_text, 1, 56);
        break;
    case OP_LENGTH_OF:
        draw_word("length of");
        draw_capsule(str_a.c_str(), override_field0_text, 0, 56);
        break;
    }
}

int operators_block_hittest_field(TTF_Font *font, OperatorsBlockType type, int x, int y, int px, int py)
{
    SDL_Rect br = operators_block_rect(type, x, y);
    if (!(px >= br.x && px < br.x + br.w && py >= br.y && py < br.y + br.h))
        return -1;
    int cap_h = 24;
    int cap_y = br.y + (br.h - cap_h) / 2;
    auto get_word_w = [&](const char *w)
    { return text_w(font, w) + 6; };
    auto get_cap_w = [&](int cw)
    { return cw + 6; };
    auto get_hex_w = [&]()
    { return 50 + 6; };
    int content_w = 0;
    switch (type)
    {
    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
        content_w = get_cap_w(36) + get_word_w("+") + get_cap_w(36);
        break;
    case OP_GT:
    case OP_LT:
    case OP_EQ:
        content_w = get_cap_w(36) + get_word_w("=") + get_cap_w(36);
        break;
    case OP_AND:
    case OP_OR:
        content_w = get_hex_w() + get_word_w("and") + get_hex_w();
        break;
    case OP_NOT:
        content_w = get_word_w("not") + get_hex_w();
        break;
    case OP_JOIN:
        content_w = get_word_w("join") + get_cap_w(46) + get_cap_w(56);
        break;
    case OP_LETTER_OF:
        content_w = get_word_w("letter") + get_cap_w(26) + get_word_w("of") + get_cap_w(56);
        break;
    case OP_LENGTH_OF:
        content_w = get_word_w("length of") + get_cap_w(56);
        break;
    }
    if (content_w > 0)
        content_w -= 6;
    int cur_x = br.x + (br.w - content_w) / 2;
    auto adv_word = [&](const char *w)
    { cur_x += text_w(font, w) + 6; };
    auto cap_rect = [&](int cw)
    { SDL_Rect rc{cur_x, cap_y, cw, cap_h}; cur_x += cw + 6; return rc; };
    auto in = [&](const SDL_Rect &rc)
    { return px >= rc.x && px < rc.x + rc.w && py >= rc.y && py < rc.y + rc.h; };

    switch (type)
    {
    case OP_ADD:
        if (in(cap_rect(36)))
            return 0;
        adv_word("+");
        if (in(cap_rect(36)))
            return 1;
        return -1;
    case OP_SUB:
        if (in(cap_rect(36)))
            return 0;
        adv_word("-");
        if (in(cap_rect(36)))
            return 1;
        return -1;
    case OP_MUL:
        if (in(cap_rect(36)))
            return 0;
        adv_word("*");
        if (in(cap_rect(36)))
            return 1;
        return -1;
    case OP_DIV:
        if (in(cap_rect(36)))
            return 0;
        adv_word("/");
        if (in(cap_rect(36)))
            return 1;
        return -1;
    case OP_GT:
        if (in(cap_rect(36)))
            return 0;
        adv_word(">");
        if (in(cap_rect(36)))
            return 1;
        return -1;
    case OP_LT:
        if (in(cap_rect(36)))
            return 0;
        adv_word("<");
        if (in(cap_rect(36)))
            return 1;
        return -1;
    case OP_EQ:
        if (in(cap_rect(36)))
            return 0;
        adv_word("=");
        if (in(cap_rect(36)))
            return 1;
        return -1;
    case OP_AND:
    case OP_OR:
    case OP_NOT:
        return -1;
    case OP_JOIN:
        adv_word("join");
        if (in(cap_rect(46)))
            return 0;
        if (in(cap_rect(56)))
            return 1;
        return -1;
    case OP_LETTER_OF:
        adv_word("letter");
        if (in(cap_rect(26)))
            return 0;
        adv_word("of");
        if (in(cap_rect(56)))
            return 1;
        return -1;
    case OP_LENGTH_OF:
        adv_word("length of");
        if (in(cap_rect(56)))
            return 0;
        return -1;
    default:
        return -1;
    }
}
void operators_block_draw_dynamic(SDL_Renderer *r, TTF_Font *font, OperatorsBlockType type, int x, int y, int total_w, const std::string &str_a, const std::string &str_b, int cap0_w, int cap1_w, bool ghost, Color panel_bg, int selected_field, const char *override_field0_text, const char *override_field1_text)
{
    Color op_col = {89, 192, 89};
    SDL_Rect br = {x, y, total_w, 40};
    bool is_bool = (type == OP_GT || type == OP_LT || type == OP_EQ || type == OP_AND || type == OP_OR || type == OP_NOT);
    if (is_bool)
        draw_boolean_shape(r, br, op_col, panel_bg, ghost);
    else
        draw_reporter_shape(r, br, op_col, ghost);

    int cy = br.y + (br.h - 16) / 2;
    int cap_h = 24;
    int cap_y = br.y + (br.h - cap_h) / 2;
    auto get_word_w = [&](const char *w)
    { return text_w(font, w) + 6; };
    auto get_cw = [&](int cw)
    { return cw + 6; };
    int content_w = 0;
    switch (type)
    {
    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
        content_w = get_cw(cap0_w) + get_word_w(type == OP_ADD ? "+" : type == OP_SUB ? "-"
                                                                   : type == OP_MUL   ? "*"
                                                                                      : "/") +
                    get_cw(cap1_w);
        break;
    case OP_GT:
    case OP_LT:
    case OP_EQ:
        content_w = get_cw(cap0_w) + get_word_w(type == OP_GT ? ">" : type == OP_LT ? "<"
                                                                                    : "=") +
                    get_cw(cap1_w);
        break;
    case OP_AND:
    case OP_OR:
        content_w = get_cw(cap0_w) + get_word_w(type == OP_AND ? "and" : "or") + get_cw(cap1_w);
        break;
    case OP_NOT:
        content_w = get_word_w("not") + get_cw(cap0_w);
        break;
    case OP_JOIN:
        content_w = get_word_w("join") + get_cw(cap0_w) + get_cw(cap1_w);
        break;
    case OP_LETTER_OF:
        content_w = get_word_w("letter") + get_cw(cap0_w) + get_word_w("of") + get_cw(cap1_w);
        break;
    case OP_LENGTH_OF:
        content_w = get_word_w("length of") + get_cw(cap0_w);
        break;
    }
    if (content_w > 0)
        content_w -= 6;
    int cur_x = br.x + (br.w - content_w) / 2;
    Color txt_col = {255, 255, 255};

    auto draw_word = [&](const char *w)
    { draw_text(r, font, w, cur_x, cy, txt_col); cur_x += text_w(font, w) + 6; };
    auto draw_capsule = [&](const char *txt, const char *override_txt, int field_index, int cw, bool is_hex)
    {
        const char *show = (override_txt && selected_field == field_index) ? override_txt : txt;
        SDL_Rect cap = input_capsule_rect(cur_x, cap_y, cw, cap_h);
        bool filled = (override_txt != nullptr && override_txt[0] == '\0' && selected_field != field_index);
        if (is_hex)
        {
            if (!filled)
                draw_hex_slot(r, cur_x, cy + 8, op_col, false);
        }
        else
        {
            draw_input_capsule(r, cap, selected_field == field_index);
            if (!filled)
            {
                int tw = text_w(font, show);
                int tx = (tw <= cap.w - 10) ? (cap.x + (cap.w - tw) / 2) : (cap.x + 6);
                int ty = cap.y + (cap.h - 16) / 2;
                draw_text(r, font, show, tx, ty, (Color){40, 40, 40});
            }
        }
        cur_x += cw + 6;
    };

    switch (type)
    {
    case OP_ADD:
        draw_capsule(str_a.c_str(), override_field0_text, 0, cap0_w, false);
        draw_word("+");
        draw_capsule(str_b.c_str(), override_field1_text, 1, cap1_w, false);
        break;
    case OP_SUB:
        draw_capsule(str_a.c_str(), override_field0_text, 0, cap0_w, false);
        draw_word("-");
        draw_capsule(str_b.c_str(), override_field1_text, 1, cap1_w, false);
        break;
    case OP_MUL:
        draw_capsule(str_a.c_str(), override_field0_text, 0, cap0_w, false);
        draw_word("*");
        draw_capsule(str_b.c_str(), override_field1_text, 1, cap1_w, false);
        break;
    case OP_DIV:
        draw_capsule(str_a.c_str(), override_field0_text, 0, cap0_w, false);
        draw_word("/");
        draw_capsule(str_b.c_str(), override_field1_text, 1, cap1_w, false);
        break;
    case OP_GT:
        draw_capsule(str_a.c_str(), override_field0_text, 0, cap0_w, false);
        draw_word(">");
        draw_capsule(str_b.c_str(), override_field1_text, 1, cap1_w, false);
        break;
    case OP_LT:
        draw_capsule(str_a.c_str(), override_field0_text, 0, cap0_w, false);
        draw_word("<");
        draw_capsule(str_b.c_str(), override_field1_text, 1, cap1_w, false);
        break;
    case OP_EQ:
        draw_capsule(str_a.c_str(), override_field0_text, 0, cap0_w, false);
        draw_word("=");
        draw_capsule(str_b.c_str(), override_field1_text, 1, cap1_w, false);
        break;
    case OP_AND:
        draw_capsule("", override_field0_text, 0, cap0_w, true);
        draw_word("and");
        draw_capsule("", override_field1_text, 1, cap1_w, true);
        break;
    case OP_OR:
        draw_capsule("", override_field0_text, 0, cap0_w, true);
        draw_word("or");
        draw_capsule("", override_field1_text, 1, cap1_w, true);
        break;
    case OP_NOT:
        draw_word("not");
        draw_capsule("", override_field0_text, 0, cap0_w, true);
        break;
    case OP_JOIN:
        draw_word("join");
        draw_capsule(str_a.c_str(), override_field0_text, 0, cap0_w, false);
        draw_capsule(str_b.c_str(), override_field1_text, 1, cap1_w, false);
        break;
    case OP_LETTER_OF:
        draw_word("letter");
        draw_capsule(str_a.c_str(), override_field0_text, 0, cap0_w, false);
        draw_word("of");
        draw_capsule(str_b.c_str(), override_field1_text, 1, cap1_w, false);
        break;
    case OP_LENGTH_OF:
        draw_word("length of");
        draw_capsule(str_a.c_str(), override_field0_text, 0, cap0_w, false);
        break;
    }
}

int operators_block_hittest_dynamic(TTF_Font *font, OperatorsBlockType type, int x, int y, int total_w, int cap0_w, int cap1_w, int px, int py)
{
    SDL_Rect br = {x, y, total_w, 40};
    if (!(px >= br.x && px < br.x + br.w && py >= br.y && py < br.y + br.h))
        return -1;
    int cap_h = 24;
    int cap_y = br.y + (br.h - cap_h) / 2;
    auto get_word_w = [&](const char *w)
    { return text_w(font, w) + 6; };
    auto get_cw = [&](int cw)
    { return cw + 6; };
    int content_w = 0;
    switch (type)
    {
    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
        content_w = get_cw(cap0_w) + get_word_w(type == OP_ADD ? "+" : type == OP_SUB ? "-"
                                                                   : type == OP_MUL   ? "*"
                                                                                      : "/") +
                    get_cw(cap1_w);
        break;
    case OP_GT:
    case OP_LT:
    case OP_EQ:
        content_w = get_cw(cap0_w) + get_word_w(type == OP_GT ? ">" : type == OP_LT ? "<"
                                                                                    : "=") +
                    get_cw(cap1_w);
        break;
    case OP_AND:
    case OP_OR:
        content_w = get_cw(cap0_w) + get_word_w(type == OP_AND ? "and" : "or") + get_cw(cap1_w);
        break;
    case OP_NOT:
        content_w = get_word_w("not") + get_cw(cap0_w);
        break;
    case OP_JOIN:
        content_w = get_word_w("join") + get_cw(cap0_w) + get_cw(cap1_w);
        break;
    case OP_LETTER_OF:
        content_w = get_word_w("letter") + get_cw(cap0_w) + get_word_w("of") + get_cw(cap1_w);
        break;
    case OP_LENGTH_OF:
        content_w = get_word_w("length of") + get_cw(cap0_w);
        break;
    }
    if (content_w > 0)
        content_w -= 6;
    int cur_x = br.x + (br.w - content_w) / 2;
    auto adv_word = [&](const char *w)
    { cur_x += text_w(font, w) + 6; };
    auto cap_rect = [&](int cw)
    { SDL_Rect rc{cur_x, cap_y, cw, cap_h}; cur_x += cw + 6; return rc; };
    auto in = [&](const SDL_Rect &rc)
    { return px >= rc.x && px < rc.x + rc.w && py >= rc.y && py < rc.y + rc.h; };

    switch (type)
    {
    case OP_ADD:
        if (in(cap_rect(cap0_w)))
            return 0;
        adv_word("+");
        if (in(cap_rect(cap1_w)))
            return 1;
        return -1;
    case OP_SUB:
        if (in(cap_rect(cap0_w)))
            return 0;
        adv_word("-");
        if (in(cap_rect(cap1_w)))
            return 1;
        return -1;
    case OP_MUL:
        if (in(cap_rect(cap0_w)))
            return 0;
        adv_word("*");
        if (in(cap_rect(cap1_w)))
            return 1;
        return -1;
    case OP_DIV:
        if (in(cap_rect(cap0_w)))
            return 0;
        adv_word("/");
        if (in(cap_rect(cap1_w)))
            return 1;
        return -1;
    case OP_GT:
        if (in(cap_rect(cap0_w)))
            return 0;
        adv_word(">");
        if (in(cap_rect(cap1_w)))
            return 1;
        return -1;
    case OP_LT:
        if (in(cap_rect(cap0_w)))
            return 0;
        adv_word("<");
        if (in(cap_rect(cap1_w)))
            return 1;
        return -1;
    case OP_EQ:
        if (in(cap_rect(cap0_w)))
            return 0;
        adv_word("=");
        if (in(cap_rect(cap1_w)))
            return 1;
        return -1;
    case OP_AND:
    case OP_OR:
    case OP_NOT:
        return -1;
    case OP_JOIN:
        adv_word("join");
        if (in(cap_rect(cap0_w)))
            return 0;
        if (in(cap_rect(cap1_w)))
            return 1;
        return -1;
    case OP_LETTER_OF:
        adv_word("letter");
        if (in(cap_rect(cap0_w)))
            return 0;
        adv_word("of");
        if (in(cap_rect(cap1_w)))
            return 1;
        return -1;
    case OP_LENGTH_OF:
        adv_word("length of");
        if (in(cap_rect(cap0_w)))
            return 0;
        return -1;
    default:
        return -1;
    }
}

/* ================================================================ */
/* ===============       V A R I A B L E S      =================== */
/* ================================================================ */

SDL_Rect variables_block_rect(VariablesBlockType type, int x, int y, const std::string &var_name)
{
    if (type == VB_VARIABLE)
        return {x, y, std::max(60, (int)var_name.length() * 8 + 24), 40};
    if (type == VB_SET || type == VB_CHANGE)
        return {x, y, 240, 40};
    return {x, y, 150, 40};
}

void variables_block_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state, VariablesBlockType type, int x, int y, const std::string &var_name, const std::string &input_text, int input_num, int opt, bool ghost, int selected_field, const char *override_field0_text)
{
    Color c = {255, 140, 26};
    SDL_Rect br = variables_block_rect(type, x, y, var_name);
    Color panel_bg = (x < 300) ? (Color){249, 249, 249} : (Color){200, 200, 200};

    if (type == VB_VARIABLE)
    {
        draw_reporter_shape(r, br, c, ghost);
        int tw = text_w(font, var_name.c_str());
        draw_text(r, font, var_name.c_str(), br.x + (br.w - tw) / 2, br.y + 12, {255, 255, 255});
    }
    else if (type == VB_SET || type == VB_CHANGE)
    {
        draw_stack_shape(r, br, c, panel_bg, ghost);
        int cx = br.x + 16;
        const char *verb = (type == VB_SET) ? "set" : "change";
        draw_text(r, font, verb, cx, br.y + 12, {255, 255, 255});
        cx += text_w(font, verb) + 8;
        std::string sel_var = (opt >= 0 && opt < (int)state.variables.size()) ? state.variables[opt] : "";
        SDL_Rect drop_r = {cx, br.y + 8, std::max(60, text_w(font, sel_var.c_str()) + 24), 24};
        renderer_fill_rounded_rect(r, &drop_r, 12, 220, 100, 10);
        draw_text(r, font, sel_var.c_str(), cx + 8, br.y + 12, {255, 255, 255});
        draw_caret(r, cx + drop_r.w - 12, br.y + 20, {255, 255, 255});
        cx += drop_r.w + 8;
        const char *prep = (type == VB_SET) ? "to" : "by";
        draw_text(r, font, prep, cx, br.y + 12, {255, 255, 255});
        cx += text_w(font, prep) + 8;
        SDL_Rect cap = input_capsule_rect(cx, br.y + 8, 46, 24);
        draw_input_capsule(r, cap, selected_field == 0);
        std::string disp = override_field0_text && selected_field == 0 ? override_field0_text : (type == VB_SET ? input_text : std::to_string(input_num));
        int dw = text_w(font, disp.c_str());
        int dx = (dw <= cap.w - 10) ? (cap.x + (cap.w - dw) / 2) : (cap.x + 6);
        draw_text(r, font, disp.c_str(), dx, cap.y + 4, {40, 40, 40});
    }
}

int variables_block_hittest_field(TTF_Font *font, const AppState &state, VariablesBlockType type, int x, int y, const std::string &var_name, int opt, int px, int py)
{
    SDL_Rect br = variables_block_rect(type, x, y, var_name);
    if (!(px >= br.x && px < br.x + br.w && py >= br.y && py < br.y + br.h))
        return -1;
    if (type == VB_SET || type == VB_CHANGE)
    {
        int cx = br.x + 16;
        cx += text_w(font, (type == VB_SET) ? "set" : "change") + 8;
        std::string sel_var = (opt >= 0 && opt < (int)state.variables.size()) ? state.variables[opt] : "";
        SDL_Rect drop_r = {cx, br.y + 8, std::max(60, text_w(font, sel_var.c_str()) + 24), 24};
        if (px >= drop_r.x && px < drop_r.x + drop_r.w && py >= drop_r.y && py < drop_r.y + drop_r.h)
            return -2;
        cx += drop_r.w + 8;
        cx += text_w(font, (type == VB_SET) ? "to" : "by") + 8;
        SDL_Rect cap = {cx, br.y + 8, 46, 24};
        if (px >= cap.x && px < cap.x + cap.w && py >= cap.y && py < cap.y + cap.h)
            return 0;
    }
    return -1;
}

SDL_Rect sensing_reporter_block_rect(SensingBlockType type, int x, int y)
{
    if (type == SENSB_ANSWER)
        return {x, y, 80, 40};
    if (type == SENSB_DISTANCE_TO)
        return {x, y, 220, 40};
    if (type == SENSB_MOUSE_X || type == SENSB_MOUSE_Y)
        return {x, y, 100, 40}; // ---> FIXED: Proper width for Mouse X/Y
    return {x, y, 100, 40};
}

void sensing_reporter_block_draw(SDL_Renderer *r, TTF_Font *font, SensingBlockType type, int x, int y, bool ghost)
{
    Color c = {90, 188, 216};
    SDL_Rect br = sensing_reporter_block_rect(type, x, y);
    draw_reporter_shape(r, br, c, ghost);
    
    if (type == SENSB_ANSWER) {
        draw_text(r, font, "answer", br.x + 16, br.y + 12, {255, 255, 255});
    }
    else if (type == SENSB_DISTANCE_TO) {
        draw_text(r, font, "distance to", br.x + 16, br.y + 12, {255, 255, 255});
        int cx = br.x + 16 + text_w(font, "distance to") + 8;
        SDL_Rect drop_r = {cx, br.y + 8, 100, 24};
        draw_dropdown_capsule(r, drop_r, {70, 150, 175});
        draw_text(r, font, "mouse-pointer", drop_r.x + 6, drop_r.y + 4, {255, 255, 255});
        draw_caret(r, drop_r.x + drop_r.w - 12, drop_r.y + 12, {255, 255, 255});
    }
    // ---> FIXED: Draw text for Mouse X and Mouse Y <---
    else if (type == SENSB_MOUSE_X) {
        draw_text(r, font, "mouse x", br.x + 16, br.y + 12, {255, 255, 255});
    }
    else if (type == SENSB_MOUSE_Y) {
        draw_text(r, font, "mouse y", br.x + 16, br.y + 12, {255, 255, 255});
    }
}

int sensing_reporter_block_hittest_field(TTF_Font *font, SensingBlockType type, int x, int y, int px, int py)
{
    SDL_Rect br = sensing_reporter_block_rect(type, x, y);
    if (!(px >= br.x && px < br.x + br.w && py >= br.y && py < br.y + br.h))
        return -1;
    if (type == SENSB_DISTANCE_TO)
    {
        int cx = br.x + 16 + text_w(font, "distance to") + 8;
        SDL_Rect drop_r = {cx, br.y + 8, 100, 24};
        if (px >= drop_r.x && px < drop_r.x + drop_r.w && py >= drop_r.y && py < drop_r.y + drop_r.h)
            return -2;
    }
    return -1;
}

/* ================================================================ */
/* ==================       P E N       =========================== */
/* ================================================================ */

int pen_block_width(PenBlockType type)
{
    switch (type)
    {
    case PB_ERASE_ALL:
        return 160;
    case PB_STAMP:
        return 140;
    case PB_PEN_DOWN:
        return 160;
    case PB_PEN_UP:
        return 140;
    case PB_SET_COLOR_TO_PICKER:
        return 240;
    case PB_CHANGE_ATTRIB_BY:
        return 260;
    case PB_SET_ATTRIB_TO:
        return 240;
    case PB_CHANGE_SIZE_BY:
        return 260;
    case PB_SET_SIZE_TO:
        return 240;
    default:
        return 160;
    }
}

SDL_Rect pen_block_rect(PenBlockType type, int x, int y)
{
    MotionBlockMetrics m = motion_block_metrics();
    return {x, y, pen_block_width(type), m.h};
}

void pen_block_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state,
                    PenBlockType type, int x, int y,
                    int a, int opt, bool ghost, Color panel_bg,
                    int selected_field, const char *override_field0_text)
{
    Color pen_col = {15, 189, 140}; // Scratch Pen Green
    SDL_Rect br = pen_block_rect(type, x, y);
    draw_stack_shape(r, br, pen_col, panel_bg, ghost);

    int padding_x = 12;
    int cy = br.y + (br.h - 16) / 2;
    int cap_h = 22;
    int cap_y = br.y + (br.h - cap_h) / 2;
    int cur_x = br.x + padding_x;
    Color txt_col = {255, 255, 255};

    char bufA[32];
    std::snprintf(bufA, sizeof(bufA), "%d", a);

    auto draw_word = [&](const char *w)
    {
        draw_text(r, font, w, cur_x, cy, txt_col);
        cur_x += text_w(font, w) + 6;
    };

    auto draw_num_caps = [&](const char *num_default, int field_index, int cap_w)
    {
        const char *show = num_default;
        if (override_field0_text && selected_field == field_index)
            show = override_field0_text;
        SDL_Rect cap = input_capsule_rect(cur_x, cap_y, cap_w, cap_h);
        draw_input_capsule(r, cap, selected_field == field_index);
        int tw = text_w(font, show);
        int tx = (tw <= cap.w - 10) ? (cap.x + (cap.w - tw) / 2) : (cap.x + 6);
        draw_text(r, font, show, tx, cap.y + (cap.h - 16) / 2, (Color){40, 40, 40});
        cur_x += cap.w + 6;
    };

    auto draw_dd = [&](const char *txt)
    {
        int tw = text_w(font, txt);
        int cap_w = std::max(90, tw + 26);
        SDL_Rect dd = {cur_x, cap_y, cap_w, cap_h};
        draw_dropdown_capsule(r, dd, (Color){10, 150, 110});
        int tx = dd.x + 10;
        int ty = dd.y + (dd.h - 16) / 2;
        draw_text(r, font, txt, tx, ty, (Color){255, 255, 255});
        draw_caret(r, dd.x + dd.w - 12, dd.y + dd.h / 2, (Color){255, 255, 255});
        cur_x += cap_w + 6;
    };

    auto draw_color_picker = [&]()
    {
        SDL_Rect cap = input_capsule_rect(cur_x, cap_y, 40, cap_h);
        Color c = {0, 255, 0};
        if (state.selected_sprite >= 0 && state.selected_sprite < (int)state.sprites.size())
        {
            auto &sc = state.sprites[state.selected_sprite].pen_color;
            c = {sc.r, sc.g, sc.b};
        }
        renderer_fill_rounded_rect(r, &cap, cap.h / 2, c.r, c.g, c.b);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, 0, 0, 0, 50);
        SDL_RenderDrawRect(r, &cap);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        cur_x += 46;
    };

    const char *attribs[] = {"color", "saturation", "brightness"};
    const char *attr_str = (opt >= 0 && opt < 3) ? attribs[opt] : "color";

    switch (type)
    {
    case PB_ERASE_ALL:
        draw_word("erase");
        draw_word("all");
        break;
    case PB_STAMP:
        draw_word("stamp");
        break;
    case PB_PEN_DOWN:
        draw_word("pen");
        draw_word("down");
        break;
    case PB_PEN_UP:
        draw_word("pen");
        draw_word("up");
        break;
    case PB_SET_COLOR_TO_PICKER:
        draw_word("set");
        draw_word("pen");
        draw_word("color");
        draw_word("to");
        draw_color_picker();
        break;
    case PB_CHANGE_ATTRIB_BY:
        draw_word("change");
        draw_word("pen");
        draw_dd(attr_str);
        draw_word("by");
        draw_num_caps(bufA, 0, 40);
        break;
    case PB_SET_ATTRIB_TO:
        draw_word("set");
        draw_word("pen");
        draw_dd(attr_str);
        draw_word("to");
        draw_num_caps(bufA, 0, 40);
        break;
    case PB_CHANGE_SIZE_BY:
        draw_word("change");
        draw_word("pen");
        draw_word("size");
        draw_word("by");
        draw_num_caps(bufA, 0, 40);
        break;
    case PB_SET_SIZE_TO:
        draw_word("set");
        draw_word("pen");
        draw_word("size");
        draw_word("to");
        draw_num_caps(bufA, 0, 40);
        break;
    }
}

int pen_block_hittest_field(TTF_Font *font, PenBlockType type, int x, int y, int opt, int px, int py)
{
    SDL_Rect br = pen_block_rect(type, x, y);
    if (!(px >= br.x && px < br.x + br.w && py >= br.y && py < br.y + br.h))
        return -1;

    int cur_x = br.x + 12;
    int cap_y = br.y + (br.h - 22) / 2;
    auto adv_word = [&](const char *w)
    { cur_x += text_w(font, w) + 6; };
    auto cap_rect = [&](int w)
    { SDL_Rect rc{cur_x, cap_y, w, 22}; cur_x += w + 6; return rc; };
    auto in = [&](const SDL_Rect &rc)
    { return px >= rc.x && px < rc.x + rc.w && py >= rc.y && py < rc.y + rc.h; };

    const char *attribs[] = {"color", "saturation", "brightness"};
    const char *attr_str = (opt >= 0 && opt < 3) ? attribs[opt] : "color";

    switch (type)
    {
    case PB_SET_COLOR_TO_PICKER:
        adv_word("set");
        adv_word("pen");
        adv_word("color");
        adv_word("to");
        if (in(cap_rect(40)))
            return -4; // specialized return code for the color picker
        return -1;
    case PB_CHANGE_ATTRIB_BY:
        adv_word("change");
        adv_word("pen");
        if (in(cap_rect(std::max(90, text_w(font, attr_str) + 26))))
            return -2; // dropdown
        adv_word("by");
        if (in(cap_rect(40)))
            return 0;
        return -1;
    case PB_SET_ATTRIB_TO:
        adv_word("set");
        adv_word("pen");
        if (in(cap_rect(std::max(90, text_w(font, attr_str) + 26))))
            return -2; // dropdown
        adv_word("to");
        if (in(cap_rect(40)))
            return 0;
        return -1;
    case PB_CHANGE_SIZE_BY:
        adv_word("change");
        adv_word("pen");
        adv_word("size");
        adv_word("by");
        if (in(cap_rect(40)))
            return 0;
        return -1;
    case PB_SET_SIZE_TO:
        adv_word("set");
        adv_word("pen");
        adv_word("size");
        adv_word("to");
        if (in(cap_rect(40)))
            return 0;
        return -1;
    default:
        return -1;
    }
}