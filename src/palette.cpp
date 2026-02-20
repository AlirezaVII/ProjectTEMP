#include "palette.h"
#include "blocks.h"
#include "block_ui.h"
#include "renderer.h"
#include "workspace.h"

static bool point_in_rect(int px, int py, const SDL_Rect &r)
{
    return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
}

void palette_layout(PaletteRects &rects)
{
    rects.panel.x = CATEGORY_WIDTH;
    rects.panel.y = NAVBAR_HEIGHT;
    rects.panel.w = PALETTE_WIDTH;
    rects.panel.h = WINDOW_HEIGHT - NAVBAR_HEIGHT;
}

void palette_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state,
                  const PaletteRects &rects, const Textures &tex)
{
    Color bg = {249, 249, 249};
    SDL_SetRenderDrawColor(r, bg.r, bg.g, bg.b, 255);
    SDL_RenderFillRect(r, &rects.panel);

    SDL_SetRenderDrawColor(r, 220, 220, 220, 255);
    SDL_RenderDrawLine(r, rects.panel.x + rects.panel.w - 1, rects.panel.y,
                       rects.panel.x + rects.panel.w - 1, rects.panel.y + rects.panel.h);

    BlockDef defs[32];
    int n = blocks_get_for_category(state.selected_category, defs, 32);

    int bx = rects.panel.x + 12;
    int by = rects.panel.y + 60; // <-- Kept your beautiful 45 spacing!

    for (int i = 0; i < n; i++)
    {
        SDL_Rect br = {0, 0, 0, 0};
        if (defs[i].kind == BK_MOTION)
            br = motion_block_rect((MotionBlockType)defs[i].subtype, bx, by);
        else if (defs[i].kind == BK_LOOKS)
            br = looks_block_rect((LooksBlockType)defs[i].subtype, bx, by);
        else if (defs[i].kind == BK_SOUND)
            br = sound_block_rect((SoundBlockType)defs[i].subtype, bx, by, 0, 0);
        else if (defs[i].kind == BK_EVENTS)
            br = events_block_rect((EventsBlockType)defs[i].subtype, bx, by, 0);
        else if (defs[i].kind == BK_SENSING)
        {
            if (defs[i].is_boolean_block)
                br = sensing_boolean_block_rect((SensingBlockType)defs[i].subtype, bx, by, 0);
            else
                br = sensing_stack_block_rect((SensingBlockType)defs[i].subtype, bx, by, 0);
        }
        else if (defs[i].kind == BK_CONTROL)
        {
            br = control_block_rect((ControlBlockType)defs[i].subtype, bx, by, 24, 24);
        }
        else if (defs[i].kind == BK_OPERATORS)
        {
            br = operators_block_rect((OperatorsBlockType)defs[i].subtype, bx, by);
        }
        else
        {
            br = {bx, by, defs[i].width, defs[i].height};
        }

        if (defs[i].is_stack_block || defs[i].is_c_shape)
        {
            BlockInstance def;
            if (defs[i].kind == BK_MOTION)
            {
                def = workspace_make_default((MotionBlockType)defs[i].subtype);
                motion_block_draw(r, font, tex, (MotionBlockType)defs[i].subtype, bx, by, def.a, def.b, (GoToTarget)def.opt, false, bg, -1);
            }
            else if (defs[i].kind == BK_LOOKS)
            {
                def = workspace_make_default_looks((LooksBlockType)defs[i].subtype);
                looks_block_draw(r, font, (LooksBlockType)defs[i].subtype, bx, by, def.text, def.a, def.b, def.opt, false, bg, -1, nullptr, nullptr);
            }
            else if (defs[i].kind == BK_SOUND)
            {
                def = workspace_make_default_sound((SoundBlockType)defs[i].subtype);
                sound_block_draw(r, font, (SoundBlockType)defs[i].subtype, bx, by, def.a, def.opt, false, bg, -1, nullptr);
            }
            else if (defs[i].kind == BK_EVENTS)
            {
                def = workspace_make_default_events((EventsBlockType)defs[i].subtype);
                events_block_draw(r, font, tex, (EventsBlockType)defs[i].subtype, bx, by, def.opt, false, bg, -1);
            }
            else if (defs[i].kind == BK_SENSING)
            {
                def = workspace_make_default_sensing((SensingBlockType)defs[i].subtype);
                sensing_stack_block_draw(r, font, (SensingBlockType)defs[i].subtype, bx, by, def.text, def.opt, false, bg, -1, nullptr);
            }
            else if (defs[i].kind == BK_CONTROL)
            {
                int def_a = (defs[i].subtype == CB_WAIT) ? 1 : 10;
                control_block_draw(r, font, (ControlBlockType)defs[i].subtype, bx, by, 24, 24, def_a, false, false, bg, -1, nullptr);
            }
            // ---> ADD THIS ENTIRE BLOCK FOR OPERATORS <---
            else if (defs[i].kind == BK_OPERATORS)
            {
                int def_a = (defs[i].subtype == OP_RANDOM) ? 1 : 0;
                int def_b = 0;
                if (defs[i].subtype == OP_RANDOM)
                    def_b = 10;
                if (defs[i].subtype == OP_GT || defs[i].subtype == OP_LT || defs[i].subtype == OP_EQ)
                    def_b = 50;

                operators_block_draw(r, font, (OperatorsBlockType)defs[i].subtype, bx, by, "", "", def_a, def_b, false, bg, -1, nullptr, nullptr);
            }
        }
        else if (defs[i].is_boolean_block)
        {
            if (defs[i].kind == BK_SENSING)
            {
                BlockInstance def = workspace_make_default_sensing((SensingBlockType)defs[i].subtype);
                sensing_boolean_block_draw(r, font, (SensingBlockType)defs[i].subtype, bx, by, def.opt, def.a, def.b, def.c, def.d, def.e, def.f, false, bg, -1, nullptr);
            }
        }
        else
        {
            renderer_fill_rounded_rect(r, &br, 8, defs[i].color.r, defs[i].color.g, defs[i].color.b);
        }

        // --- FIX FOR EVENTS OVERLAP ---
        int padding = 12; // Base gap for all blocks
        if (defs[i].kind == BK_EVENTS)
        {
            padding = 28; // Event hats stick out the top, so the block *below* needs more distance!
        }
        by += br.h + padding;
    }
}

bool palette_handle_event(const SDL_Event &e, AppState &state,
                          const PaletteRects &rects, TTF_Font *font)
{
    (void)font;
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        int mx = e.button.x;
        int my = e.button.y;

        if (point_in_rect(mx, my, rects.panel))
        {
            BlockDef defs[32];
            int n = blocks_get_for_category(state.selected_category, defs, 32);

            int bx = rects.panel.x + 12;
            int by = rects.panel.y + 60; // <-- Kept your beautiful 45 spacing!

            for (int i = 0; i < n; i++)
            {
                SDL_Rect br = {0, 0, 0, 0};
                if (defs[i].kind == BK_MOTION)
                    br = motion_block_rect((MotionBlockType)defs[i].subtype, bx, by);
                else if (defs[i].kind == BK_LOOKS)
                    br = looks_block_rect((LooksBlockType)defs[i].subtype, bx, by);
                else if (defs[i].kind == BK_SOUND)
                    br = sound_block_rect((SoundBlockType)defs[i].subtype, bx, by, 0, 0);
                else if (defs[i].kind == BK_EVENTS)
                    br = events_block_rect((EventsBlockType)defs[i].subtype, bx, by, 0);
                else if (defs[i].kind == BK_SENSING)
                {
                    if (defs[i].is_boolean_block)
                        br = sensing_boolean_block_rect((SensingBlockType)defs[i].subtype, bx, by, 0);
                    else
                        br = sensing_stack_block_rect((SensingBlockType)defs[i].subtype, bx, by, 0);
                }
                else if (defs[i].kind == BK_CONTROL)
                {
                    br = control_block_rect((ControlBlockType)defs[i].subtype, bx, by, 24, 24);
                }
                else if (defs[i].kind == BK_OPERATORS)
                {
                    br = operators_block_rect((OperatorsBlockType)defs[i].subtype, bx, by);
                }
                else
                {
                    br = {bx, by, defs[i].width, defs[i].height};
                }

                if (point_in_rect(mx, my, br))
                {
                    state.drag.active = true;
                    state.drag.from_palette = true;
                    state.drag.palette_kind = defs[i].kind;
                    state.drag.palette_subtype = defs[i].subtype;

                    state.drag.off_x = mx - bx;
                    state.drag.off_y = my - by;
                    state.drag.ghost_x = bx;
                    state.drag.ghost_y = by;
                    state.drag.mouse_x = mx;
                    state.drag.mouse_y = my;

                    state.drag.snap_valid = false;
                    state.drag.snap_target_id = -1;
                    return true;
                }

                // --- FIX FOR CLICK HITBOXES OVERLAPPING ---
                int padding = 12;
                if (defs[i].kind == BK_EVENTS)
                {
                    padding = 28;
                }
                by += br.h + padding;
            }
            return true;
        }
    }
    return false;
}