#include "stage.h"
#include "config.h"
#include "renderer.h"

static bool point_in_rect(int px, int py, const SDL_Rect &r) {
    return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
}

static void set_color(SDL_Renderer *r, Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255);
}

void stage_layout(StageRects &rects) {
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
                const StageRects &rects, const Textures &tex) {
    /* panel bg */
    set_color(r, COL_STAGE_BG);
    SDL_RenderFillRect(r, &rects.panel);

    /* border */
    set_color(r, COL_STAGE_BORDER);
    SDL_RenderDrawRect(r, &rects.stage_area);

    /* Render the Sprite */
    if (state.sprite.visible) {
        int cx = rects.stage_area.x + rects.stage_area.w / 2 + state.sprite.x;
        int cy = rects.stage_area.y + rects.stage_area.h / 2 - state.sprite.y;

        int tex_w = 100, tex_h = 100;
        if (tex.scratch_cat) {
            SDL_QueryTexture(tex.scratch_cat, NULL, NULL, &tex_w, &tex_h);
        }

        /* Apply Size */
        int w = (tex_w * state.sprite.size) / 100;
        int h = (tex_h * state.sprite.size) / 100;

        SDL_Rect dest = { cx - w / 2, cy - h / 2, w, h };

        /* Clip inside stage so sprite doesn't bleed out */
        SDL_RenderSetClipRect(r, const_cast<SDL_Rect*>(&rects.stage_area));
        
        if (tex.scratch_cat) {
            double angle = state.sprite.direction - 90.0;
            SDL_RenderCopyEx(r, tex.scratch_cat, NULL, &dest, angle, NULL, SDL_FLIP_NONE);
        } else {
            /* Fallback orange box if image is missing */
            SDL_SetRenderDrawColor(r, 255, 165, 0, 255);
            SDL_RenderFillRect(r, &dest);
        }
        SDL_RenderSetClipRect(r, NULL);
    }
}

bool stage_handle_event(const SDL_Event &e, AppState &state,
                        const StageRects &rects, const Textures &tex) {
    int tex_w = 100, tex_h = 100;
    if (tex.scratch_cat) {
        SDL_QueryTexture(tex.scratch_cat, NULL, NULL, &tex_w, &tex_h);
    }
    int w = (tex_w * state.sprite.size) / 100;
    int h = (tex_h * state.sprite.size) / 100;

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x, my = e.button.y;
        if (point_in_rect(mx, my, rects.stage_area)) {
            if (state.sprite.visible) {
                int cx = rects.stage_area.x + rects.stage_area.w / 2 + state.sprite.x;
                int cy = rects.stage_area.y + rects.stage_area.h / 2 - state.sprite.y;
                SDL_Rect sprite_rect = {cx - w / 2, cy - h / 2, w, h};
                
                if (point_in_rect(mx, my, sprite_rect)) {
                    state.stage_drag_active = true;
                    state.stage_drag_off_x = mx - cx;
                    state.stage_drag_off_y = my - cy;
                    return true;
                }
            }
        }
    } else if (e.type == SDL_MOUSEMOTION) {
        if (state.stage_drag_active) {
            int cx = e.motion.x - state.stage_drag_off_x;
            int cy = e.motion.y - state.stage_drag_off_y;
            state.sprite.x = cx - (rects.stage_area.x + rects.stage_area.w / 2);
            state.sprite.y = (rects.stage_area.y + rects.stage_area.h / 2) - cy; // Y is inverted visually
            return true;
        }
    } else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
        if (state.stage_drag_active) {
            state.stage_drag_active = false;
            return true;
        }
    }
    return false;
}