// palette.cpp

#include "palette.h"
#include "config.h"
#include "blocks.h"
#include "renderer.h"
#include "block_ui.h"
#include "workspace.h"

static bool point_in_rect(int px, int py, const SDL_Rect &r)
{
    return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
}

static void set_color(SDL_Renderer *r, Color c)
{
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255);
}

void palette_layout(PaletteRects &rects)
{
    int top = NAVBAR_HEIGHT + TAB_BAR_HEIGHT;
    int panel_h = WINDOW_HEIGHT - top;

    rects.panel.x = CATEGORY_WIDTH;
    rects.panel.y = top;
    rects.panel.w = PALETTE_WIDTH;
    rects.panel.h = panel_h;
}

void palette_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state,
                  const PaletteRects &rects, const Textures &tex)
{
    set_color(r, COL_PALETTE_BG);
    SDL_RenderFillRect(r, &rects.panel);

    set_color(r, COL_SEPARATOR);
    SDL_RenderDrawLine(r, rects.panel.x + rects.panel.w - 1, rects.panel.y,
                       rects.panel.x + rects.panel.w - 1,
                       rects.panel.y + rects.panel.h);

    BlockDef blocks[64];
    int count = blocks_get_for_category(state.selected_category, blocks, 64);

    int bx = rects.panel.x + 10;

    // ---- IMPORTANT: extra top padding for Events hat blocks ----
    // draw_events_hat uses cap_h=22 and draws at y - cap_h/2 => 11px goes above br.y
    const int TOP_PAD = 10;
    const int EVENTS_HAT_EXTRA = 11;
    int by = rects.panel.y + TOP_PAD + EVENTS_HAT_EXTRA;

    for (int i = 0; i < count; ++i)
    {
        if (!blocks[i].is_stack_block)
        {
            SDL_Rect br{bx, by, blocks[i].width, blocks[i].height};
            renderer_fill_rounded_rect(r, &br, 6,
                                       blocks[i].color.r, blocks[i].color.g, blocks[i].color.b);
            by += blocks[i].height + 6;
            continue;
        }

        int step = motion_block_metrics().h + 10;

        if (blocks[i].kind == BK_MOTION)
        {
            BlockInstance def = workspace_make_default((MotionBlockType)blocks[i].subtype);
            motion_block_draw(r, font, tex,
                              (MotionBlockType)blocks[i].subtype,
                              bx, by,
                              def.a, def.b, (GoToTarget)def.opt,
                              false, COL_PALETTE_BG, -1);
        }
        else if (blocks[i].kind == BK_LOOKS)
        {
            BlockInstance def = workspace_make_default_looks((LooksBlockType)blocks[i].subtype);
            looks_block_draw(r, font,
                             (LooksBlockType)blocks[i].subtype,
                             bx, by,
                             def.text,
                             def.a, def.b, def.opt,
                             false, COL_PALETTE_BG, -1,
                             nullptr, nullptr);
        }
        else if (blocks[i].kind == BK_SOUND)
        {
            BlockInstance def = workspace_make_default_sound((SoundBlockType)blocks[i].subtype);
            sound_block_draw(r, font,
                             (SoundBlockType)blocks[i].subtype,
                             bx, by,
                             def.a, def.opt,
                             false, COL_PALETTE_BG, -1,
                             nullptr);
        }
        else if (blocks[i].kind == BK_EVENTS)
        {
            BlockInstance def = workspace_make_default_events((EventsBlockType)blocks[i].subtype);
            events_block_draw(r, font, tex,
                              (EventsBlockType)blocks[i].subtype,
                              bx, by,
                              def.opt,
                              false, COL_PALETTE_BG, -1);

            // optional: a bit more breathing room between event blocks
            step += 20;
        }
        // در تابع palette_draw، بعد از case BK_EVENTS:

        else if (blocks[i].kind == BK_SENSING)
        {
            SensingBlockType sbt = (SensingBlockType)blocks[i].subtype;
            BlockInstance def = workspace_make_default_sensing(sbt);

            if (sbt == SENSB_TOUCHING || sbt == SENSB_KEY_PRESSED ||
                sbt == SENSB_MOUSE_DOWN)
            {
                // Boolean block: opt, a, b, c, d, e, f
                // a..f = color RGB values (for touching-color blocks), default 0
                sensing_boolean_block_draw(r, font, sbt, bx, by,
                                           def.opt,
                                           0, 0, 0, // r1, g1, b1 (color 1)
                                           0, 0, 0, // r2, g2, b2 (color 2)
                                           false, COL_PALETTE_BG, -1,
                                           nullptr);
                SDL_Rect br = sensing_boolean_block_rect(sbt, bx, by, def.opt);
                step = br.h + 10;
            }
            else
            {
                // Stack block: text, opt
                sensing_stack_block_draw(r, font, sbt, bx, by,
                                         def.text, def.opt,
                                         false, COL_PALETTE_BG, -1,
                                         nullptr);
                SDL_Rect br = sensing_stack_block_rect(sbt, bx, by, def.opt);
                step = br.h + 10;
            }
        }

        by += step;
    }
}

bool palette_handle_event(const SDL_Event &e, AppState &state,
                          const PaletteRects &rects,
                          TTF_Font *font)
{
    (void)font;

    if (state.drag.active)
        return false;

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        int mx = e.button.x;
        int my = e.button.y;

        if (!point_in_rect(mx, my, rects.panel))
            return false;

        BlockDef blocks[64];
        int count = blocks_get_for_category(state.selected_category, blocks, 64);

        int bx = rects.panel.x + 10;

        // Must match palette_draw() so click aligns with visuals
        const int TOP_PAD = 10;
        const int EVENTS_HAT_EXTRA = 11;
        int by = rects.panel.y + TOP_PAD + EVENTS_HAT_EXTRA;

        for (int i = 0; i < count; ++i)
        {
            if (!blocks[i].is_stack_block)
            {
                by += blocks[i].height + 6;
                continue;
            }

            SDL_Rect br{0, 0, 0, 0};

            if (blocks[i].kind == BK_MOTION)
            {
                br = motion_block_rect((MotionBlockType)blocks[i].subtype, bx, by);
            }
            else if (blocks[i].kind == BK_LOOKS)
            {
                br = looks_block_rect((LooksBlockType)blocks[i].subtype, bx, by);
            }
            else if (blocks[i].kind == BK_SOUND)
            {
                BlockInstance def = workspace_make_default_sound((SoundBlockType)blocks[i].subtype);
                br = sound_block_rect((SoundBlockType)blocks[i].subtype, bx, by, def.a, def.opt);
            }
            else if (blocks[i].kind == BK_EVENTS)
            {
                BlockInstance def = workspace_make_default_events((EventsBlockType)blocks[i].subtype);
                br = events_block_rect((EventsBlockType)blocks[i].subtype, bx, by, def.opt);

                // Expand hitbox upward to include the Events hat (11px)
                const int HAT_EXTRA = 11;
                br.y -= HAT_EXTRA;
                br.h += HAT_EXTRA;
            }

            else if (blocks[i].kind == BK_SENSING)
            {
                SensingBlockType sbt = (SensingBlockType)blocks[i].subtype;
                BlockInstance def = workspace_make_default_sensing(sbt);

                if (sbt == SENSB_TOUCHING || sbt == SENSB_KEY_PRESSED ||
                    sbt == SENSB_MOUSE_DOWN)
                {
                    br = sensing_boolean_block_rect(sbt, bx, by, def.opt);
                }
                else
                {
                    br = sensing_stack_block_rect(sbt, bx, by, def.opt);
                }
            }

            // ---- این بخش باید بعد از محاسبه br برای همه categoryها باشد ----
            if (point_in_rect(mx, my, br))
            {
                state.drag.active = true;
                state.drag.from_palette = true;
                state.drag.palette_kind = blocks[i].kind;
                state.drag.palette_subtype = blocks[i].subtype;

                state.drag.dragged_block_id = -1;
                state.drag.off_x = mx - br.x;
                state.drag.off_y = my - br.y;
                state.drag.ghost_x = br.x;
                state.drag.ghost_y = br.y;
                state.drag.mouse_x = mx;
                state.drag.mouse_y = my;

                state.drag.snap_valid = false;
                state.drag.snap_target_id = -1;
                return true;
            }

            // advance like draw()
            int step = motion_block_metrics().h + 10;
            if (blocks[i].kind == BK_EVENTS)
                step += 20;
            by += step;
        }

        return true;
    }

    return false;
}
