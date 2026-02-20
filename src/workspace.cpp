#include "workspace.h"
#include "block_ui.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <unordered_set>

/* ── helper: is this a Sensing Boolean (hexagonal) block? ── */
static bool is_boolean_block(BlockKind kind, int subtype)
{
    if (kind != BK_SENSING)
        return false;
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

/* --- Forward Declarations for AST --- */
static SDL_Rect block_rect(const AppState &state, const BlockInstance &b)
{
    if (b.kind == BK_CONTROL)
    {
        ControlBlockType ct = (ControlBlockType)b.subtype;
        int inner1_h = chain_height(state, b.child_id);
        int inner2_h = chain_height(state, b.child2_id);
        return control_block_rect(ct, b.x, b.y, inner1_h, inner2_h);
    }
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
        if (sbt == SENSB_TOUCHING || sbt == SENSB_KEY_PRESSED || sbt == SENSB_MOUSE_DOWN)
        {
            return sensing_boolean_block_rect(sbt, b.x, b.y, b.opt);
        }
        else
        {
            return sensing_stack_block_rect(sbt, b.x, b.y, b.opt);
        }
    }
    return {b.x, b.y, 240, 40};
}

static int block_height(const AppState &state, const BlockInstance &b)
{
    return block_rect(state, b).h;
}

static int chain_height(const AppState &state, int root_id)
{
    int h = 0;
    int cur = root_id;
    MotionBlockMetrics m = motion_block_metrics();
    while (cur != -1)
    {
        const BlockInstance *b = workspace_find_const(state, cur);
        if (!b)
            break;
        h += block_height(state, *b);
        cur = b->next_id;
        if (cur != -1)
            h -= m.overlap;
    }
    return h;
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

        // Align Boolean inside Hex slot
        if (b->condition_id != -1)
        {
            BlockInstance *cond = workspace_find(state, b->condition_id);
            if (cond)
            {
                cond->x = x + 35;
                cond->y = y + 8;
                workspace_layout_chain(state, cond->id);
            }
        }
        // Align inside Top Mouth
        if (b->child_id != -1)
        {
            BlockInstance *child = workspace_find(state, b->child_id);
            if (child)
            {
                child->x = x + 16;
                child->y = y + 40 - m.overlap;
                workspace_layout_chain(state, child->id);
            }
        }
        // Align inside Else Mouth
        if (b->child2_id != -1)
        {
            BlockInstance *child2 = workspace_find(state, b->child2_id);
            if (child2)
            {
                int h1 = std::max(24, chain_height(state, b->child_id));
                child2->x = x + 16;
                child2->y = y + 40 + h1 + 32 - m.overlap;
                workspace_layout_chain(state, child2->id);
            }
        }

        y += (block_height(state, *b) - m.overlap);
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
            if (p->next_id == id)
                p->next_id = -1;
            else if (p->child_id == id)
                p->child_id = -1;
            else if (p->child2_id == id)
                p->child2_id = -1;
            else if (p->condition_id == id)
                p->condition_id = -1;
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
        state.drag.snap_valid = false; // boolean → ❌ no stack snap
    else
        state.drag.snap_valid = true; // stack → ✅ allow
}

/* ---------------- snap compute ---------------- */

static void compute_snap(AppState &state)
{
    state.drag.snap_above = false;
    state.drag.snap_valid = false;
    state.drag.snap_target_id = -1;
    state.drag.snap_type = SNAP_NONE;
    if (!state.drag.active)
        return;
    const int SNAP_DIST = 20;

    SDL_Rect dr = {state.drag.ghost_x, state.drag.ghost_y, 200, 40};

    // --- NEW BULLETPROOF BOOLEAN CHECK ---
    bool dragging_bool = false;
    auto is_bool = [](BlockKind k, int sub)
    {
        return k == BK_SENSING && (sub == SENSB_TOUCHING || sub == SENSB_KEY_PRESSED || sub == SENSB_MOUSE_DOWN);
    };

    if (state.drag.from_palette)
    {
        dragging_bool = is_bool(state.drag.palette_kind, state.drag.palette_subtype);
    }
    else
    {
        const BlockInstance *rb = workspace_find_const(state, state.drag.dragged_block_id);
        if (rb)
        {
            dr = block_rect(state, *rb);
            dr.x = state.drag.ghost_x;
            dr.y = state.drag.ghost_y;
            dragging_bool = is_bool(rb->kind, rb->subtype);
        }
    }

    int best_id = -1, best_dx = 0, best_dy = 0, best_dist = 999999;
    SnapType best_type = SNAP_NONE;

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

    auto check_block_snaps = [&](auto &self, int root_id) -> void
    {
        int cur = root_id;
        while (cur != -1)
        {
            if (dragged_ids.count(cur))
                return;
            const BlockInstance *b = workspace_find_const(state, cur);
            if (!b)
                break;

            SDL_Rect br = block_rect(state, *b);
            auto try_snap = [&](int tx, int ty, SnapType st, bool is_hex_slot)
            {
                if (dragging_bool != is_hex_slot)
                    return;
                int dx = tx - dr.x, dy = ty - dr.y;
                if (std::abs(dx) <= SNAP_DIST && std::abs(dy) <= SNAP_DIST)
                {
                    int dist = std::abs(dx) + std::abs(dy);
                    if (dist < best_dist)
                    {
                        best_dist = dist;
                        best_id = cur;
                        best_dx = dx;
                        best_dy = dy;
                        best_type = st;
                    }
                }
            };

            // Top and Bottom Notches
            try_snap(br.x, br.y + br.h - motion_block_metrics().overlap, SNAP_AFTER, false);
            try_snap(br.x, br.y - (dr.h - motion_block_metrics().overlap), SNAP_BEFORE, false);

            // C-Shapes & Hex Slots
            if (b->kind == BK_CONTROL)
            {
                ControlBlockType ct = (ControlBlockType)b->subtype;
                if (ct == CB_IF || ct == CB_IF_ELSE || ct == CB_WAIT_UNTIL)
                    try_snap(br.x + 35, br.y + 8, SNAP_CONDITION, true);
                if (ct == CB_REPEAT || ct == CB_FOREVER || ct == CB_IF || ct == CB_IF_ELSE)
                    try_snap(br.x + 16, br.y + 40 - motion_block_metrics().overlap, SNAP_INSIDE_1, false);
                if (ct == CB_IF_ELSE)
                    try_snap(br.x + 16, br.y + 40 + std::max(24, chain_height(state, b->child_id)) + 32 - motion_block_metrics().overlap, SNAP_INSIDE_2, false);
            }

            if (b->child_id != -1)
                self(self, b->child_id);
            if (b->child2_id != -1)
                self(self, b->child2_id);
            if (b->condition_id != -1)
                self(self, b->condition_id);

            cur = b->next_id;
        }
    };

    for (int root_id : state.top_level_blocks)
        check_block_snaps(check_block_snaps, root_id);

    if (best_id != -1)
    {
        state.drag.snap_valid = true;
        state.drag.snap_target_id = best_id;
        state.drag.snap_type = best_type;
        state.drag.snap_x = dr.x + best_dx;
        state.drag.snap_y = dr.y + best_dy;
        state.drag.snap_above = (best_type == SNAP_BEFORE);
    }
}
/* ---------------- draw ---------------- */

static void draw_chain(SDL_Renderer *r, TTF_Font *font, const Textures &tex,
                       const AppState &state, Color bg, int root_id, bool ghost, int off_x, int off_y)
{
    int cur = root_id;
    while (cur != -1)
    {
        const BlockInstance *b = workspace_find_const(state, cur);
        if (!b)
            break;
        int bx = b->x + off_x;
        int by = b->y + off_y;

        int sel = -1;
        const char *ov0 = nullptr, *ov1 = nullptr;
        if (!ghost && state.active_input == INPUT_BLOCK_FIELD && state.block_input.block_id == b->id)
        {
            sel = state.block_input.field_index;
            if (state.block_input.type == BFT_INT)
            {
                if (sel == 0)
                    ov0 = state.input_buffer.c_str();
                if (sel == 1)
                    ov1 = state.input_buffer.c_str();
            }
            else if (sel == 0)
                ov0 = state.input_buffer.c_str();
        }

        if (b->kind == BK_MOTION)
            motion_block_draw(r, font, tex, (MotionBlockType)b->subtype, bx, by, b->a, b->b, (GoToTarget)b->opt, ghost, bg, sel, ov0, ov1);
        else if (b->kind == BK_LOOKS)
            looks_block_draw(r, font, (LooksBlockType)b->subtype, bx, by, b->text, b->a, b->b, b->opt, ghost, bg, sel, ov0, ov1);
        else if (b->kind == BK_SOUND)
            sound_block_draw(r, font, (SoundBlockType)b->subtype, bx, by, b->a, b->opt, ghost, bg, sel, ov0);
        else if (b->kind == BK_EVENTS)
            events_block_draw(r, font, tex, (EventsBlockType)b->subtype, bx, by, b->opt, ghost, bg, -1);
        else if (b->kind == BK_SENSING)
        {
            SensingBlockType sbt = (SensingBlockType)b->subtype;
            if (sbt == SENSB_TOUCHING || sbt == SENSB_KEY_PRESSED || sbt == SENSB_MOUSE_DOWN)
                sensing_boolean_block_draw(r, font, sbt, bx, by, b->opt, b->a, b->b, b->c, b->d, b->e, b->f, ghost, bg, sel, ov0);
            else
                sensing_stack_block_draw(r, font, sbt, bx, by, b->text, b->opt, ghost, bg, sel, ov0);
        }
        else if (b->kind == BK_CONTROL)
        {
            int h1 = chain_height(state, b->child_id);
            int h2 = chain_height(state, b->child2_id);
            bool has_cond = (b->condition_id != -1);
            control_block_draw(r, font, (ControlBlockType)b->subtype, bx, by, h1, h2, b->a, has_cond, ghost, bg, sel, ov0);
        }

        // Recursively draw children inside C-shapes
        if (b->condition_id != -1)
            draw_chain(r, font, tex, state, bg, b->condition_id, ghost, off_x, off_y);
        if (b->child_id != -1)
            draw_chain(r, font, tex, state, bg, b->child_id, ghost, off_x, off_y);
        if (b->child2_id != -1)
            draw_chain(r, font, tex, state, bg, b->child2_id, ghost, off_x, off_y);

        cur = b->next_id;
    }
}

void workspace_draw(SDL_Renderer *r, TTF_Font *font, const Textures &tex,
                    const AppState &state, const SDL_Rect &workspace_rect, Color bg)
{
    (void)workspace_rect;

    // Draw real blocks
    for (int root_id : state.top_level_blocks)
    {
        draw_chain(r, font, tex, state, bg, root_id, false, 0, 0);
    }

    // Draw dragged shadow and ghost
    if (state.drag.active)
    {
        // Draw shadow slot
        if (state.drag.snap_valid)
        {
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(r, 0, 0, 0, 60);
            SDL_Rect sr = {state.drag.snap_x, state.drag.snap_y, 200, 40};
            if (state.drag.from_palette)
                sr.w = 160;
            else
            {
                BlockInstance *drb = workspace_find((AppState &)state, state.drag.dragged_block_id);
                if (drb)
                    sr = block_rect(state, *drb);
            }
            sr.x += 2;
            sr.y += 2;
            SDL_RenderFillRect(r, &sr);
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        }

        // Draw Ghost
        int dx = state.drag.snap_valid ? (state.drag.snap_x - state.drag.ghost_x) : 0;
        int dy = state.drag.snap_valid ? (state.drag.snap_y - state.drag.ghost_y) : 0;

        if (state.drag.from_palette)
        {
            BlockInstance def;
            if (state.drag.palette_kind == BK_MOTION)
                def = workspace_make_default((MotionBlockType)state.drag.palette_subtype);
            else if (state.drag.palette_kind == BK_LOOKS)
                def = workspace_make_default_looks((LooksBlockType)state.drag.palette_subtype);
            else if (state.drag.palette_kind == BK_SOUND)
                def = workspace_make_default_sound((SoundBlockType)state.drag.palette_subtype);
            else if (state.drag.palette_kind == BK_EVENTS)
                def = workspace_make_default_events((EventsBlockType)state.drag.palette_subtype);
            else if (state.drag.palette_kind == BK_SENSING)
                def = workspace_make_default_sensing((SensingBlockType)state.drag.palette_subtype);
            else
            {
                def.kind = BK_CONTROL;
                def.subtype = state.drag.palette_subtype;
            }

            def.id = 9999;
            def.x = state.drag.ghost_x;
            def.y = state.drag.ghost_y;
            ((AppState &)state).blocks.push_back(def);
            draw_chain(r, font, tex, state, bg, 9999, true, dx, dy);
            ((AppState &)state).blocks.pop_back();
        }
        else
        {
            BlockInstance *drb = workspace_find((AppState &)state, state.drag.dragged_block_id);
            int real_dx = state.drag.ghost_x - drb->x + dx;
            int real_dy = state.drag.ghost_y - drb->y + dy;
            draw_chain(r, font, tex, state, bg, state.drag.dragged_block_id, true, real_dx, real_dy);
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

    int new_root_id = -1;
    if (state.drag.from_palette)
    {
        BlockInstance b;
        if (state.drag.palette_kind == BK_MOTION)
            b = workspace_make_default((MotionBlockType)state.drag.palette_subtype);
        else if (state.drag.palette_kind == BK_LOOKS)
            b = workspace_make_default_looks((LooksBlockType)state.drag.palette_subtype);
        else if (state.drag.palette_kind == BK_SOUND)
            b = workspace_make_default_sound((SoundBlockType)state.drag.palette_subtype);
        else if (state.drag.palette_kind == BK_EVENTS)
            b = workspace_make_default_events((EventsBlockType)state.drag.palette_subtype);
        else if (state.drag.palette_kind == BK_SENSING)
            b = workspace_make_default_sensing((SensingBlockType)state.drag.palette_subtype);
        else
        {
            b.kind = BK_CONTROL;
            b.subtype = state.drag.palette_subtype;
            if (b.subtype == CB_WAIT)
                b.a = 1;
            else
                b.a = 10;
        }

        b.x = state.drag.snap_valid ? state.drag.snap_x : state.drag.ghost_x;
        b.y = state.drag.snap_valid ? state.drag.snap_y : state.drag.ghost_y;
        new_root_id = workspace_add_top_level(state, b);
    }
    else
    {
        BlockInstance *root = workspace_find(state, state.drag.dragged_block_id);
        if (root)
        {
            root->x = state.drag.snap_valid ? state.drag.snap_x : state.drag.ghost_x;
            root->y = state.drag.snap_valid ? state.drag.snap_y : state.drag.ghost_y;
            new_root_id = root->id;
            state.top_level_blocks.push_back(new_root_id);
        }
    }

    if (state.drag.snap_valid && new_root_id != -1)
    {
        BlockInstance *target = workspace_find(state, state.drag.snap_target_id);
        
        if (state.drag.snap_type == SNAP_BEFORE)
        {
            insert_chain_before(state, state.drag.snap_target_id, new_root_id);
        }
        else if (state.drag.snap_type == SNAP_AFTER)
        {
            attach_chain(state, state.drag.snap_target_id, new_root_id);
            remove_from_top_level(state, new_root_id);
        }
        // --- FIXED: Boolean Hexagon Snapping ---
        else if (state.drag.snap_type == SNAP_CONDITION && target)
        {
            target->condition_id = new_root_id; // Added this missing line back!
            BlockInstance *nr = workspace_find(state, new_root_id);
            if (nr)
                nr->parent_id = target->id;
            remove_from_top_level(state, new_root_id);
        }
        // --- FIXED: C-Shape Nesting ---
        else if ((state.drag.snap_type == SNAP_INSIDE_1 || state.drag.snap_type == SNAP_INSIDE_2) && target)
        {
            int old_child = (state.drag.snap_type == SNAP_INSIDE_1) ? target->child_id : target->child2_id;
            
            if (state.drag.snap_type == SNAP_INSIDE_1)
                target->child_id = new_root_id;
            else
                target->child2_id = new_root_id;

            BlockInstance *nr = workspace_find(state, new_root_id);
            if (nr)
                nr->parent_id = target->id;
            remove_from_top_level(state, new_root_id);
            
            if (old_child != -1)
                attach_chain(state, new_root_id, old_child);
        }
        
        workspace_layout_chain(state, workspace_root_id(state, new_root_id));
    }
    else if (new_root_id != -1)
    {
        workspace_layout_chain(state, new_root_id);
    }

    for (int tl_id : state.top_level_blocks)
    {
        workspace_layout_chain(state, tl_id);
    }

    state.drag.active = false;
    state.drag.snap_valid = false;
    state.drag.snap_target_id = -1;
    state.drag.dragged_block_id = -1;
}
/* ---------------- input typing ---------------- */

static BlockFieldType block_field_type(const BlockInstance &b, int field)
{
    if (b.kind == BK_LOOKS && (b.subtype == LB_SAY_FOR || b.subtype == LB_SAY || b.subtype == LB_THINK_FOR || b.subtype == LB_THINK))
    {
        if (field == 0)
            return BFT_TEXT;
    }
    if (b.kind == BK_SENSING && (b.subtype == SENSB_ASK_AND_WAIT))
    {
        if (field == 0)
            return BFT_TEXT;
    }
    return BFT_INT; // Everything else (including BK_CONTROL) is strictly an Integer!
}

void workspace_commit_active_input(AppState &state)
{
    if (state.active_input != INPUT_BLOCK_FIELD)
        return;
    BlockInstance *b = workspace_find(state, state.block_input.block_id);
    if (!b)
        return;

    if (state.block_input.type == BFT_INT)
    {
        int val = 0;
        if (!state.input_buffer.empty() && state.input_buffer != "-")
        {
            val = std::atoi(state.input_buffer.c_str());
        }
        if (state.block_input.field_index == 0)
            b->a = val;
        else if (state.block_input.field_index == 1)
            b->b = val;
        else if (state.block_input.field_index == 2)
            b->c = val;
    }
    else
    {
        if (state.block_input.field_index == 0)
            b->text = state.input_buffer;
    }
}

/* ---------------- workspace events ---------------- */

bool workspace_handle_event(const SDL_Event &e, AppState &state, const SDL_Rect &workspace_rect, const SDL_Rect &palette_rect, TTF_Font *font)
{
    (void)palette_rect;
    if (e.type == SDL_KEYDOWN && state.drag.active)
    {
        if (e.key.keysym.sym == SDLK_DELETE || e.key.keysym.sym == SDLK_BACKSPACE)
        {
            if (!state.drag.from_palette && state.drag.dragged_block_id != -1)
                delete_chain(state, state.drag.dragged_block_id);
            state.drag.active = false;
            state.drag.snap_valid = false;
            state.drag.snap_target_id = -1;
            state.drag.dragged_block_id = -1;
            return true;
        }
    }

    if (e.type == SDL_MOUSEMOTION && state.drag.active)
    {
        state.drag.mouse_x = e.motion.x;
        state.drag.mouse_y = e.motion.y;
        state.drag.ghost_x = e.motion.x - state.drag.off_x;
        state.drag.ghost_y = e.motion.y - state.drag.off_y;
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
                state.input_buffer += e.text.text;
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
            if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER || e.key.keysym.sym == SDLK_ESCAPE)
            {
                workspace_commit_active_input(state);
                state.active_input = INPUT_NONE;
                state.input_buffer.clear();
                return true;
            }
        }
    }

    if (e.type != SDL_MOUSEBUTTONDOWN || e.button.button != SDL_BUTTON_LEFT)
        return false;
    if (!point_in_rect(e.button.x, e.button.y, workspace_rect))
        return false;

    auto find_hit = [&](auto &self, int root_id) -> int
    {
        int cur = root_id;
        while (cur != -1)
        {
            const BlockInstance *b = workspace_find_const(state, cur);
            if (!b)
                break;
            if (b->condition_id != -1)
            {
                int h = self(self, b->condition_id);
                if (h != -1)
                    return h;
            }
            if (b->child_id != -1)
            {
                int h = self(self, b->child_id);
                if (h != -1)
                    return h;
            }
            if (b->child2_id != -1)
            {
                int h = self(self, b->child2_id);
                if (h != -1)
                    return h;
            }
            if (point_in_rect(e.button.x, e.button.y, block_rect(state, *b)))
                return cur;
            cur = b->next_id;
        }
        return -1;
    };

    int hit_id = -1;
    for (int i = (int)state.top_level_blocks.size() - 1; i >= 0 && hit_id == -1; --i)
        hit_id = find_hit(find_hit, state.top_level_blocks[i]);

    if (hit_id != -1)
    {
        BlockInstance *b = workspace_find(state, hit_id);
        if (!b)
            return true;

        int field = -1;
        if (b->kind == BK_MOTION)
            field = motion_block_hittest_field(font, (MotionBlockType)b->subtype, b->x, b->y, b->a, b->b, (GoToTarget)b->opt, e.button.x, e.button.y);
        else if (b->kind == BK_LOOKS)
            field = looks_block_hittest_field(font, (LooksBlockType)b->subtype, b->x, b->y, b->text, b->a, b->b, b->opt, e.button.x, e.button.y);
        else if (b->kind == BK_EVENTS)
            field = events_block_hittest_field(font, (EventsBlockType)b->subtype, b->x, b->y, b->opt, e.button.x, e.button.y);
        else if (b->kind == BK_SOUND)
            field = sound_block_hittest_field(font, (SoundBlockType)b->subtype, b->x, b->y, b->a, b->opt, e.button.x, e.button.y);
        else if (b->kind == BK_CONTROL)
            field = control_block_hittest_field(font, (ControlBlockType)b->subtype, b->x, b->y, chain_height(state, b->child_id), chain_height(state, b->child2_id), b->a, e.button.x, e.button.y);
        else if (b->kind == BK_SENSING)
        {
            SensingBlockType sbt = (SensingBlockType)b->subtype;
            if (sbt == SENSB_TOUCHING || sbt == SENSB_KEY_PRESSED || sbt == SENSB_MOUSE_DOWN)
                field = sensing_boolean_block_hittest_field(font, sbt, b->x, b->y, b->opt, b->a, b->b, b->c, b->d, b->e, b->f, e.button.x, e.button.y);
            else
                field = sensing_stack_block_hittest_field(font, sbt, b->x, b->y, b->text, b->opt, e.button.x, e.button.y);
        }

        // --- NEW BULLETPROOF DROPDOWN & INPUT LOGIC ---
        if (field == -2)
        {
            // IT IS A DROPDOWN! Cycle the options.
            int max_opt = 2; // Default for most dropdowns
            if (b->kind == BK_SENSING && b->subtype == SENSB_TOUCHING) max_opt = 3;
            else if (b->kind == BK_SENSING && b->subtype == SENSB_KEY_PRESSED) max_opt = 37;
            else if (b->kind == BK_EVENTS && b->subtype == EB_WHEN_KEY_PRESSED) max_opt = 41; // 41 keys!
            else if (b->kind == BK_LOOKS && (b->subtype == LB_SWITCH_COSTUME_TO || b->subtype == LB_SWITCH_BACKDROP_TO)) max_opt = 3;
            
            b->opt = (b->opt + 1) % max_opt;

            // Layout again just in case the dropdown text changed the block width
            for (int tl_id : state.top_level_blocks)
                workspace_layout_chain(state, tl_id);
            return true; // STOP HERE! Do not start a drag!
        }
        else if (field >= 0)
        {
            // IT IS A TEXT/NUMBER INPUT! Open the typing caret.
            state.active_input = INPUT_BLOCK_FIELD;
            state.block_input.block_id = b->id;
            state.block_input.field_index = field;
            state.block_input.type = block_field_type(*b, field);
            
            if (state.block_input.type == BFT_TEXT) state.input_buffer = b->text;
            else if (field == 0) state.input_buffer = std::to_string(b->a);
            else if (field == 1) state.input_buffer = std::to_string(b->b);
            else state.input_buffer = std::to_string(b->c);
            
            return true; // STOP HERE! Do not start a drag!
        }

        // Clean up input if clicking the body of the block
        if (state.active_input == INPUT_BLOCK_FIELD)
        {
            workspace_commit_active_input(state);
            state.active_input = INPUT_NONE;
            state.input_buffer.clear();
        }

        start_drag_from_workspace(state, b->id, e.button.x, e.button.y);

        // Shrink C-shapes instantly when a block is pulled out!
        for (int tl_id : state.top_level_blocks)
        {
            workspace_layout_chain(state, tl_id);
        }

        compute_snap(state);
        return true;
    }
    return false;
}

bool workspace_save_txt(const AppState & /*state*/, const char * /*path*/) { return false; }
bool workspace_load_txt(AppState & /*state*/, const char * /*path*/) { return false; }