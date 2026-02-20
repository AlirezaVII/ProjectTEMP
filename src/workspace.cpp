#include "workspace.h"
#include "block_ui.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <unordered_set>


/* ── helper: is this a Sensing Boolean (hexagonal) block? ── */
static bool is_boolean_block(BlockKind kind, int subtype)
{
    if (kind != BK_SENSING) return false;
    SensingBlockType st = (SensingBlockType)subtype;
    return (st == SENSB_TOUCHING ||
            st == SENSB_KEY_PRESSED ||
            st == SENSB_MOUSE_DOWN);
}

/* ---------------- utils ---------------- */

static bool point_in_rect(int px, int py, const SDL_Rect &r)
{
    return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
}

BlockInstance *workspace_find(AppState &state, int id)
{
    for (auto &b : state.blocks)
    {
        if (b.id == id)
            return &b;
    }
    return nullptr;
}

const BlockInstance *workspace_find_const(const AppState &state, int id)
{
    for (const auto &b : state.blocks)
    {
        if (b.id == id)
            return &b;
    }
    return nullptr;
}

/* Find last id in a chain starting at root */
static int last_in_chain(const AppState &state, int root_id)
{
    int cur = root_id;
    int last = cur;
    while (cur != -1)
    {
        const BlockInstance *b = workspace_find_const(state, cur);
        if (!b)
            break;
        last = cur;
        cur = b->next_id;
    }
    return last;
}

static SDL_Rect block_rect(const BlockInstance &b)
{
    if (b.kind == BK_MOTION)
        return motion_block_rect((MotionBlockType)b.subtype, b.x, b.y);
    if (b.kind == BK_LOOKS)
        return looks_block_rect((LooksBlockType)b.subtype, b.x, b.y);
    if (b.kind == BK_SOUND)
        return sound_block_rect((SoundBlockType)b.subtype, b.x, b.y, b.a, b.opt);
    if (b.kind == BK_EVENTS)
        return events_block_rect((EventsBlockType)b.subtype, b.x, b.y, b.opt);

    if (b.kind == BK_SENSING)
    {
        SensingBlockType sbt = (SensingBlockType)b.subtype;
        if (sbt == SENSB_TOUCHING || sbt == SENSB_KEY_PRESSED ||
            sbt == SENSB_MOUSE_DOWN)
        {
            return sensing_boolean_block_rect(sbt, b.x, b.y, b.opt);
        }
        else
        {
            return sensing_stack_block_rect(sbt, b.x, b.y, b.opt);
        }
    }

    // fallback
    return {b.x, b.y, 240, 40};
}

static int block_height(const BlockInstance &b)
{
    return block_rect(b).h;
}

/* ---------------- defaults ---------------- */

BlockInstance workspace_make_default(MotionBlockType type)
{
    BlockInstance b;
    b.kind = BK_MOTION;
    b.subtype = (int)type;
    b.next_id = -1;
    b.parent_id = -1;

    switch (type)
    {
    case MB_MOVE_STEPS:
        b.a = 10;
        break;
    case MB_TURN_RIGHT_DEG:
        b.a = 15;
        break;
    case MB_TURN_LEFT_DEG:
        b.a = 15;
        break;
    case MB_GO_TO_XY:
        b.a = 0;
        b.b = 0;
        break;
    case MB_CHANGE_X_BY:
        b.a = 10;
        break;
    case MB_CHANGE_Y_BY:
        b.a = 10;
        break;
    case MB_POINT_IN_DIR:
        b.a = 90;
        break;
    case MB_GO_TO_TARGET:
        b.opt = (int)TARGET_RANDOM_POSITION;
        break;
    default:
        break;
    }

    return b;
}

BlockInstance workspace_make_default_looks(LooksBlockType type)
{
    BlockInstance b;
    b.kind = BK_LOOKS;
    b.subtype = (int)type;
    b.next_id = -1;
    b.parent_id = -1;

    switch (type)
    {
    case LB_SAY:
        b.text = "Hello!";
        break;
    case LB_SAY_FOR:
        b.text = "Hello!";
        b.a = 2;
        break;
    case LB_THINK:
        b.text = "Hmm...";
        break;
    case LB_THINK_FOR:
        b.text = "Hmm...";
        b.a = 2;
        break;
    case LB_SWITCH_COSTUME_TO:
        b.opt = 0;
        break;
    case LB_SWITCH_BACKDROP_TO:
        b.opt = 0;
        break;
    case LB_CHANGE_SIZE_BY:
        b.a = 10;
        break;
    case LB_SET_SIZE_TO:
        b.a = 100;
        break;
    case LB_GO_TO_LAYER:
        b.opt = 0;
        break;
    case LB_GO_LAYERS:
        b.opt = 0;
        b.a = 1;
        break;
    default:
        break;
    }

    return b;
}

BlockInstance workspace_make_default_sound(SoundBlockType type)
{
    BlockInstance b;
    b.kind = BK_SOUND;
    b.subtype = (int)type;
    b.next_id = -1;
    b.parent_id = -1;

    switch (type)
    {
    case SB_CHANGE_VOLUME_BY:
        b.a = -10;
        break;
    case SB_SET_VOLUME_TO:
        b.a = 100;
        break;
    case SB_START_SOUND:
    case SB_PLAY_SOUND_UNTIL_DONE:
        b.opt = 0; /* only "Meow" for now */
        break;
    case SB_STOP_ALL_SOUNDS:
    default:
        break;
    }

    return b;
}

BlockInstance workspace_make_default_events(EventsBlockType type)
{
    BlockInstance b;
    b.kind = BK_EVENTS;
    b.subtype = (int)type;
    b.next_id = -1;
    b.parent_id = -1;

    switch (type)
    {
    case EB_WHEN_KEY_PRESSED:
        b.opt = 0;
        break; // space
    case EB_WHEN_I_RECEIVE:
        b.opt = 0;
        break; // message1
    case EB_BROADCAST:
        b.opt = 0;
        break; // message1
    default:
        b.opt = 0;
        break;
    }
    return b;
}
BlockInstance workspace_make_default_sensing(SensingBlockType type)
{
    BlockInstance b;
    b.kind = BK_SENSING;
    b.subtype = (int)type;
    b.next_id = -1;
    b.parent_id = -1;

    switch (type)
    {
    case SENSB_TOUCHING:
        b.opt = 0; // 0=mouse-pointer, 1=edge, 2=sprite
        break;
    case SENSB_ASK_AND_WAIT:
        b.text = "What's your name?";
        break;
    case SENSB_KEY_PRESSED:
        b.opt = 0; // 0=space, 1..26=a..z, 27..36=0..9
        break;
    case SENSB_MOUSE_DOWN:
        break; // بدون پارامتر
    case SENSB_SET_DRAG_MODE:
        b.opt = 0; // 0=draggable, 1=not draggable
        break;
    default:
        break;
    }

    return b;
}

/* --------------- add / layout ---------------- */

int workspace_add_top_level(AppState &state, const BlockInstance &b0)
{
    BlockInstance b = b0;
    b.id = state.next_block_id++;
    state.blocks.push_back(b);
    state.top_level_blocks.push_back(b.id);
    return b.id;
}

int workspace_root_id(const AppState &state, int id)
{
    const BlockInstance *b = workspace_find_const(state, id);
    if (!b)
        return -1;

    int cur = id;
    int parent = b->parent_id;
    while (parent != -1)
    {
        cur = parent;
        const BlockInstance *p = workspace_find_const(state, parent);
        if (!p)
            break;
        parent = p->parent_id;
    }
    return cur;
}

void workspace_layout_chain(AppState &state, int root_id)
{
    BlockInstance *root = workspace_find(state, root_id);
    if (!root)
        return;

    MotionBlockMetrics m = motion_block_metrics();
    int cur = root_id;
    int x = root->x;
    int y = root->y;

    while (cur != -1)
    {
        BlockInstance *b = workspace_find(state, cur);
        if (!b)
            break;

        b->x = x;
        b->y = y;

        y += (block_height(*b) - m.overlap);
        cur = b->next_id;
    }
}

/* --------------- detach / attach ---------------- */

static void remove_from_top_level(AppState &state, int root_id)
{
    auto &tl = state.top_level_blocks;
    tl.erase(std::remove(tl.begin(), tl.end(), root_id), tl.end());
}

/* Detach chain starting at id from its parent (or top-level list) and return root of detached chain */
static int detach_subchain(AppState &state, int id)
{
    BlockInstance *b = workspace_find(state, id);
    if (!b)
        return -1;

    int parent = b->parent_id;
    if (parent == -1)
    {
        remove_from_top_level(state, id);
    }
    else
    {
        BlockInstance *p = workspace_find(state, parent);
        if (p)
        {
            p->next_id = -1;
        }
        b->parent_id = -1;
    }
    return id;
}

// drag & drop from top

static void insert_chain_before(AppState &state, int target_id, int chain_root_id)
{
    BlockInstance *t = workspace_find(state, target_id);
    BlockInstance *r = workspace_find(state, chain_root_id);
    if (!t || !r)
        return;

    int parent = t->parent_id;

    // پیدا کردن آخر chain جدید
    int last_new = last_in_chain(state, chain_root_id);
    BlockInstance *lastb = workspace_find(state, last_new);
    if (!lastb)
        return;

    // chain جدید -> target
    lastb->next_id = target_id;
    t->parent_id = last_new;

    // root chain جدید parent می‌گیرد
    r->parent_id = parent;

    if (parent == -1)
    {
        // target یک top-level بوده: باید در top_level جایگزین شود
        bool replaced = false;
        for (size_t i = 0; i < state.top_level_blocks.size(); ++i)
        {
            if (state.top_level_blocks[i] == target_id)
            {
                state.top_level_blocks[i] = chain_root_id;
                replaced = true;
                break;
            }
        }
        if (!replaced)
        {
            state.top_level_blocks.push_back(chain_root_id);
        }
    }
    else
    {
        // parent قبلی باید به root جدید اشاره کند (نه به target)
        BlockInstance *p = workspace_find(state, parent);
        if (p)
            p->next_id = chain_root_id;
    }
}

static void attach_chain(AppState &state, int target_id, int chain_root_id)
{
    BlockInstance *t = workspace_find(state, target_id);
    BlockInstance *r = workspace_find(state, chain_root_id);
    if (!t || !r)
        return;

    int last = last_in_chain(state, target_id);
    BlockInstance *lastb = workspace_find(state, last);
    if (!lastb)
        return;

    lastb->next_id = chain_root_id;
    r->parent_id = last;
}

/* ---------------- delete action ---------------- */

static void delete_chain(AppState &state, int root_id)
{
    // جمع کردن تمام idهای زنجیره
    std::vector<int> ids;
    int cur = root_id;
    while (cur != -1)
    {
        ids.push_back(cur);
        BlockInstance *b = workspace_find(state, cur);
        if (!b)
            break;
        cur = b->next_id;
    }

    // پاک کردن از state.blocks
    state.blocks.erase(
        std::remove_if(state.blocks.begin(), state.blocks.end(),
                       [&](const BlockInstance &b)
                       {
                           return std::find(ids.begin(), ids.end(), b.id) != ids.end();
                       }),
        state.blocks.end());

    // پاک کردن از top_level اگر هنوز وجود داشت
    for (int id : ids)
    {
        auto &tl = state.top_level_blocks;
        tl.erase(std::remove(tl.begin(), tl.end(), id), tl.end());
    }

    // اگر کاربر داشت روی یکی از همین بلاک‌ها تایپ می‌کرد، input رو هم ببند
    if (state.active_input == INPUT_BLOCK_FIELD)
    {
        if (std::find(ids.begin(), ids.end(), state.block_input.block_id) != ids.end())
        {
            state.active_input = INPUT_NONE;
            state.input_buffer.clear();
            state.block_input.block_id = -1;
        }
    }
}

/* ── set snap_valid, but reject Boolean blocks ── */
/* ── set snap_valid, but reject Boolean blocks ── */
static void try_set_snap(AppState &state)
{
    bool dragging_bool = false;
    if (state.drag.from_palette)
        dragging_bool = is_boolean_block(state.drag.palette_kind,
                                          state.drag.palette_subtype);
    else if (state.drag.dragged_block_id >= 0 &&
             state.drag.dragged_block_id < (int)state.blocks.size())
        dragging_bool = is_boolean_block(
            state.blocks[state.drag.dragged_block_id].kind,
            state.blocks[state.drag.dragged_block_id].subtype);

    if (dragging_bool)
        state.drag.snap_valid = false;   // boolean → ❌ no stack snap
    else
        state.drag.snap_valid = true;    // stack → ✅ allow
}


/* ---------------- snap compute ---------------- */

static void compute_snap(AppState &state)
{
    state.drag.snap_above = false;
    state.drag.snap_valid = false;
    state.drag.snap_target_id = -1;

    if (!state.drag.active)
        return;

    const int SNAP_DIST = 18;

    auto get_drag_rect = [&]() -> SDL_Rect
    {
        if (state.drag.from_palette)
        {
            if (state.drag.palette_kind == BK_MOTION)
                return motion_block_rect((MotionBlockType)state.drag.palette_subtype, state.drag.ghost_x, state.drag.ghost_y);
            if (state.drag.palette_kind == BK_LOOKS)
                return looks_block_rect((LooksBlockType)state.drag.palette_subtype, state.drag.ghost_x, state.drag.ghost_y);
            if (state.drag.palette_kind == BK_SOUND)
            {
                BlockInstance def = workspace_make_default_sound((SoundBlockType)state.drag.palette_subtype);
                def.x = state.drag.ghost_x;
                def.y = state.drag.ghost_y;
                return sound_block_rect((SoundBlockType)state.drag.palette_subtype, def.x, def.y, def.a, def.opt);
            }
            if (state.drag.palette_kind == BK_EVENTS)
            {
                BlockInstance def = workspace_make_default_events((EventsBlockType)state.drag.palette_subtype);
                def.x = state.drag.ghost_x;
                def.y = state.drag.ghost_y;
                return events_block_rect((EventsBlockType)state.drag.palette_subtype, def.x, def.y, def.opt);
            }
            if (state.drag.palette_kind == BK_SENSING)
            {
                BlockInstance def = workspace_make_default_sensing((SensingBlockType)state.drag.palette_subtype);
                def.x = state.drag.ghost_x;
                def.y = state.drag.ghost_y;
                SensingBlockType sbt = (SensingBlockType)state.drag.palette_subtype;
                if (sbt == SENSB_TOUCHING || sbt == SENSB_KEY_PRESSED ||
                    sbt == SENSB_MOUSE_DOWN)
                {
                    return sensing_boolean_block_rect(sbt, def.x, def.y, def.opt);
                }
                else
                {
                    return sensing_stack_block_rect(sbt, def.x, def.y, def.opt);
                }
            }
        }
        else
        {
            const BlockInstance *root = workspace_find_const(state, state.drag.dragged_block_id);
            if (!root)
                return SDL_Rect{state.drag.ghost_x, state.drag.ghost_y, 240, 40};
            BlockInstance tmp = *root;
            tmp.x = state.drag.ghost_x;
            tmp.y = state.drag.ghost_y;
            return block_rect(tmp);
        }
    };

    SDL_Rect dr = get_drag_rect();
    int best_id = -1;
    int best_dx = 0, best_dy = 0;
    int best_dist = 999999;

    std::unordered_set<int> dragged_ids;
    if (!state.drag.from_palette)
    {
        int cur = state.drag.dragged_block_id;
        while (cur != -1)
        {
            dragged_ids.insert(cur);
            const BlockInstance *b = workspace_find_const(state, cur);
            if (!b)
                break;
            cur = b->next_id;
        }
    }

    for (int root_id : state.top_level_blocks)
    {
        int cur = root_id;
        while (cur != -1)
        {
            const BlockInstance *b = workspace_find_const(state, cur);
            if (!b)
                break;

            if (dragged_ids.count(cur))
            {
                cur = b->next_id;
                continue;
            }

            SDL_Rect br = block_rect(*b);

            // --- گزینه 1: snap به پایین target (append)
            {
                int snap_x = br.x;
                int snap_y = br.y + br.h - motion_block_metrics().overlap;

                int dx = snap_x - dr.x;
                int dy = snap_y - dr.y;

                if (std::abs(dx) <= SNAP_DIST && std::abs(dy) <= SNAP_DIST)
                {
                    int dist = std::abs(dx) + std::abs(dy);
                    if (dist < best_dist)
                    {
                        best_dist = dist;
                        best_id = cur;
                        best_dx = dx;
                        best_dy = dy;
                        state.drag.snap_above = false;
                    }
                }
            }

            // --- گزینه 2: snap به بالای target (insert before)
            // هدف: پایینِ بلاکِ درگ‌شده به بالای target بچسبد
            {
                int snap_x = br.x;

                // y مطلوب برای ریشه‌ی بلاکِ درگ‌شده:
                // dr.y + (dr.h - overlap) = br.y  =>  dr.y = br.y - (dr.h - overlap)
                int desired_drag_y = br.y - (dr.h - motion_block_metrics().overlap);

                int dx = snap_x - dr.x;
                int dy = desired_drag_y - dr.y;

                if (std::abs(dx) <= SNAP_DIST && std::abs(dy) <= SNAP_DIST)
                {
                    int dist = std::abs(dx) + std::abs(dy);
                    if (dist < best_dist)
                    {
                        best_dist = dist;
                        best_id = cur;
                        best_dx = dx;
                        best_dy = dy;
                        state.drag.snap_above = true;
                    }
                }
            }

            cur = b->next_id;
        }
    }

    if (best_id != -1)
    {
        try_set_snap(state);
        state.drag.snap_target_id = best_id;
        state.drag.snap_x = dr.x + best_dx;
        state.drag.snap_y = dr.y + best_dy;
    }
}

/* ---------------- draw ---------------- */

void workspace_draw(SDL_Renderer *r, TTF_Font *font, const Textures &tex,
                    const AppState &state, const SDL_Rect &workspace_rect, Color bg)
{
    (void)workspace_rect;

    for (int root_id : state.top_level_blocks)
    {
        int cur = root_id;
        while (cur != -1)
        {
            const BlockInstance *b = workspace_find_const(state, cur);
            if (!b)
                break;

            int selected_field = -1;
            const char *ov0 = nullptr;
            const char *ov1 = nullptr;

            /* اگر این بلاک همون بلاکیه که داریم روش تایپ می‌کنیم */
            if (state.active_input == INPUT_BLOCK_FIELD &&
                state.block_input.block_id == b->id)
            {
                selected_field = state.block_input.field_index;

                if (state.block_input.type == BFT_INT)
                {
                    if (state.block_input.field_index == 0)
                        ov0 = state.input_buffer.c_str();
                    if (state.block_input.field_index == 1)
                        ov1 = state.input_buffer.c_str();
                }
                else if (state.block_input.type == BFT_TEXT)
                {
                    // ورودی‌های متنی (say/think) فقط field 0 دارند
                    if (state.block_input.field_index == 0)
                        ov0 = state.input_buffer.c_str();
                }
            }

            if (b->kind == BK_MOTION)
            {
                motion_block_draw(r, font, tex,
                                  (MotionBlockType)b->subtype,
                                  b->x, b->y,
                                  b->a, b->b, (GoToTarget)b->opt,
                                  false, bg, selected_field,
                                  ov0, ov1);
            }
            else if (b->kind == BK_LOOKS)
            {
                looks_block_draw(r, font,
                                 (LooksBlockType)b->subtype,
                                 b->x, b->y,
                                 b->text,
                                 b->a, b->b, b->opt,
                                 false, bg, selected_field,
                                 ov0, ov1);
            }
            else if (b->kind == BK_SOUND)
            {
                sound_block_draw(r, font,
                                 (SoundBlockType)b->subtype,
                                 b->x, b->y,
                                 b->a, b->opt,
                                 false, bg, selected_field,
                                 ov0);
            }
            else if (b->kind == BK_EVENTS)
            {
                events_block_draw(r, font, tex,
                                  (EventsBlockType)b->subtype,
                                  b->x, b->y,
                                  b->opt,
                                  false, bg, -1);
            }
            else if (b->kind == BK_SENSING)
            {
                SensingBlockType sbt = (SensingBlockType)b->subtype;
                if (sbt == SENSB_TOUCHING || sbt == SENSB_KEY_PRESSED ||
                    sbt == SENSB_MOUSE_DOWN)
                {
                    sensing_boolean_block_draw(r, font,
                                               sbt,
                                               b->x, b->y,
                                               b->opt, b->a, b->b, b->c,
                                               b->d, b->e, b->f,
                                               false, bg, selected_field,
                                               ov0);
                }
                else
                {
                    sensing_stack_block_draw(r, font,
                                             sbt,
                                             b->x, b->y,
                                             b->text, b->opt,
                                             false, bg, selected_field,
                                             ov0);
                }
            }

            cur = b->next_id;
        }
    }

    /* snap shadow */
    if (state.drag.active && state.drag.snap_valid)
    {
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, 0, 0, 0, 80);

        SDL_Rect sr;
        if (state.drag.from_palette)
        {
            if (state.drag.palette_kind == BK_MOTION)
            {
                sr = motion_block_rect((MotionBlockType)state.drag.palette_subtype, state.drag.snap_x, state.drag.snap_y);
            }
            else if (state.drag.palette_kind == BK_LOOKS)
            {
                sr = looks_block_rect((LooksBlockType)state.drag.palette_subtype, state.drag.snap_x, state.drag.snap_y);
            }
            else if (state.drag.palette_kind == BK_SOUND)
            {
                BlockInstance def = workspace_make_default_sound((SoundBlockType)state.drag.palette_subtype);
                def.x = state.drag.snap_x;
                def.y = state.drag.snap_y;
                sr = sound_block_rect((SoundBlockType)state.drag.palette_subtype, def.x, def.y, def.a, def.opt);
            }
            else if (state.drag.palette_kind == BK_EVENTS)
            {
                BlockInstance def = workspace_make_default_events((EventsBlockType)state.drag.palette_subtype);
                def.x = state.drag.snap_x;
                def.y = state.drag.snap_y;
                sr = events_block_rect((EventsBlockType)state.drag.palette_subtype, def.x, def.y, def.opt);
            }
            else if (state.drag.palette_kind == BK_SENSING)
            {
                BlockInstance def = workspace_make_default_sensing((SensingBlockType)state.drag.palette_subtype);
                def.x = state.drag.snap_x;
                def.y = state.drag.snap_y;
                SensingBlockType sbt = (SensingBlockType)state.drag.palette_subtype;
                if (sbt == SENSB_TOUCHING || sbt == SENSB_KEY_PRESSED ||
                    sbt == SENSB_MOUSE_DOWN)
                {
                    sr = sensing_boolean_block_rect(sbt, def.x, def.y, def.opt);
                }
                else
                {
                    sr = sensing_stack_block_rect(sbt, def.x, def.y, def.opt);
                }
            }
        }
        else
        {
            const BlockInstance *root = workspace_find_const(state, state.drag.dragged_block_id);
            if (root)
            {
                BlockInstance tmp = *root;
                tmp.x = state.drag.snap_x;
                tmp.y = state.drag.snap_y;
                sr = block_rect(tmp);
            }
            else
            {
                sr = SDL_Rect{state.drag.snap_x, state.drag.snap_y, 240, 40};
            }
        }

        SDL_Rect sh = sr;
        sh.x += 2;
        sh.y += 2;
        SDL_RenderFillRect(r, &sh);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    }

    /* dragged ghost */
    if (state.drag.active)
    {
        int gx = state.drag.snap_valid ? state.drag.snap_x : state.drag.ghost_x;
        int gy = state.drag.snap_valid ? state.drag.snap_y : state.drag.ghost_y;

        if (state.drag.from_palette)
        {
            if (state.drag.palette_kind == BK_MOTION)
            {
                BlockInstance def = workspace_make_default((MotionBlockType)state.drag.palette_subtype);
                motion_block_draw(r, font, tex,
                                  (MotionBlockType)state.drag.palette_subtype,
                                  gx, gy,
                                  def.a, def.b, (GoToTarget)def.opt,
                                  true, bg, -1,
                                  nullptr, nullptr);
            }
            else if (state.drag.palette_kind == BK_LOOKS)
            {
                BlockInstance def = workspace_make_default_looks((LooksBlockType)state.drag.palette_subtype);
                looks_block_draw(r, font,
                                 (LooksBlockType)state.drag.palette_subtype,
                                 gx, gy,
                                 def.text,
                                 def.a, def.b, def.opt,
                                 true, bg, -1,
                                 nullptr, nullptr);
            }
            else if (state.drag.palette_kind == BK_SOUND)
            {
                BlockInstance def = workspace_make_default_sound((SoundBlockType)state.drag.palette_subtype);
                sound_block_draw(r, font,
                                 (SoundBlockType)state.drag.palette_subtype,
                                 gx, gy,
                                 def.a, def.opt,
                                 true, bg, -1,
                                 nullptr);
            }
            else if (state.drag.palette_kind == BK_EVENTS)
            {
                BlockInstance def = workspace_make_default_events((EventsBlockType)state.drag.palette_subtype);
                events_block_draw(r, font, tex,
                                  (EventsBlockType)state.drag.palette_subtype,
                                  gx, gy,
                                  def.opt,
                                  true, bg, -1);
            }
            else if (state.drag.palette_kind == BK_SENSING)
            {
                BlockInstance def = workspace_make_default_sensing((SensingBlockType)state.drag.palette_subtype);
                SensingBlockType sbt = (SensingBlockType)state.drag.palette_subtype;
                if (sbt == SENSB_TOUCHING || sbt == SENSB_KEY_PRESSED ||
                    sbt == SENSB_MOUSE_DOWN)
                {
                    sensing_boolean_block_draw(r, font,
                                               sbt,
                                               gx, gy,
                                               def.opt, def.a, def.b, def.c,
                                               def.d, def.e, def.f,
                                               true, bg, -1,
                                               nullptr);
                }
                else
                {
                    sensing_stack_block_draw(r, font,
                                             sbt,
                                             gx, gy,
                                             def.text, def.opt,
                                             true, bg, -1,
                                             nullptr);
                }
            }
        }
        else
        {
            const BlockInstance *root = workspace_find_const(state, state.drag.dragged_block_id);
            if (!root)
                return;

            if (root->kind == BK_MOTION)
            {
                motion_block_draw(r, font, tex,
                                  (MotionBlockType)root->subtype,
                                  gx, gy,
                                  root->a, root->b, (GoToTarget)root->opt,
                                  true, bg, -1,
                                  nullptr, nullptr);
            }
            else if (root->kind == BK_LOOKS)
            {
                looks_block_draw(r, font,
                                 (LooksBlockType)root->subtype,
                                 gx, gy,
                                 root->text,
                                 root->a, root->b, root->opt,
                                 true, bg, -1,
                                 nullptr, nullptr);
            }
            else if (root->kind == BK_SOUND)
            {
                sound_block_draw(r, font,
                                 (SoundBlockType)root->subtype,
                                 gx, gy,
                                 root->a, root->opt,
                                 true, bg, -1,
                                 nullptr);
            }
            else if (root->kind == BK_EVENTS)
            {
                events_block_draw(r, font, tex,
                                  (EventsBlockType)root->subtype,
                                  gx, gy,
                                  root->opt,
                                  true, bg, -1);
            }
            else if (root->kind == BK_SENSING)
            {
                SensingBlockType sbt = (SensingBlockType)root->subtype;
                if (sbt == SENSB_TOUCHING || sbt == SENSB_KEY_PRESSED ||
                    sbt == SENSB_MOUSE_DOWN)
                {
                    sensing_boolean_block_draw(r, font,
                                               sbt,
                                               gx, gy,
                                               root->opt, root->a, root->b, root->c,
                                               root->d, root->e, root->f,
                                               true, bg, -1,
                                               nullptr);
                }
                else
                {
                    sensing_stack_block_draw(r, font,
                                             sbt,
                                             gx, gy,
                                             root->text, root->opt,
                                             true, bg, -1,
                                             nullptr);
                }
            }

            MotionBlockMetrics m = motion_block_metrics();
            int cur = root->next_id;
            int y = gy + (block_height(*root) - m.overlap);

            while (cur != -1)
            {
                const BlockInstance *b = workspace_find_const(state, cur);
                if (!b)
                    break;

                BlockInstance tmp = *b;
                tmp.x = gx;
                tmp.y = y;

                if (tmp.kind == BK_MOTION)
                {
                    motion_block_draw(r, font, tex,
                                      (MotionBlockType)tmp.subtype,
                                      tmp.x, tmp.y,
                                      tmp.a, tmp.b, (GoToTarget)tmp.opt,
                                      true, bg, -1,
                                      nullptr, nullptr);
                }
                else if (tmp.kind == BK_LOOKS)
                {
                    looks_block_draw(r, font,
                                     (LooksBlockType)tmp.subtype,
                                     tmp.x, tmp.y,
                                     tmp.text,
                                     tmp.a, tmp.b, tmp.opt,
                                     true, bg, -1,
                                     nullptr, nullptr);
                }
                else if (tmp.kind == BK_SOUND)
                {
                    sound_block_draw(r, font,
                                     (SoundBlockType)tmp.subtype,
                                     tmp.x, tmp.y,
                                     tmp.a, tmp.opt,
                                     true, bg, -1,
                                     nullptr);
                }
                else if (tmp.kind == BK_EVENTS)
                {
                    events_block_draw(r, font, tex,
                                      (EventsBlockType)tmp.subtype,
                                      tmp.x, tmp.y,
                                      tmp.opt,
                                      true, bg, -1);
                }
                else if (tmp.kind == BK_SENSING)
                {
                    SensingBlockType sbt = (SensingBlockType)tmp.subtype;
                    if (sbt == SENSB_TOUCHING || sbt == SENSB_KEY_PRESSED ||
                        sbt == SENSB_MOUSE_DOWN)
                    {
                        sensing_boolean_block_draw(r, font,
                                                   sbt,
                                                   tmp.x, tmp.y,
                                                   tmp.opt, tmp.a, tmp.b, tmp.c,
                                                   tmp.d, tmp.e, tmp.f,
                                                   true, bg, -1,
                                                   nullptr);
                    }
                    else
                    {
                        sensing_stack_block_draw(r, font,
                                                 sbt,
                                                 tmp.x, tmp.y,
                                                 tmp.text, tmp.opt,
                                                 true, bg, -1,
                                                 nullptr);
                    }
                }

                y += (block_height(tmp) - m.overlap);
                cur = b->next_id;
            }
        }
    }
}

/* ---------------- drag start/finish ---------------- */

static void start_drag_from_workspace(AppState &state, int clicked_id, int mx, int my)
{
    BlockInstance *start = workspace_find(state, clicked_id);
    if (!start)
        return;

    int drag_root = detach_subchain(state, clicked_id);
    if (drag_root == -1)
        return;

    state.drag.active = true;
    state.drag.from_palette = false;
    state.drag.dragged_block_id = drag_root;

    state.drag.off_x = mx - start->x;
    state.drag.off_y = my - start->y;

    state.drag.ghost_x = start->x;
    state.drag.ghost_y = start->y;

    compute_snap(state);
}

static void finish_drag(AppState &state)
{
    if (!state.drag.active)
        return;

    int place_x = state.drag.snap_valid ? state.drag.snap_x : state.drag.ghost_x;
    int place_y = state.drag.snap_valid ? state.drag.snap_y : state.drag.ghost_y;

    int new_root_id = -1;

    if (state.drag.from_palette)
    {
        BlockInstance b;
        if (state.drag.palette_kind == BK_MOTION)
        {
            b = workspace_make_default((MotionBlockType)state.drag.palette_subtype);
        }
        else if (state.drag.palette_kind == BK_LOOKS)
        {
            b = workspace_make_default_looks((LooksBlockType)state.drag.palette_subtype);
        }
        else if (state.drag.palette_kind == BK_SOUND)
        {
            b = workspace_make_default_sound((SoundBlockType)state.drag.palette_subtype);
        }
        else if (state.drag.palette_kind == BK_EVENTS)
        {
            b = workspace_make_default_events((EventsBlockType)state.drag.palette_subtype);
        }

        else if (state.drag.palette_kind == BK_SENSING)
        {
            b = workspace_make_default_sensing((SensingBlockType)state.drag.palette_subtype);
        }

        b.x = place_x;
        b.y = place_y;

        new_root_id = workspace_add_top_level(state, b);
    }
    else
    {
        BlockInstance *root = workspace_find(state, state.drag.dragged_block_id);
        if (root)
        {
            root->x = place_x;
            root->y = place_y;
            new_root_id = root->id;
            state.top_level_blocks.push_back(new_root_id);
        }
    }

    if (state.drag.snap_valid && new_root_id != -1)
    {

        if (state.drag.snap_above)
        {
            insert_chain_before(state, state.drag.snap_target_id, new_root_id);
            int root_chain = workspace_root_id(state, new_root_id);
            workspace_layout_chain(state, root_chain);
        }
        else
        {
            attach_chain(state, state.drag.snap_target_id, new_root_id);
            int root_chain = workspace_root_id(state, state.drag.snap_target_id);
            workspace_layout_chain(state, root_chain);
        }
    }
    else if (new_root_id != -1)
    {
        workspace_layout_chain(state, new_root_id);
    }

    state.drag.active = false;
    state.drag.snap_valid = false;
    state.drag.snap_target_id = -1;
    state.drag.dragged_block_id = -1;
    state.drag.snap_above = false;
}

/* ---------------- input typing ---------------- */

static BlockFieldType block_field_type(const BlockInstance &b, int field_index)
{
    (void)field_index;

    if (b.kind == BK_MOTION)
        return BFT_INT;
    if (b.kind == BK_SOUND)
        return BFT_INT;
    if (b.kind == BK_SENSING)
    {
        SensingBlockType sbt = (SensingBlockType)b.subtype;
        if (sbt == SENSB_ASK_AND_WAIT)
            return BFT_TEXT; // ask and wait متن می‌گیرد
        return BFT_INT;      // بقیه عدد (مثل R, G, B)
    }
    LooksBlockType t = (LooksBlockType)b.subtype;
    switch (t)
    {
    case LB_SAY_FOR:
    case LB_THINK_FOR:
        return (field_index == 0) ? BFT_TEXT : BFT_INT;
    case LB_SAY:
    case LB_THINK:
        return BFT_TEXT;
    default:
        return BFT_INT;
    }
}

void workspace_commit_active_input(AppState &state)
{
    if (state.active_input != INPUT_BLOCK_FIELD)
        return;

    BlockInstance *b = workspace_find(state, state.block_input.block_id);
    if (!b)
        return;

    int fi = state.block_input.field_index;

    if (state.block_input.type == BFT_TEXT)
    {
        b->text = state.input_buffer;
        return;
    }

    int v = std::atoi(state.input_buffer.c_str());

    if (b->kind == BK_SOUND)
    {
        SoundBlockType t = (SoundBlockType)b->subtype;

        /* Clamp input values per your request */
        if (t == SB_SET_VOLUME_TO)
        {
            if (v < 0)
                v = 0;
            if (v > 100)
                v = 100;
        }
        else if (t == SB_CHANGE_VOLUME_BY)
        {
            if (v < -100)
                v = -100;
            if (v > 100)
                v = 100;
        }

        b->a = v;
        return;
    }
    if (b->kind == BK_SENSING)
    {
        SensingBlockType sbt = (SensingBlockType)b->subtype;

        if (sbt == SENSB_ASK_AND_WAIT)
        {
            b->text = state.input_buffer;
            return;
        }

        int v = std::atoi(state.input_buffer.c_str());

        return;
    }

    if (b->kind == BK_MOTION)
    {
        MotionBlockType t = (MotionBlockType)b->subtype;
        if (t == MB_GO_TO_XY)
        {
            if (fi == 0)
                b->a = v;
            if (fi == 1)
                b->b = v;
        }
        else
        {
            b->a = v;
        }
        return;
    }

    LooksBlockType t = (LooksBlockType)b->subtype;
    if ((t == LB_SAY_FOR || t == LB_THINK_FOR) && fi == 1)
        b->a = v;
    else
        b->a = v;
}

/* ---------------- workspace events ---------------- */

bool workspace_handle_event(const SDL_Event &e, AppState &state,
                            const SDL_Rect &workspace_rect,
                            const SDL_Rect &palette_rect,
                            TTF_Font *font)
{
    (void)palette_rect;

    // Delete while dragging: delete dragged chain (if from workspace), or cancel drag (if from palette)
    if (e.type == SDL_KEYDOWN && state.drag.active)
    {
        if (e.key.keysym.sym == SDLK_DELETE || e.key.keysym.sym == SDLK_BACKSPACE)
        {

            if (!state.drag.from_palette)
            {
                // dragged_block_id is already detached from workspace/top-level
                int rid = state.drag.dragged_block_id;
                if (rid != -1)
                    delete_chain(state, rid);
            }

            // reset drag state
            state.drag.active = false;
            state.drag.snap_valid = false;
            state.drag.snap_target_id = -1;
            state.drag.dragged_block_id = -1;
            return true;
        }
    }

    if (e.type == SDL_MOUSEMOTION && state.drag.active)
    {
        int mx = e.motion.x;
        int my = e.motion.y;

        state.drag.mouse_x = mx;
        state.drag.mouse_y = my;

        state.drag.ghost_x = mx - state.drag.off_x;
        state.drag.ghost_y = my - state.drag.off_y;
        compute_snap(state);
        return true;
    }

    if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
    {
        if (state.drag.active)
        {
            finish_drag(state);
            return true;
        }
    }

    /* typing into active block field */
    if (state.active_input == INPUT_BLOCK_FIELD)
    {
        if (e.type == SDL_TEXTINPUT)
        {
            if (state.block_input.type == BFT_INT)
            {
                for (int i = 0; e.text.text[i]; ++i)
                {
                    char c = e.text.text[i];
                    if ((c >= '0' && c <= '9') || (c == '-' && state.input_buffer.empty()))
                        state.input_buffer.push_back(c);
                }
            }
            else
            {
                state.input_buffer += e.text.text;
            }
            return true;
        }
        if (e.type == SDL_KEYDOWN)
        {
            if (e.key.keysym.sym == SDLK_BACKSPACE)
            {
                if (!state.input_buffer.empty())
                    state.input_buffer.pop_back();
                return true;
            }
            if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER)
            {
                workspace_commit_active_input(state);
                state.active_input = INPUT_NONE;
                state.input_buffer.clear();
                return true;
            }
            if (e.key.keysym.sym == SDLK_ESCAPE)
            {
                state.active_input = INPUT_NONE;
                state.input_buffer.clear();
                return true;
            }
        }
    }

    if (e.type != SDL_MOUSEBUTTONDOWN || e.button.button != SDL_BUTTON_LEFT)
        return false;

    int mx = e.button.x;
    int my = e.button.y;

    if (!point_in_rect(mx, my, workspace_rect))
        return false;

    /* hit test: find topmost block by walking in draw order */
    int hit_id = -1;
    for (int i = (int)state.top_level_blocks.size() - 1; i >= 0 && hit_id == -1; --i)
    {
        int cur = state.top_level_blocks[i];
        while (cur != -1)
        {
            const BlockInstance *b = workspace_find_const(state, cur);
            if (!b)
                break;
            SDL_Rect br = block_rect(*b);
            if (point_in_rect(mx, my, br))
            {
                hit_id = cur;
                break;
            }
            cur = b->next_id;
        }
    }

    if (hit_id != -1)
    {
        BlockInstance *b = workspace_find(state, hit_id);
        if (!b)
            return true;

        int field = -1;

        if (b->kind == BK_MOTION)
        {
            field = motion_block_hittest_field(font,
                                               (MotionBlockType)b->subtype,
                                               b->x, b->y,
                                               b->a, b->b, (GoToTarget)b->opt,
                                               mx, my);
        }
        else if (b->kind == BK_LOOKS)
        {
            field = looks_block_hittest_field(font,
                                              (LooksBlockType)b->subtype,
                                              b->x, b->y,
                                              b->text,
                                              b->a, b->b, b->opt,
                                              mx, my);
        }
        else if (b->kind == BK_EVENTS)
        {
            field = events_block_hittest_field(font,
                                               (EventsBlockType)b->subtype,
                                               b->x, b->y, b->opt,
                                               mx, my);
        }
        else if (b->kind == BK_SOUND)
        {
            field = sound_block_hittest_field(font,
                                              (SoundBlockType)b->subtype,
                                              b->x, b->y,
                                              b->a, b->opt,
                                              mx, my);
        }
        else if (b->kind == BK_SENSING)
        {
            SensingBlockType sbt = (SensingBlockType)b->subtype;
            if (sbt == SENSB_TOUCHING || sbt == SENSB_KEY_PRESSED ||
                sbt == SENSB_MOUSE_DOWN)
            {
                field = sensing_boolean_block_hittest_field(font,
                                                            sbt,
                                                            b->x, b->y,
                                                            b->opt, b->a, b->b, b->c,
                                                            b->d, b->e, b->f,
                                                            mx, my);
            }
            else
            {
                field = sensing_stack_block_hittest_field(font,
                                                          sbt,
                                                          b->x, b->y,
                                                          b->text, b->opt,
                                                          mx, my);
            }
        }

        /* input click */
        if (field == 0 || field == 1)
        {
            state.active_input = INPUT_BLOCK_FIELD;
            state.block_input.block_id = b->id;
            state.block_input.field_index = field;
            state.block_input.type = block_field_type(*b, field);

            if (state.block_input.type == BFT_INT)
            {
                int val = 0;
                if (b->kind == BK_MOTION)
                {
                    MotionBlockType t = (MotionBlockType)b->subtype;
                    val = (t == MB_GO_TO_XY) ? ((field == 0) ? b->a : b->b) : b->a;
                }
                else if (b->kind == BK_LOOKS)
                {
                    LooksBlockType t = (LooksBlockType)b->subtype;
                    if ((t == LB_SAY_FOR || t == LB_THINK_FOR) && field == 1)
                        val = b->a;
                    else
                        val = b->a;
                }
                else
                {
                    val = b->a;
                }
                state.input_buffer = std::to_string(val);
            }
            else
            {
                state.input_buffer = b->text;
            }
            return true;
        }

        /* dropdown click */
        if (field == -2)
        {
            if (b->kind == BK_MOTION)
            {
                if ((MotionBlockType)b->subtype == MB_GO_TO_TARGET)
                    b->opt = (b->opt == (int)TARGET_RANDOM_POSITION) ? (int)TARGET_MOUSE_POINTER
                                                                     : (int)TARGET_RANDOM_POSITION;
                return true;
            }
            else if (b->kind == BK_LOOKS)
            {
                LooksBlockType t = (LooksBlockType)b->subtype;
                if (t == LB_SWITCH_COSTUME_TO)
                    b->opt = (b->opt + 1) % 3;
                else if (t == LB_SWITCH_BACKDROP_TO)
                    b->opt = (b->opt + 1) % 3;
                else if (t == LB_GO_TO_LAYER)
                    b->opt = (b->opt + 1) % 2;
                else if (t == LB_GO_LAYERS)
                    b->opt = (b->opt + 1) % 2;
                return true;
            }
            else if (b->kind == BK_EVENTS)
            {
                EventsBlockType t = (EventsBlockType)b->subtype;

                if (t == EB_WHEN_KEY_PRESSED)
                {
                    // cycle: space(0) .. right(4) .. a..z(5..30) .. 0..9(31..40)
                    b->opt++;
                    if (b->opt > 40)
                        b->opt = 0;
                    return true;
                }

                if (t == EB_WHEN_I_RECEIVE || t == EB_BROADCAST)
                {
                    // فعلاً فقط message1 داری، پس همین رو ثابت نگه دار
                    b->opt = 0;
                    return true;
                }

                return true;
            }
            else if (b->kind == BK_SENSING)
            {
                SensingBlockType sbt = (SensingBlockType)b->subtype;

                if (sbt == SENSB_TOUCHING)
                {
                    b->opt = (b->opt + 1) % 3; // cycle: mouse-pointer(0), edge(1), sprite(2)
                    return true;
                }
                if (sbt == SENSB_KEY_PRESSED)
                {
                    // cycle: space(0), a..z(1..26), 0..9(27..36)
                    b->opt++;
                    if (b->opt > 36)
                        b->opt = 0;
                    return true;
                }
                if (sbt == SENSB_SET_DRAG_MODE)
                {
                    b->opt = (b->opt + 1) % 2; // draggable(0), not draggable(1)
                    return true;
                }
                return true;
            }
            else
            {
                /* only Meow for now */
                b->opt = 0;
                return true;
            }
        }

        /* commit current edit if click elsewhere on block */
        if (state.active_input == INPUT_BLOCK_FIELD)
        {
            workspace_commit_active_input(state);
            state.active_input = INPUT_NONE;
            state.input_buffer.clear();
        }

        start_drag_from_workspace(state, b->id, mx, my);
        compute_snap(state);
        return true;
    }

    return false;
}

bool workspace_save_txt(const AppState & /*state*/, const char * /*path*/) { return false; }
bool workspace_load_txt(AppState & /*state*/, const char * /*path*/) { return false; }