#include "blocks.h"
#include <cstring>
#include <algorithm>
#include <string>

static const char *CAT_NAMES[] = {
    "Motion", "Looks", "Sound", "Events",
    "Control", "Sensing", "Operators", "Variables", "My Blocks", "Pen"};

static const Color CAT_COLORS[] = {
    {76, 151, 255}, {153, 102, 255}, {207, 99, 207}, {255, 191, 38}, {255, 171, 25}, {90, 188, 216}, {89, 192, 89}, {255, 140, 26}, {255, 102, 128}, {15, 189, 140}};

static BlockDef make_c_shape(BlockKind kind, int subtype, Color col, int w, int h, const char *label)
{
    BlockDef b;
    b.label = label;
    b.color = col;
    b.width = w;
    b.height = h;
    b.is_stack_block = true;
    b.is_c_shape = true;
    b.kind = kind;
    b.subtype = subtype;
    return b;
}
static BlockDef make_reporter(BlockKind kind, int subtype, Color col, int w, int h, const char *label)
{
    BlockDef b;
    b.label = label;
    b.color = col;
    b.width = w;
    b.height = h;
    b.is_reporter_block = true;
    b.kind = kind;
    b.subtype = subtype;
    return b;
}
static BlockDef make_e_shape(BlockKind kind, int subtype, Color col, int w, int h, const char *label)
{
    BlockDef b;
    b.label = label;
    b.color = col;
    b.width = w;
    b.height = h;
    b.is_stack_block = true;
    b.is_c_shape = true;
    b.is_e_shape = true;
    b.kind = kind;
    b.subtype = subtype;
    return b;
}
static BlockDef make_placeholder(const char *label, Color col, int w, int h)
{
    BlockDef b;
    b.label = label;
    b.color = col;
    b.width = w;
    b.height = h;
    return b;
}
static BlockDef make_stack(BlockKind kind, int subtype, Color col, int w, int h, const char *label)
{
    BlockDef b;
    b.label = label;
    b.color = col;
    b.width = w;
    b.height = h;
    b.is_stack_block = true;
    b.kind = kind;
    b.subtype = subtype;
    return b;
}
static BlockDef make_boolean(BlockKind kind, int subtype, Color col, int w, int h, const char *label)
{
    BlockDef b;
    b.label = label;
    b.color = col;
    b.width = w;
    b.height = h;
    b.is_boolean_block = true;
    b.kind = kind;
    b.subtype = subtype;
    return b;
}

const char *blocks_category_name(int cat) { return (cat < 0 || cat > 9) ? "???" : CAT_NAMES[cat]; }
Color blocks_category_color(int cat) { return (cat < 0 || cat > 9) ? (Color){128, 128, 128} : CAT_COLORS[cat]; }

int blocks_get_for_category(const AppState &state, int cat, BlockDef *out, int max_out)
{
    Color c = blocks_category_color(cat);
    int n = 0;

    auto push = [&](BlockDef b)
    { if (out && n < max_out) out[n] = b; n++; };

    switch (cat)
    {
    case 0: /* Motion */
        push(make_stack(BK_MOTION, (int)MB_MOVE_STEPS, c, 230, 40, "move"));
        push(make_stack(BK_MOTION, (int)MB_TURN_RIGHT_DEG, c, 230, 40, "turn right"));
        push(make_stack(BK_MOTION, (int)MB_TURN_LEFT_DEG, c, 230, 40, "turn left"));
        push(make_stack(BK_MOTION, (int)MB_GO_TO_TARGET, c, 250, 40, "go to"));
        push(make_stack(BK_MOTION, (int)MB_GO_TO_XY, c, 260, 40, "go to x y"));
        push(make_stack(BK_MOTION, (int)MB_CHANGE_X_BY, c, 220, 40, "change x by"));
        push(make_stack(BK_MOTION, (int)MB_CHANGE_Y_BY, c, 220, 40, "change y by"));
        push(make_stack(BK_MOTION, (int)MB_POINT_IN_DIR, c, 250, 40, "point in direction"));
        break;
    case 1: /* Looks */
        push(make_stack(BK_LOOKS, (int)LB_SAY_FOR, c, 300, 40, "say for"));
        push(make_stack(BK_LOOKS, (int)LB_SAY, c, 250, 40, "say"));
        push(make_stack(BK_LOOKS, (int)LB_THINK_FOR, c, 320, 40, "think for"));
        push(make_stack(BK_LOOKS, (int)LB_THINK, c, 260, 40, "think"));
        push(make_stack(BK_LOOKS, (int)LB_SWITCH_COSTUME_TO, c, 300, 40, "switch costume to"));
        push(make_stack(BK_LOOKS, (int)LB_NEXT_COSTUME, c, 220, 40, "next costume"));
        push(make_stack(BK_LOOKS, (int)LB_SWITCH_BACKDROP_TO, c, 310, 40, "switch backdrop to"));
        push(make_stack(BK_LOOKS, (int)LB_NEXT_BACKDROP, c, 220, 40, "next backdrop"));
        push(make_stack(BK_LOOKS, (int)LB_CHANGE_SIZE_BY, c, 270, 40, "change size by"));
        push(make_stack(BK_LOOKS, (int)LB_SET_SIZE_TO, c, 270, 40, "set size to"));
        push(make_stack(BK_LOOKS, (int)LB_SHOW, c, 160, 40, "show"));
        push(make_stack(BK_LOOKS, (int)LB_HIDE, c, 160, 40, "hide"));
        push(make_stack(BK_LOOKS, (int)LB_GO_TO_LAYER, c, 260, 40, "go to layer"));
        push(make_stack(BK_LOOKS, (int)LB_GO_LAYERS, c, 310, 40, "go layers"));
        push(make_reporter(BK_LOOKS, (int)LB_SIZE, c, 80, 40, "size"));
        push(make_reporter(BK_LOOKS, (int)LB_BACKDROP_NUM_NAME, c, 170, 40, "backdrop"));
        push(make_reporter(BK_LOOKS, (int)LB_COSTUME_NUM_NAME, c, 170, 40, "costume"));
        break;
    case 2: /* Sound */
        push(make_stack(BK_SOUND, (int)SB_CHANGE_VOLUME_BY, c, 300, 40, "change volume by"));
        push(make_stack(BK_SOUND, (int)SB_SET_VOLUME_TO, c, 280, 40, "set volume to"));
        push(make_stack(BK_SOUND, (int)SB_STOP_ALL_SOUNDS, c, 230, 40, "stop all sounds"));
        push(make_stack(BK_SOUND, (int)SB_START_SOUND, c, 260, 40, "start sound"));
        push(make_stack(BK_SOUND, (int)SB_PLAY_SOUND_UNTIL_DONE, c, 330, 40, "play sound until done"));
        break;
    case 3: /* Events */
        // ---> FIXED: Hat blocks use height 48, Broadcast is a normal stack block height 40 <---
        push(make_stack(BK_EVENTS, (int)EB_WHEN_FLAG_CLICKED, c, 260, 48, "when flag clicked"));
        push(make_stack(BK_EVENTS, (int)EB_WHEN_KEY_PRESSED, c, 330, 48, "when key pressed"));
        push(make_stack(BK_EVENTS, (int)EB_WHEN_SPRITE_CLICKED, c, 280, 48, "when sprite clicked"));
        push(make_stack(BK_EVENTS, (int)EB_WHEN_I_RECEIVE, c, 320, 48, "when I receive"));
        push(make_stack(BK_EVENTS, (int)EB_BROADCAST, c, 260, 40, "broadcast"));
        break;
    case 4: /* Control */
        push(make_stack(BK_CONTROL, (int)CB_WAIT, c, 200, 40, "wait"));
        push(make_c_shape(BK_CONTROL, (int)CB_REPEAT, c, 220, 80, "repeat"));
        push(make_c_shape(BK_CONTROL, (int)CB_FOREVER, c, 180, 70, "forever"));
        push(make_c_shape(BK_CONTROL, (int)CB_IF, c, 240, 80, "if"));
        push(make_e_shape(BK_CONTROL, (int)CB_IF_ELSE, c, 240, 130, "if else"));
        push(make_stack(BK_CONTROL, (int)CB_WAIT_UNTIL, c, 240, 40, "wait until"));
        break;
    case 5: /* Sensing */
        push(make_boolean(BK_SENSING, (int)SENSB_TOUCHING, c, 280, 36, "touching"));
        push(make_boolean(BK_SENSING, (int)SENSB_TOUCHING_COLOR, c, 160, 36, "touching color"));
        push(make_boolean(BK_SENSING, (int)SENSB_COLOR_IS_TOUCHING_COLOR, c, 260, 36, "color is touching color"));
        push(make_stack(BK_SENSING, (int)SENSB_ASK_AND_WAIT, c, 320, 40, "ask and wait"));
        push(make_reporter(BK_SENSING, (int)SENSB_ANSWER, c, 80, 40, "answer"));
        push(make_boolean(BK_SENSING, (int)SENSB_KEY_PRESSED, c, 280, 36, "key pressed"));
        push(make_boolean(BK_SENSING, (int)SENSB_MOUSE_DOWN, c, 220, 36, "mouse down"));
        push(make_reporter(BK_SENSING, (int)SENSB_MOUSE_X, c, 100, 40, "mouse x"));
        push(make_reporter(BK_SENSING, (int)SENSB_MOUSE_Y, c, 100, 40, "mouse y"));
        push(make_stack(BK_SENSING, (int)SENSB_SET_DRAG_MODE, c, 300, 40, "set drag mode"));
        push(make_reporter(BK_SENSING, (int)SENSB_DISTANCE_TO, c, 180, 40, "distance to"));
        break;
    case 6: /* Operators */
        push(make_reporter(BK_OPERATORS, (int)OP_ADD, c, 120, 40, "+"));
        push(make_reporter(BK_OPERATORS, (int)OP_SUB, c, 120, 40, "-"));
        push(make_reporter(BK_OPERATORS, (int)OP_MUL, c, 120, 40, "*"));
        push(make_reporter(BK_OPERATORS, (int)OP_DIV, c, 120, 40, "/"));
        push(make_boolean(BK_OPERATORS, (int)OP_GT, c, 140, 40, ">"));
        push(make_boolean(BK_OPERATORS, (int)OP_LT, c, 140, 40, "<"));
        push(make_boolean(BK_OPERATORS, (int)OP_EQ, c, 140, 40, "="));
        push(make_boolean(BK_OPERATORS, (int)OP_AND, c, 140, 40, "and"));
        push(make_boolean(BK_OPERATORS, (int)OP_OR, c, 140, 40, "or"));
        push(make_boolean(BK_OPERATORS, (int)OP_NOT, c, 100, 40, "not"));
        push(make_reporter(BK_OPERATORS, (int)OP_JOIN, c, 180, 40, "join"));
        push(make_reporter(BK_OPERATORS, (int)OP_LETTER_OF, c, 180, 40, "letter of"));
        push(make_reporter(BK_OPERATORS, (int)OP_LENGTH_OF, c, 140, 40, "length of"));
        break;
    case 7: /* Variables */
        for (const auto &var : state.variables)
        {
            push(make_reporter(BK_VARIABLES, (int)VB_VARIABLE, c, std::max(60, (int)var.length() * 8 + 24), 40, var.c_str()));
        }
        push(make_stack(BK_VARIABLES, (int)VB_SET, c, 240, 40, "set"));
        push(make_stack(BK_VARIABLES, (int)VB_CHANGE, c, 240, 40, "change"));
        break;
    case 8: /* My Blocks */
        break;
    case 9: /* Pen */
        push(make_stack(BK_PEN, (int)PB_ERASE_ALL, c, 160, 40, "erase all"));
        push(make_stack(BK_PEN, (int)PB_STAMP, c, 140, 40, "stamp"));
        push(make_stack(BK_PEN, (int)PB_PEN_DOWN, c, 160, 40, "pen down"));
        push(make_stack(BK_PEN, (int)PB_PEN_UP, c, 140, 40, "pen up"));
        push(make_stack(BK_PEN, (int)PB_SET_COLOR_TO_PICKER, c, 240, 40, "set pen color to"));
        push(make_stack(BK_PEN, (int)PB_CHANGE_ATTRIB_BY, c, 260, 40, "change pen color by"));
        push(make_stack(BK_PEN, (int)PB_SET_ATTRIB_TO, c, 240, 40, "set pen color to"));
        push(make_stack(BK_PEN, (int)PB_CHANGE_SIZE_BY, c, 260, 40, "change pen size by"));
        push(make_stack(BK_PEN, (int)PB_SET_SIZE_TO, c, 240, 40, "set pen size to"));
        break;
    default:
        push(make_placeholder("Not implemented", c, 160, 34));
        break;
    }
    return n;
}