#include "blocks.h"
#include <cstring>

static const char *CAT_NAMES[] = {
    "Motion", "Looks", "Sound", "Events",
    "Control", "Sensing", "Operators", "Variables", "My Blocks"};

static const Color CAT_COLORS[] = {
    {76, 151, 255}, {153, 102, 255}, {207, 99, 207}, {255, 191, 38}, {255, 171, 25}, {90, 188, 216}, {89, 192, 89}, {255, 140, 26}, {255, 102, 128}};

static BlockDef make_c_shape(BlockKind kind, int subtype, Color col, int w, int h, const char *label)
{
    BlockDef b;
    b.label = label;
    b.color = col;
    b.width = w;
    b.height = h;
    b.is_stack_block = true;
    b.is_boolean_block = false;
    b.is_c_shape = true;
    b.is_e_shape = false;
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
    b.is_stack_block = false;
    b.is_boolean_block = false;
    b.is_c_shape = false;
    b.is_e_shape = false;
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
    b.is_boolean_block = false;
    b.is_c_shape = true;
    b.is_e_shape = true;
    b.kind = kind;
    b.subtype = subtype;
    return b;
}

const char *blocks_category_name(int cat)
{
    if (cat < 0 || cat > 8)
        return "???";
    return CAT_NAMES[cat];
}

Color blocks_category_color(int cat)
{
    if (cat < 0 || cat > 8)
    {
        Color c = {128, 128, 128};
        return c;
    }
    return CAT_COLORS[cat];
}

static BlockDef make_placeholder(const char *label, Color col, int w, int h)
{
    BlockDef b;
    b.label = label;
    b.color = col;
    b.width = w;
    b.height = h;
    b.is_stack_block = false;
    b.is_boolean_block = false;
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
    b.is_boolean_block = false;
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
    b.is_stack_block = false;
    b.is_boolean_block = true;
    b.kind = kind;
    b.subtype = subtype;
    return b;
}

int blocks_get_for_category(int cat, BlockDef *out, int max_out)
{
    Color c = blocks_category_color(cat);
    int n = 0;

    switch (cat)
    {
    case 0: /* Motion */
        if (n < max_out)
            out[n++] = make_stack(BK_MOTION, (int)MB_MOVE_STEPS, c, 230, 40, "move");
        if (n < max_out)
            out[n++] = make_stack(BK_MOTION, (int)MB_TURN_RIGHT_DEG, c, 230, 40, "turn right");
        if (n < max_out)
            out[n++] = make_stack(BK_MOTION, (int)MB_TURN_LEFT_DEG, c, 230, 40, "turn left");
        if (n < max_out)
            out[n++] = make_stack(BK_MOTION, (int)MB_GO_TO_TARGET, c, 250, 40, "go to");
        if (n < max_out)
            out[n++] = make_stack(BK_MOTION, (int)MB_GO_TO_XY, c, 260, 40, "go to x y");
        if (n < max_out)
            out[n++] = make_stack(BK_MOTION, (int)MB_CHANGE_X_BY, c, 220, 40, "change x by");
        if (n < max_out)
            out[n++] = make_stack(BK_MOTION, (int)MB_CHANGE_Y_BY, c, 220, 40, "change y by");
        if (n < max_out)
            out[n++] = make_stack(BK_MOTION, (int)MB_POINT_IN_DIR, c, 250, 40, "point in direction");
        break;

    case 1: /* Looks (NO reporters anymore) */
        if (n < max_out)
            out[n++] = make_stack(BK_LOOKS, (int)LB_SAY_FOR, c, 300, 40, "say for");
        if (n < max_out)
            out[n++] = make_stack(BK_LOOKS, (int)LB_SAY, c, 250, 40, "say");
        if (n < max_out)
            out[n++] = make_stack(BK_LOOKS, (int)LB_THINK_FOR, c, 320, 40, "think for");
        if (n < max_out)
            out[n++] = make_stack(BK_LOOKS, (int)LB_THINK, c, 260, 40, "think");
        if (n < max_out)
            out[n++] = make_stack(BK_LOOKS, (int)LB_SWITCH_COSTUME_TO, c, 300, 40, "switch costume to");
        if (n < max_out)
            out[n++] = make_stack(BK_LOOKS, (int)LB_NEXT_COSTUME, c, 220, 40, "next costume");
        if (n < max_out)
            out[n++] = make_stack(BK_LOOKS, (int)LB_SWITCH_BACKDROP_TO, c, 310, 40, "switch backdrop to");
        if (n < max_out)
            out[n++] = make_stack(BK_LOOKS, (int)LB_NEXT_BACKDROP, c, 220, 40, "next backdrop");
        if (n < max_out)
            out[n++] = make_stack(BK_LOOKS, (int)LB_CHANGE_SIZE_BY, c, 270, 40, "change size by");
        if (n < max_out)
            out[n++] = make_stack(BK_LOOKS, (int)LB_SET_SIZE_TO, c, 270, 40, "set size to");
        if (n < max_out)
            out[n++] = make_stack(BK_LOOKS, (int)LB_SHOW, c, 160, 40, "show");
        if (n < max_out)
            out[n++] = make_stack(BK_LOOKS, (int)LB_HIDE, c, 160, 40, "hide");
        if (n < max_out)
            out[n++] = make_stack(BK_LOOKS, (int)LB_GO_TO_LAYER, c, 260, 40, "go to layer");
        if (n < max_out)
            out[n++] = make_stack(BK_LOOKS, (int)LB_GO_LAYERS, c, 310, 40, "go layers");
        break;

    case 2: /* Sound */
        if (n < max_out)
            out[n++] = make_stack(BK_SOUND, (int)SB_CHANGE_VOLUME_BY, c, 300, 40, "change volume by");
        if (n < max_out)
            out[n++] = make_stack(BK_SOUND, (int)SB_SET_VOLUME_TO, c, 280, 40, "set volume to");
        if (n < max_out)
            out[n++] = make_stack(BK_SOUND, (int)SB_STOP_ALL_SOUNDS, c, 230, 40, "stop all sounds");
        if (n < max_out)
            out[n++] = make_stack(BK_SOUND, (int)SB_START_SOUND, c, 260, 40, "start sound");
        if (n < max_out)
            out[n++] = make_stack(BK_SOUND, (int)SB_PLAY_SOUND_UNTIL_DONE, c, 330, 40, "play sound until done");
        break;
    case 3: /* Events */
        if (n < max_out)
            out[n++] = make_stack(BK_EVENTS, (int)EB_WHEN_FLAG_CLICKED, c, 260, 40, "when flag clicked");
        if (n < max_out)
            out[n++] = make_stack(BK_EVENTS, (int)EB_WHEN_KEY_PRESSED, c, 330, 40, "when key pressed");
        if (n < max_out)
            out[n++] = make_stack(BK_EVENTS, (int)EB_WHEN_SPRITE_CLICKED, c, 280, 40, "when sprite clicked");
        if (n < max_out)
            out[n++] = make_stack(BK_EVENTS, (int)EB_WHEN_I_RECEIVE, c, 320, 40, "when I receive");
        if (n < max_out)
            out[n++] = make_stack(BK_EVENTS, (int)EB_BROADCAST, c, 260, 40, "broadcast");
        break;
    case 4: /* Control */
    {
        Color cc = CAT_COLORS[4]; // Orange
        if (n < max_out)
            out[n++] = make_stack(BK_CONTROL, (int)CB_WAIT, cc, 200, 40, "wait");
        if (n < max_out)
            out[n++] = make_c_shape(BK_CONTROL, (int)CB_REPEAT, cc, 220, 80, "repeat");
        if (n < max_out)
            out[n++] = make_c_shape(BK_CONTROL, (int)CB_FOREVER, cc, 180, 70, "forever");
        if (n < max_out)
            out[n++] = make_c_shape(BK_CONTROL, (int)CB_IF, cc, 240, 80, "if");
        if (n < max_out)
            out[n++] = make_e_shape(BK_CONTROL, (int)CB_IF_ELSE, cc, 240, 130, "if else");
        if (n < max_out)
            out[n++] = make_stack(BK_CONTROL, (int)CB_WAIT_UNTIL, cc, 240, 40, "wait until");
        break;
    }
    case 5: /* Sensing */
    {
        Color c = {90, 188, 216}; // Sensing color
        if (n < max_out) out[n++] = make_boolean(BK_SENSING, (int)SENSB_TOUCHING, c, 280, 36, "touching");
        if (n < max_out) out[n++] = make_stack(BK_SENSING, (int)SENSB_ASK_AND_WAIT, c, 320, 40, "ask and wait");
        if (n < max_out) out[n++] = make_boolean(BK_SENSING, (int)SENSB_KEY_PRESSED, c, 280, 36, "key pressed");
        if (n < max_out) out[n++] = make_boolean(BK_SENSING, (int)SENSB_MOUSE_DOWN, c, 220, 36, "mouse down");
        if (n < max_out) out[n++] = make_stack(BK_SENSING, (int)SENSB_SET_DRAG_MODE, c, 300, 40, "set drag mode");
        break;
    }
    case 6: /* Operators */{
        Color c = CAT_COLORS[6];
        // Math Reporters
        if (n < max_out) out[n++] = make_reporter(BK_OPERATORS, (int)OP_ADD, c, 120, 40, "+");
        if (n < max_out) out[n++] = make_reporter(BK_OPERATORS, (int)OP_SUB, c, 120, 40, "-");
        if (n < max_out) out[n++] = make_reporter(BK_OPERATORS, (int)OP_MUL, c, 120, 40, "*");
        if (n < max_out) out[n++] = make_reporter(BK_OPERATORS, (int)OP_DIV, c, 120, 40, "/");
        if (n < max_out) out[n++] = make_reporter(BK_OPERATORS, (int)OP_RANDOM, c, 190, 40, "pick random");
        // Booleans
        if (n < max_out) out[n++] = make_boolean(BK_OPERATORS, (int)OP_GT, c, 140, 40, ">");
        if (n < max_out) out[n++] = make_boolean(BK_OPERATORS, (int)OP_LT, c, 140, 40, "<");
        if (n < max_out) out[n++] = make_boolean(BK_OPERATORS, (int)OP_EQ, c, 140, 40, "=");
        if (n < max_out) out[n++] = make_boolean(BK_OPERATORS, (int)OP_AND, c, 140, 40, "and");
        if (n < max_out) out[n++] = make_boolean(BK_OPERATORS, (int)OP_OR, c, 140, 40, "or");
        if (n < max_out) out[n++] = make_boolean(BK_OPERATORS, (int)OP_NOT, c, 100, 40, "not");
        // String Reporters
        if (n < max_out) out[n++] = make_reporter(BK_OPERATORS, (int)OP_JOIN, c, 180, 40, "join");
        if (n < max_out) out[n++] = make_reporter(BK_OPERATORS, (int)OP_LETTER_OF, c, 180, 40, "letter of");
        if (n < max_out) out[n++] = make_reporter(BK_OPERATORS, (int)OP_LENGTH_OF, c, 140, 40, "length of");
        if (n < max_out) out[n++] = make_boolean(BK_OPERATORS, (int)OP_CONTAINS, c, 200, 40, "contains");
        break;
    }

    default:
        if (n < max_out)
            out[n++] = make_placeholder("Not implemented", c, 160, 34);
        break;
    }

    return n;
}
