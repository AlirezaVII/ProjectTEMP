#include "workspace.h"
#include "block_ui.h"
#include "renderer.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <iostream>
#include <unordered_set>

static bool point_in_rect(int px, int py, const SDL_Rect &r) { return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h; }

// ---> FIXED: MAPS WORKSPACE DIRECTLY TO THE SELECTED SPRITE! <---
BlockInstance *workspace_find(AppState &state, int id)
{
    if (state.selected_sprite < 0 || state.selected_sprite >= (int)state.sprites.size())
        return nullptr;
    for (auto &b : state.sprites[state.selected_sprite].blocks)
        if (b.id == id)
            return &b;
    return nullptr;
}
const BlockInstance *workspace_find_const(const AppState &state, int id)
{
    if (state.selected_sprite < 0 || state.selected_sprite >= (int)state.sprites.size())
        return nullptr;
    for (const auto &b : state.sprites[state.selected_sprite].blocks)
        if (b.id == id)
            return &b;
    return nullptr;
}

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

static void get_operator_capsule_widths(const AppState &state, const BlockInstance &b, int &cw0, int &cw1)
{
    auto get_w = [&](int arg_id, const std::string &txt, int min_w, bool is_hex)
    {
        if (arg_id != -1)
        {
            const BlockInstance *child = workspace_find_const(state, arg_id);
            if (child)
                return block_rect(state, *child).w;
        }
        if (is_hex)
            return 50;
        return std::max(min_w, (int)txt.length() * 8 + 16);
    };
    bool hex0 = (b.subtype == OP_AND || b.subtype == OP_OR || b.subtype == OP_NOT);
    bool hex1 = (b.subtype == OP_AND || b.subtype == OP_OR);
    int d0 = 36, d1 = 36;
    if (b.subtype == OP_JOIN)
    {
        d0 = 46;
        d1 = 56;
    }
    if (b.subtype == OP_LETTER_OF)
    {
        d0 = 26;
        d1 = 56;
    }
    if (b.subtype == OP_LENGTH_OF)
    {
        d0 = 56;
    }
    std::string s0 = b.text;
    std::string s1 = b.text2;
    if (s0.empty() && b.a != 0)
        s0 = std::to_string(b.a);
    if (s1.empty() && b.b != 0)
        s1 = std::to_string(b.b);
    if (b.subtype == OP_GT || b.subtype == OP_LT || b.subtype == OP_EQ)
    {
        if (s1.empty() && b.b == 0)
            s1 = "50";
    }
    if (b.subtype == OP_JOIN && s0.empty() && s1.empty())
    {
        s0 = "apple";
        s1 = "banana";
    }
    if (b.subtype == OP_LETTER_OF && s0.empty() && s1.empty())
    {
        s0 = "1";
        s1 = "apple";
    }
    if (b.subtype == OP_LENGTH_OF && s0.empty())
    {
        s0 = "apple";
    }
    cw0 = get_w(b.arg0_id, s0, d0, hex0);
    cw1 = get_w(b.arg1_id, s1, d1, hex1);
}

static SDL_Rect block_rect(const AppState &state, const BlockInstance &b)
{
    if (b.kind == BK_CONTROL)
    {
        return control_block_rect((ControlBlockType)b.subtype, b.x, b.y, chain_height(state, b.child_id), chain_height(state, b.child2_id));
    }
    if (b.kind == BK_MOTION)
        return motion_block_rect((MotionBlockType)b.subtype, b.x, b.y);
    if (b.kind == BK_LOOKS)
        return looks_block_rect((LooksBlockType)b.subtype, b.x, b.y);
    if (b.kind == BK_SOUND)
        return sound_block_rect((SoundBlockType)b.subtype, b.x, b.y, b.a, b.opt);
    if (b.kind == BK_EVENTS)
        return events_block_rect((EventsBlockType)b.subtype, b.x, b.y, b.opt);
    if (b.kind == BK_OPERATORS)
    {
        int cw0, cw1;
        get_operator_capsule_widths(state, b, cw0, cw1);
        int base_w = 140;
        int d0 = 36, d1 = 36;
        switch (b.subtype)
        {
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
            base_w = 140;
            d0 = 36;
            d1 = 36;
            break;
        case OP_GT:
        case OP_LT:
        case OP_EQ:
            base_w = 160;
            d0 = 36;
            d1 = 36;
            break;
        case OP_AND:
        case OP_OR:
            base_w = 190;
            d0 = 50;
            d1 = 50;
            break;
        case OP_NOT:
            base_w = 140;
            d0 = 50;
            d1 = 0;
            break;
        case OP_JOIN:
            base_w = 190;
            d0 = 46;
            d1 = 56;
            break;
        case OP_LETTER_OF:
            base_w = 200;
            d0 = 26;
            d1 = 56;
            break;
        case OP_LENGTH_OF:
            base_w = 160;
            d0 = 56;
            d1 = 0;
            break;
        }
        int extra_w = 0;
        if (cw0 > d0)
            extra_w += (cw0 - d0);
        if (cw1 > d1)
            extra_w += (cw1 - d1);
        return {b.x, b.y, base_w + extra_w, 40};
    }
    if (b.kind == BK_SENSING)
    {
        SensingBlockType sbt = (SensingBlockType)b.subtype;
        if (sbt == SENSB_TOUCHING || sbt == SENSB_KEY_PRESSED || sbt == SENSB_MOUSE_DOWN)
            return sensing_boolean_block_rect(sbt, b.x, b.y, b.opt);
        else if (sbt == SENSB_ANSWER || sbt == SENSB_DISTANCE_TO)
            return sensing_reporter_block_rect(sbt, b.x, b.y);
        else
            return sensing_stack_block_rect(sbt, b.x, b.y, b.opt);
    }
    if (b.kind == BK_VARIABLES)
        return variables_block_rect((VariablesBlockType)b.subtype, b.x, b.y, b.text);
    return {b.x, b.y, 240, 40};
}

static int block_height(const AppState &state, const BlockInstance &b) { return block_rect(state, b).h; }
int chain_height(const AppState &state, int root_id)
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

static SDL_Rect get_capsule_rect(TTF_Font *font, const AppState &state, const BlockInstance &b, int arg_index)
{
    SDL_Rect br = block_rect(state, b);
    int cy = br.y + br.h / 2;
    int hit_start = -1;
    int hit_end = -1;
    for (int px = br.x; px < br.x + br.w + 60; px += 2)
    {
        int field = -1;
        if (b.kind == BK_MOTION)
            field = motion_block_hittest_field(font, (MotionBlockType)b.subtype, b.x, b.y, b.a, b.b, (GoToTarget)b.opt, px, cy);
        else if (b.kind == BK_LOOKS)
            field = looks_block_hittest_field(font, state, (LooksBlockType)b.subtype, b.x, b.y, b.text, b.a, b.b, b.opt, px, cy);
        else if (b.kind == BK_SOUND)
            field = sound_block_hittest_field(font, (SoundBlockType)b.subtype, b.x, b.y, b.a, b.opt, px, cy);
        else if (b.kind == BK_EVENTS)
            field = events_block_hittest_field(font, (EventsBlockType)b.subtype, b.x, b.y, b.opt, px, cy);
        else if (b.kind == BK_CONTROL)
            field = control_block_hittest_field(font, (ControlBlockType)b.subtype, b.x, b.y, chain_height(state, b.child_id), chain_height(state, b.child2_id), b.a, px, cy);
        else if (b.kind == BK_SENSING)
        {
            if (b.subtype == SENSB_TOUCHING || b.subtype == SENSB_KEY_PRESSED || b.subtype == SENSB_MOUSE_DOWN)
                field = sensing_boolean_block_hittest_field(font, (SensingBlockType)b.subtype, b.x, b.y, b.opt, b.a, b.b, b.c, b.d, b.e, b.f, px, cy);
            else if (b.subtype == SENSB_ANSWER || b.subtype == SENSB_DISTANCE_TO)
                field = sensing_reporter_block_hittest_field(font, (SensingBlockType)b.subtype, b.x, b.y, px, cy);
            else
                field = sensing_stack_block_hittest_field(font, (SensingBlockType)b.subtype, b.x, b.y, b.text, b.opt, px, cy);
        }
        else if (b.kind == BK_OPERATORS)
        {
            int cw0, cw1;
            get_operator_capsule_widths(state, b, cw0, cw1);
            int total_w = block_rect(state, b).w;
            field = operators_block_hittest_dynamic(font, (OperatorsBlockType)b.subtype, b.x, b.y, total_w, cw0, cw1, px, cy);
        }
        else if (b.kind == BK_VARIABLES)
        {
            field = variables_block_hittest_field(font, state, (VariablesBlockType)b.subtype, b.x, b.y, b.text, b.opt, px, cy);
        }
        if (field == arg_index)
        {
            if (hit_start == -1)
                hit_start = px;
            hit_end = px;
        }
    }
    if (hit_start != -1)
        return {hit_start, br.y + (br.h - 24) / 2, hit_end - hit_start + 2, 24};
    return {br.x + 20, br.y + 8, 40, 24};
}

BlockInstance workspace_make_default(MotionBlockType type)
{
    BlockInstance b;
    b.kind = BK_MOTION;
    b.subtype = (int)type;
    switch (type)
    {
    case MB_MOVE_STEPS:
    case MB_CHANGE_X_BY:
    case MB_CHANGE_Y_BY:
        b.a = 10;
        break;
    case MB_TURN_RIGHT_DEG:
    case MB_TURN_LEFT_DEG:
        b.a = 15;
        break;
    case MB_POINT_IN_DIR:
        b.a = 90;
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
    case LB_CHANGE_SIZE_BY:
        b.a = 10;
        break;
    case LB_SET_SIZE_TO:
        b.a = 100;
        break;
    case LB_GO_LAYERS:
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
    if (type == SB_CHANGE_VOLUME_BY)
        b.a = -10;
    if (type == SB_SET_VOLUME_TO)
        b.a = 100;
    return b;
}
BlockInstance workspace_make_default_events(EventsBlockType type)
{
    BlockInstance b;
    b.kind = BK_EVENTS;
    b.subtype = (int)type;
    return b;
}
BlockInstance workspace_make_default_sensing(SensingBlockType type)
{
    BlockInstance b;
    b.kind = BK_SENSING;
    b.subtype = (int)type;
    if (type == SENSB_ASK_AND_WAIT)
        b.text = "What's your name?";
    return b;
}
BlockInstance workspace_make_default_operators(OperatorsBlockType type)
{
    BlockInstance b;
    b.kind = BK_OPERATORS;
    b.subtype = (int)type;
    if (type == OP_JOIN)
    {
        b.text = "apple";
        b.text2 = "banana";
    }
    else if (type == OP_LETTER_OF)
    {
        b.text = "1";
        b.text2 = "apple";
    }
    else if (type == OP_LENGTH_OF)
    {
        b.text = "apple";
    }
    else if (type == OP_GT || type == OP_LT || type == OP_EQ)
    {
        b.text2 = "50";
    }
    return b;
}
BlockInstance workspace_make_default_variables(VariablesBlockType type, const std::string &var_name)
{
    BlockInstance b;
    b.kind = BK_VARIABLES;
    b.subtype = (int)type;
    if (type == VB_VARIABLE)
        b.text = var_name;
    if (type == VB_SET)
    {
        b.opt = 0;
        b.text = "0";
    }
    if (type == VB_CHANGE)
    {
        b.opt = 0;
        b.a = 1;
    }
    return b;
}

int workspace_add_top_level(AppState &state, const BlockInstance &b0)
{
    if (state.selected_sprite < 0 || state.selected_sprite >= (int)state.sprites.size())
        return -1;
    BlockInstance b = b0;
    b.id = state.next_block_id++;
    state.sprites[state.selected_sprite].blocks.push_back(b);
    state.sprites[state.selected_sprite].top_level_blocks.push_back(b.id);
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
        if (b->child2_id != -1)
        {
            BlockInstance *child2 = workspace_find(state, b->child2_id);
            if (child2)
            {
                child2->x = x + 16;
                child2->y = y + 40 + std::max(24, chain_height(state, b->child_id)) + 32 - m.overlap;
                workspace_layout_chain(state, child2->id);
            }
        }
        y += (block_height(state, *b) - m.overlap);
        cur = b->next_id;
    }
}

static void remove_from_top_level(AppState &state, int root_id)
{
    if (state.selected_sprite < 0 || state.selected_sprite >= (int)state.sprites.size())
        return;
    auto &tl = state.sprites[state.selected_sprite].top_level_blocks;
    tl.erase(std::remove(tl.begin(), tl.end(), root_id), tl.end());
}
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
            else if (p->arg0_id == id)
                p->arg0_id = -1;
            else if (p->arg1_id == id)
                p->arg1_id = -1;
            else if (p->arg2_id == id)
                p->arg2_id = -1;
        }
        b->parent_id = -1;
    }
    return id;
}

static void insert_chain_before(AppState &state, int target_id, int chain_root_id)
{
    BlockInstance *t = workspace_find(state, target_id);
    BlockInstance *r = workspace_find(state, chain_root_id);
    if (!t || !r)
        return;
    int parent = t->parent_id;
    int last_new = last_in_chain(state, chain_root_id);
    BlockInstance *lastb = workspace_find(state, last_new);
    if (!lastb)
        return;
    lastb->next_id = target_id;
    t->parent_id = last_new;
    r->parent_id = parent;
    if (parent == -1)
    {
        if (state.selected_sprite < 0 || state.selected_sprite >= (int)state.sprites.size())
            return;
        auto &top_levels = state.sprites[state.selected_sprite].top_level_blocks;
        bool replaced = false;
        for (size_t i = 0; i < top_levels.size(); ++i)
        {
            if (top_levels[i] == target_id)
            {
                top_levels[i] = chain_root_id;
                replaced = true;
                break;
            }
        }
        if (!replaced)
            top_levels.push_back(chain_root_id);
    }
    else
    {
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
    if (lastb)
    {
        lastb->next_id = chain_root_id;
        r->parent_id = last;
    }
}

static void delete_chain(AppState &state, int root_id)
{
    if (state.selected_sprite < 0 || state.selected_sprite >= (int)state.sprites.size())
        return;
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

    auto &blks = state.sprites[state.selected_sprite].blocks;
    blks.erase(std::remove_if(blks.begin(), blks.end(), [&](const BlockInstance &b)
                              { return std::find(ids.begin(), ids.end(), b.id) != ids.end(); }),
               blks.end());
    for (int id : ids)
        remove_from_top_level(state, id);
    if (state.active_input == INPUT_BLOCK_FIELD && std::find(ids.begin(), ids.end(), state.block_input.block_id) != ids.end())
    {
        state.active_input = INPUT_NONE;
        state.input_buffer.clear();
        state.block_input.block_id = -1;
    }
}

static void compute_snap(AppState &state, TTF_Font *font)
{
    state.drag.snap_above = false;
    state.drag.snap_valid = false;
    state.drag.snap_target_id = -1;
    state.drag.snap_type = SNAP_NONE;
    if (!state.drag.active)
        return;
    const int SNAP_DIST = 20;
    SDL_Rect dr = {state.drag.ghost_x, state.drag.ghost_y, 200, 40};
    auto is_bool = [](BlockKind k, int sub)
    { if (k == BK_SENSING && (sub == SENSB_TOUCHING || sub == SENSB_KEY_PRESSED || sub == SENSB_MOUSE_DOWN)) return true; if (k == BK_OPERATORS && (sub == OP_GT || sub == OP_LT || sub == OP_EQ || sub == OP_AND || sub == OP_OR || sub == OP_NOT)) return true; return false; };
    bool dragging_bool = false;
    bool dragging_reporter = false;
    if (state.drag.from_palette)
    {
        dragging_bool = is_bool(state.drag.palette_kind, state.drag.palette_subtype);
        dragging_reporter = (state.drag.palette_kind == BK_OPERATORS) || (state.drag.palette_kind == BK_VARIABLES && state.drag.palette_subtype == VB_VARIABLE) || dragging_bool || (state.drag.palette_kind == BK_SENSING && (state.drag.palette_subtype == SENSB_ANSWER || state.drag.palette_subtype == SENSB_DISTANCE_TO)) || (state.drag.palette_kind == BK_LOOKS && (state.drag.palette_subtype == LB_SIZE || state.drag.palette_subtype == LB_BACKDROP_NUM_NAME || state.drag.palette_subtype == LB_COSTUME_NUM_NAME));
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
            dragging_reporter = (rb->kind == BK_OPERATORS) || (rb->kind == BK_VARIABLES && rb->subtype == VB_VARIABLE) || dragging_bool || (rb->kind == BK_SENSING && (rb->subtype == SENSB_ANSWER || rb->subtype == SENSB_DISTANCE_TO)) || (rb->kind == BK_LOOKS && (rb->subtype == LB_SIZE || rb->subtype == LB_BACKDROP_NUM_NAME || rb->subtype == LB_COSTUME_NUM_NAME));
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
            if (dragging_reporter)
            {
                int field = -1;
                int px = state.drag.mouse_x;
                int py = state.drag.mouse_y;
                if (b->kind == BK_MOTION)
                    field = motion_block_hittest_field(font, (MotionBlockType)b->subtype, b->x, b->y, b->a, b->b, (GoToTarget)b->opt, px, py);
                else if (b->kind == BK_LOOKS)
                    // ---> FIXED: THIS LINE WAS MISSING THE STATE PARAMETER! <---
                    field = looks_block_hittest_field(font, state, (LooksBlockType)b->subtype, b->x, b->y, b->text, b->a, b->b, b->opt, px, py);
                else if (b->kind == BK_SOUND)
                    field = sound_block_hittest_field(font, (SoundBlockType)b->subtype, b->x, b->y, b->a, b->opt, px, py);
                else if (b->kind == BK_EVENTS)
                    field = events_block_hittest_field(font, (EventsBlockType)b->subtype, b->x, b->y, b->opt, px, py);
                else if (b->kind == BK_CONTROL)
                    field = control_block_hittest_field(font, (ControlBlockType)b->subtype, b->x, b->y, chain_height(state, b->child_id), chain_height(state, b->child2_id), b->a, px, py);
                else if (b->kind == BK_SENSING)
                {
                    if (b->subtype == SENSB_TOUCHING || b->subtype == SENSB_KEY_PRESSED || b->subtype == SENSB_MOUSE_DOWN)
                        field = sensing_boolean_block_hittest_field(font, (SensingBlockType)b->subtype, b->x, b->y, b->opt, b->a, b->b, b->c, b->d, b->e, b->f, px, py);
                    else if (b->subtype == SENSB_ANSWER || b->subtype == SENSB_DISTANCE_TO)
                        field = sensing_reporter_block_hittest_field(font, (SensingBlockType)b->subtype, b->x, b->y, px, py);
                    else
                        field = sensing_stack_block_hittest_field(font, (SensingBlockType)b->subtype, b->x, b->y, b->text, b->opt, px, py);
                }
                else if (b->kind == BK_OPERATORS)
                {
                    int cw0, cw1;
                    get_operator_capsule_widths(state, *b, cw0, cw1);
                    int total_w = block_rect(state, *b).w;
                    field = operators_block_hittest_dynamic(font, (OperatorsBlockType)b->subtype, b->x, b->y, total_w, cw0, cw1, px, py);
                }
                else if (b->kind == BK_VARIABLES)
                {
                    field = variables_block_hittest_field(font, state, (VariablesBlockType)b->subtype, b->x, b->y, b->text, b->opt, px, py);
                }
                if (field >= 0 && field <= 2)
                {
                    best_dist = 0;
                    best_id = cur;
                    best_dx = 0;
                    best_dy = 0;
                    best_type = (field == 0) ? SNAP_INPUT_1 : (field == 1 ? SNAP_INPUT_2 : SNAP_INPUT_3);
                    return;
                }
            }
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
            if (!dragging_reporter || dragging_bool)
            {
                try_snap(br.x, br.y + br.h - motion_block_metrics().overlap, SNAP_AFTER, false);
                try_snap(br.x, br.y - (dr.h - motion_block_metrics().overlap), SNAP_BEFORE, false);
            }
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
            if (b->arg0_id != -1)
                self(self, b->arg0_id);
            if (b->arg1_id != -1)
                self(self, b->arg1_id);
            if (b->arg2_id != -1)
                self(self, b->arg2_id);
            cur = b->next_id;
        }
    };
    if (state.selected_sprite >= 0 && state.selected_sprite < (int)state.sprites.size())
    {
        for (int root_id : state.sprites[state.selected_sprite].top_level_blocks)
            check_block_snaps(check_block_snaps, root_id);
    }
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

static void draw_chain(SDL_Renderer *r, TTF_Font *font, const Textures &tex, const AppState &state, Color bg, int root_id, bool ghost, int off_x, int off_y)
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
        // ---> FIXED: Pass the saved text buffer as the override so it renders! <---
        const char *ov0 = (b->arg0_id != -1) ? "" : (b->text.empty() ? nullptr : b->text.c_str());
        const char *ov1 = (b->arg1_id != -1) ? "" : (b->text2.empty() ? nullptr : b->text2.c_str());
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
            else
            {
                if (sel == 0)
                    ov0 = state.input_buffer.c_str();
                if (sel == 1)
                    ov1 = state.input_buffer.c_str();
            }
        }
        if (b->kind == BK_MOTION)
            motion_block_draw(r, font, tex, (MotionBlockType)b->subtype, bx, by, b->a, b->b, (GoToTarget)b->opt, ghost, bg, sel, ov0, ov1);
        else if (b->kind == BK_LOOKS)
            looks_block_draw(r, font, state, (LooksBlockType)b->subtype, bx, by, b->text, b->a, b->b, b->opt, ghost, bg, sel, ov0, ov1);
        else if (b->kind == BK_SOUND)
            sound_block_draw(r, font, (SoundBlockType)b->subtype, bx, by, b->a, b->opt, ghost, bg, sel, ov0);
        else if (b->kind == BK_EVENTS)
            events_block_draw(r, font, tex, (EventsBlockType)b->subtype, bx, by, b->opt, ghost, bg, -1);
        else if (b->kind == BK_SENSING)
        {
            SensingBlockType sbt = (SensingBlockType)b->subtype;
            if (sbt == SENSB_TOUCHING || sbt == SENSB_KEY_PRESSED || sbt == SENSB_MOUSE_DOWN)
                sensing_boolean_block_draw(r, font, sbt, bx, by, b->opt, b->a, b->b, b->c, b->d, b->e, b->f, ghost, bg, sel, ov0);
            else if (sbt == SENSB_ANSWER || sbt == SENSB_DISTANCE_TO)
                sensing_reporter_block_draw(r, font, sbt, bx, by, ghost);
            else
                sensing_stack_block_draw(r, font, sbt, bx, by, b->text, b->opt, ghost, bg, sel, ov0);
        }
        else if (b->kind == BK_CONTROL)
        {
            bool has_cond = (b->condition_id != -1);
            control_block_draw(r, font, (ControlBlockType)b->subtype, bx, by, chain_height(state, b->child_id), chain_height(state, b->child2_id), b->a, has_cond, ghost, bg, sel, ov0);
        }
        else if (b->kind == BK_OPERATORS)
        {
            int cw0, cw1;
            get_operator_capsule_widths(state, *b, cw0, cw1);
            int total_w = block_rect(state, *b).w;
            std::string s0 = b->text;
            std::string s1 = b->text2;
            if (s0.empty() && b->a != 0)
                s0 = std::to_string(b->a);
            if (s1.empty() && b->b != 0)
                s1 = std::to_string(b->b);
            if (b->subtype == OP_GT || b->subtype == OP_LT || b->subtype == OP_EQ)
            {
                if (s1.empty() && b->b == 0)
                    s1 = "50";
            }
            if (b->subtype == OP_JOIN && s0.empty() && s1.empty())
            {
                s0 = "apple";
                s1 = "banana";
            }
            if (b->subtype == OP_LETTER_OF && s0.empty() && s1.empty())
            {
                s0 = "1";
                s1 = "apple";
            }
            if (b->subtype == OP_LENGTH_OF && s0.empty())
            {
                s0 = "apple";
            }
            operators_block_draw_dynamic(r, font, (OperatorsBlockType)b->subtype, bx, by, total_w, s0, s1, cw0, cw1, ghost, bg, sel, ov0, ov1);
        }
        else if (b->kind == BK_VARIABLES)
        {
            if (b->subtype == VB_VARIABLE)
                variables_block_draw(r, font, state, VB_VARIABLE, bx, by, b->text, "", 0, 0, ghost, sel, ov0);
            else
                variables_block_draw(r, font, state, (VariablesBlockType)b->subtype, bx, by, "", b->text, b->a, b->opt, ghost, sel, ov0);
        }
        if (b->condition_id != -1)
            draw_chain(r, font, tex, state, bg, b->condition_id, ghost, off_x, off_y);
        if (b->child_id != -1)
            draw_chain(r, font, tex, state, bg, b->child_id, ghost, off_x, off_y);
        if (b->child2_id != -1)
            draw_chain(r, font, tex, state, bg, b->child2_id, ghost, off_x, off_y);
        auto draw_reporter = [&](int arg_id, int index)
        { if (arg_id != -1) { SDL_Rect cap = get_capsule_rect(font, state, *b, index); const BlockInstance *child = workspace_find_const(state, arg_id); if (child) { SDL_Rect cbr = block_rect(state, *child); int cx = cap.x - 2; int cy = cap.y + (cap.h - cbr.h) / 2; BlockInstance *mutable_child = workspace_find((AppState &)state, arg_id); if (mutable_child) { mutable_child->x = cx; mutable_child->y = cy; } draw_chain(r, font, tex, state, bg, arg_id, ghost, cx - child->x + off_x, cy - child->y + off_y); } } };
        draw_reporter(b->arg0_id, 0);
        draw_reporter(b->arg1_id, 1);
        draw_reporter(b->arg2_id, 2);
        cur = b->next_id;
    }
}

void workspace_draw(SDL_Renderer *r, TTF_Font *font, const Textures &tex, const AppState &state, const SDL_Rect &workspace_rect, Color bg)
{
    (void)workspace_rect;
    if (state.selected_sprite >= 0 && state.selected_sprite < (int)state.sprites.size())
    {
        for (int root_id : state.sprites[state.selected_sprite].top_level_blocks)
            draw_chain(r, font, tex, state, bg, root_id, false, 0, 0);
    }
    if (state.drag.active)
    {
        if (state.drag.snap_valid)
        {
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
            if (state.drag.snap_type == SNAP_INPUT_1 || state.drag.snap_type == SNAP_INPUT_2 || state.drag.snap_type == SNAP_INPUT_3)
            {
                BlockInstance *target = workspace_find((AppState &)state, state.drag.snap_target_id);
                if (target)
                {
                    int arg_idx = (state.drag.snap_type == SNAP_INPUT_1) ? 0 : (state.drag.snap_type == SNAP_INPUT_2 ? 1 : 2);
                    SDL_Rect cap = get_capsule_rect(font, state, *target, arg_idx);
                    SDL_SetRenderDrawColor(r, 255, 255, 255, 150);
                    renderer_fill_rounded_rect(r, &cap, cap.h / 2, 255, 255, 255);
                }
            }
            else
            {
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
            }
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        }
        int dx = state.drag.snap_valid && (state.drag.snap_type != SNAP_INPUT_1 && state.drag.snap_type != SNAP_INPUT_2 && state.drag.snap_type != SNAP_INPUT_3) ? (state.drag.snap_x - state.drag.ghost_x) : 0;
        int dy = state.drag.snap_valid && (state.drag.snap_type != SNAP_INPUT_1 && state.drag.snap_type != SNAP_INPUT_2 && state.drag.snap_type != SNAP_INPUT_3) ? (state.drag.snap_y - state.drag.ghost_y) : 0;
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
            else if (state.drag.palette_kind == BK_OPERATORS)
                def = workspace_make_default_operators((OperatorsBlockType)state.drag.palette_subtype);
            else if (state.drag.palette_kind == BK_VARIABLES)
                def = workspace_make_default_variables((VariablesBlockType)state.drag.palette_subtype, state.drag.palette_text);
            else
            {
                def.kind = BK_CONTROL;
                def.subtype = state.drag.palette_subtype;
                if (def.subtype == CB_WAIT)
                    def.a = 1;
                else
                    def.a = 10;
            }
            def.id = 9999;
            def.x = state.drag.ghost_x;
            def.y = state.drag.ghost_y;
            if (state.selected_sprite >= 0)
                ((AppState &)state).sprites[state.selected_sprite].blocks.push_back(def);
            draw_chain(r, font, tex, state, bg, 9999, true, dx, dy);
            if (state.selected_sprite >= 0)
                ((AppState &)state).sprites[state.selected_sprite].blocks.pop_back();
        }
        else
        {
            BlockInstance *drb = workspace_find((AppState &)state, state.drag.dragged_block_id);
            if (drb)
            {
                int real_dx = state.drag.ghost_x - drb->x + dx;
                int real_dy = state.drag.ghost_y - drb->y + dy;
                draw_chain(r, font, tex, state, bg, state.drag.dragged_block_id, true, real_dx, real_dy);
            }
        }
    }
}

static void start_drag_from_workspace(AppState &state, int clicked_id, int mx, int my, TTF_Font *font)
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
    compute_snap(state, font);
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
        else if (state.drag.palette_kind == BK_OPERATORS)
            b = workspace_make_default_operators((OperatorsBlockType)state.drag.palette_subtype);
        else if (state.drag.palette_kind == BK_VARIABLES)
            b = workspace_make_default_variables((VariablesBlockType)state.drag.palette_subtype, state.drag.palette_text);
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
            root->x = state.drag.ghost_x;
            root->y = state.drag.ghost_y;
            if (state.drag.snap_valid && (state.drag.snap_type != SNAP_INPUT_1 && state.drag.snap_type != SNAP_INPUT_2 && state.drag.snap_type != SNAP_INPUT_3))
            {
                root->x = state.drag.snap_x;
                root->y = state.drag.snap_y;
            }
            new_root_id = root->id;
            if (state.selected_sprite >= 0)
                state.sprites[state.selected_sprite].top_level_blocks.push_back(new_root_id);
        }
    }
    if (state.drag.snap_valid && new_root_id != -1)
    {
        BlockInstance *target = workspace_find(state, state.drag.snap_target_id);
        if (state.drag.snap_type == SNAP_BEFORE)
            insert_chain_before(state, state.drag.snap_target_id, new_root_id);
        else if (state.drag.snap_type == SNAP_AFTER)
        {
            attach_chain(state, state.drag.snap_target_id, new_root_id);
            remove_from_top_level(state, new_root_id);
        }
        else if (state.drag.snap_type == SNAP_CONDITION && target)
        {
            target->condition_id = new_root_id;
            BlockInstance *nr = workspace_find(state, new_root_id);
            if (nr)
                nr->parent_id = target->id;
            remove_from_top_level(state, new_root_id);
        }
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
        else if ((state.drag.snap_type == SNAP_INPUT_1 || state.drag.snap_type == SNAP_INPUT_2 || state.drag.snap_type == SNAP_INPUT_3) && target)
        {
            int old_child = -1;
            if (state.drag.snap_type == SNAP_INPUT_1)
            {
                old_child = target->arg0_id;
                target->arg0_id = new_root_id;
            }
            else if (state.drag.snap_type == SNAP_INPUT_2)
            {
                old_child = target->arg1_id;
                target->arg1_id = new_root_id;
            }
            else if (state.drag.snap_type == SNAP_INPUT_3)
            {
                old_child = target->arg2_id;
                target->arg2_id = new_root_id;
            }
            BlockInstance *nr = workspace_find(state, new_root_id);
            if (nr)
                nr->parent_id = target->id;
            remove_from_top_level(state, new_root_id);
            if (old_child != -1)
            {
                BlockInstance *old = workspace_find(state, old_child);
                if (old)
                {
                    old->parent_id = -1;
                    if (state.selected_sprite >= 0)
                        state.sprites[state.selected_sprite].top_level_blocks.push_back(old->id);
                    old->x = state.drag.ghost_x + 20;
                    old->y = state.drag.ghost_y + 40;
                }
            }
        }
        workspace_layout_chain(state, workspace_root_id(state, new_root_id));
    }
    else if (new_root_id != -1)
    {
        workspace_layout_chain(state, new_root_id);
    }
    if (state.selected_sprite >= 0)
    {
        for (int tl_id : state.sprites[state.selected_sprite].top_level_blocks)
            workspace_layout_chain(state, tl_id);
    }
    state.drag.active = false;
    state.drag.snap_valid = false;
    state.drag.snap_target_id = -1;
    state.drag.dragged_block_id = -1;
}

static BlockFieldType block_field_type(const BlockInstance &b, int field)
{
    if (b.kind == BK_LOOKS && (b.subtype == LB_SAY_FOR || b.subtype == LB_SAY || b.subtype == LB_THINK_FOR || b.subtype == LB_THINK))
        if (field == 0)
            return BFT_TEXT;
    if (b.kind == BK_SENSING && (b.subtype == SENSB_ASK_AND_WAIT))
        if (field == 0)
            return BFT_TEXT;
    if (b.kind == BK_OPERATORS)
        return BFT_TEXT;
    if (b.kind == BK_VARIABLES && b.subtype == VB_SET)
        return BFT_TEXT;
    if (b.kind == BK_VARIABLES && b.subtype == VB_CHANGE)
        return BFT_INT;
    return BFT_INT;
}

void workspace_commit_active_input(AppState &state)
{
    if (state.active_input != INPUT_BLOCK_FIELD)
        return;
    BlockInstance *b = workspace_find(state, state.block_input.block_id);
    if (!b)
        return;

    // ---> FIXED: Ensure we always save the raw text first for ALL block types! <---
    if (state.block_input.field_index == 0)
        b->text = state.input_buffer;
    else if (state.block_input.field_index == 1)
        b->text2 = state.input_buffer;

    // Then, if it's supposed to be an integer, also parse it to the math fields
    if (state.block_input.type == BFT_INT)
    {
        int val = 0;
        if (!state.input_buffer.empty() && state.input_buffer != "-")
            val = std::atoi(state.input_buffer.c_str());

        if (state.block_input.field_index == 0)
            b->a = val;
        else if (state.block_input.field_index == 1)
            b->b = val;
        else if (state.block_input.field_index == 2)
            b->c = val;
    }
}

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
        compute_snap(state, font);
        BlockInstance *dragged_b = workspace_find(state, state.drag.dragged_block_id);
        if (dragged_b && dragged_b->kind == BK_SENSING && dragged_b->subtype == SENSB_DISTANCE_TO)
        {
            if (state.selected_sprite >= 0 && state.selected_sprite < (int)state.sprites.size())
            {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                float dx = (mx - (1280 - 240)) - state.sprites[state.selected_sprite].x;
                float dy = (180 - my + 60) - state.sprites[state.selected_sprite].y;
                std::cout << "[DRAGGING] Live Distance to mouse-pointer: " << std::sqrt(dx * dx + dy * dy) << std::endl;
            }
        }
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
    if (state.selected_sprite < 0 || state.selected_sprite >= (int)state.sprites.size())
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
            if (b->arg0_id != -1)
            {
                int h = self(self, b->arg0_id);
                if (h != -1)
                    return h;
            }
            if (b->arg1_id != -1)
            {
                int h = self(self, b->arg1_id);
                if (h != -1)
                    return h;
            }
            if (b->arg2_id != -1)
            {
                int h = self(self, b->arg2_id);
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
    for (int i = (int)state.sprites[state.selected_sprite].top_level_blocks.size() - 1; i >= 0 && hit_id == -1; --i)
        hit_id = find_hit(find_hit, state.sprites[state.selected_sprite].top_level_blocks[i]);

    if (hit_id != -1)
    {
        BlockInstance *b = workspace_find(state, hit_id);
        if (!b)
            return true;
        int field = -1;
        if (b->kind == BK_MOTION)
            field = motion_block_hittest_field(font, (MotionBlockType)b->subtype, b->x, b->y, b->a, b->b, (GoToTarget)b->opt, e.button.x, e.button.y);
        else if (b->kind == BK_LOOKS)
            field = looks_block_hittest_field(font, state, (LooksBlockType)b->subtype, b->x, b->y, b->text, b->a, b->b, b->opt, e.button.x, e.button.y);
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
            else if (sbt == SENSB_ANSWER || sbt == SENSB_DISTANCE_TO)
                field = sensing_reporter_block_hittest_field(font, sbt, b->x, b->y, e.button.x, e.button.y);
            else
                field = sensing_stack_block_hittest_field(font, sbt, b->x, b->y, b->text, b->opt, e.button.x, e.button.y);
        }
        else if (b->kind == BK_OPERATORS)
        {
            int cw0, cw1;
            get_operator_capsule_widths(state, *b, cw0, cw1);
            int total_w = block_rect(state, *b).w;
            field = operators_block_hittest_dynamic(font, (OperatorsBlockType)b->subtype, b->x, b->y, total_w, cw0, cw1, e.button.x, e.button.y);
        }
        else if (b->kind == BK_VARIABLES)
        {
            field = variables_block_hittest_field(font, state, (VariablesBlockType)b->subtype, b->x, b->y, b->text, b->opt, e.button.x, e.button.y);
        }

        if (field == -2)
        {
            if (state.active_input == INPUT_BLOCK_FIELD)
            {
                workspace_commit_active_input(state);
                state.active_input = INPUT_NONE;
                state.input_buffer.clear();
            }
            int max_opt = 0;
            if (b->kind == BK_SENSING && b->subtype == SENSB_TOUCHING)
                max_opt = 3;
            else if (b->kind == BK_SENSING && b->subtype == SENSB_KEY_PRESSED)
                max_opt = 41;
            else if (b->kind == BK_EVENTS && b->subtype == EB_WHEN_KEY_PRESSED)
                max_opt = 41;
            else if (b->kind == BK_LOOKS && b->subtype == LB_SWITCH_COSTUME_TO)
                max_opt = 3;
            else if (b->kind == BK_LOOKS && b->subtype == LB_SWITCH_BACKDROP_TO)
                max_opt = state.backdrops.size();
            else if (b->kind == BK_LOOKS && b->subtype == LB_GO_TO_LAYER)
                max_opt = 2;
            else if (b->kind == BK_LOOKS && b->subtype == LB_GO_LAYERS)
                max_opt = 2;
            else if (b->kind == BK_LOOKS && b->subtype == LB_BACKDROP_NUM_NAME)
                max_opt = 2;
            else if (b->kind == BK_LOOKS && b->subtype == LB_COSTUME_NUM_NAME)
                max_opt = 2;
            else if (b->kind == BK_VARIABLES && (b->subtype == VB_SET || b->subtype == VB_CHANGE))
                max_opt = state.variables.size();
            else if (b->kind == BK_MOTION && b->subtype == MB_GO_TO_TARGET)
                max_opt = 2;
            else if (b->kind == BK_SOUND && (b->subtype == SB_START_SOUND || b->subtype == SB_PLAY_SOUND_UNTIL_DONE))
                max_opt = 1;
            else if (b->kind == BK_SENSING && b->subtype == SENSB_DISTANCE_TO)
                max_opt = 1;
            else if (b->kind == BK_SENSING && b->subtype == SENSB_SET_DRAG_MODE)
                max_opt = 2;
            if (max_opt > 0)
            {
                b->opt = (b->opt + 1) % max_opt;
                if (b->kind == BK_SENSING && b->subtype == SENSB_SET_DRAG_MODE)
                    state.sprites[state.selected_sprite].draggable = (b->opt == 0);
            }
            for (int tl_id : state.sprites[state.selected_sprite].top_level_blocks)
                workspace_layout_chain(state, tl_id);
            return true;
        }
        else if (field >= 0)
        {
            if (state.active_input == INPUT_BLOCK_FIELD)
            {
                workspace_commit_active_input(state);
            }
            state.active_input = INPUT_BLOCK_FIELD;
            state.block_input.block_id = b->id;
            state.block_input.field_index = field;
            state.block_input.type = block_field_type(*b, field);
            if (state.block_input.type == BFT_TEXT)
            {
                if (field == 0)
                    state.input_buffer = b->text;
                else
                    state.input_buffer = b->text2;
            }
            else
            {
                if (field == 0)
                    state.input_buffer = std::to_string(b->a);
                else if (field == 1)
                    state.input_buffer = std::to_string(b->b);
                else
                    state.input_buffer = std::to_string(b->c);
            }
            return true;
        }
        if (state.active_input == INPUT_BLOCK_FIELD)
        {
            workspace_commit_active_input(state);
            state.active_input = INPUT_NONE;
            state.input_buffer.clear();
        }
        start_drag_from_workspace(state, b->id, e.button.x, e.button.y, font);
        for (int tl_id : state.sprites[state.selected_sprite].top_level_blocks)
            workspace_layout_chain(state, tl_id);
        compute_snap(state, font);
        return true;
    }
    return false;
}
bool workspace_save_txt(const AppState & /*state*/, const char * /*path*/) { return false; }
bool workspace_load_txt(AppState & /*state*/, const char * /*path*/) { return false; }