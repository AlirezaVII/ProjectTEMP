#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>
#include <unordered_map>
#include <SDL.h>

enum Tab
{
    TAB_CODE = 0,
    TAB_COSTUMES = 1,
    TAB_SOUNDS = 2
};
enum AppMode
{
    MODE_EDITOR = 0,
    MODE_SPRITE_LIBRARY,
    MODE_BACKDROP_LIBRARY
};

enum ActiveInput
{
    INPUT_NONE = 0,
    INPUT_PROJECT_NAME,
    INPUT_SPRITE_NAME,
    INPUT_X,
    INPUT_Y,
    INPUT_DIRECTION,
    INPUT_SIZE,
    INPUT_BLOCK_FIELD,
    INPUT_VAR_MODAL
};

enum MotionBlockType
{
    MB_MOVE_STEPS = 0,
    MB_TURN_RIGHT_DEG,
    MB_TURN_LEFT_DEG,
    MB_GO_TO_XY,
    MB_CHANGE_X_BY,
    MB_CHANGE_Y_BY,
    MB_POINT_IN_DIR,
    MB_GO_TO_TARGET
};
enum GoToTarget
{
    TARGET_RANDOM_POSITION = 0,
    TARGET_MOUSE_POINTER = 1
};
enum LooksBlockType
{
    LB_SAY_FOR = 0,
    LB_SAY,
    LB_THINK_FOR,
    LB_THINK,
    LB_SWITCH_COSTUME_TO,
    LB_NEXT_COSTUME,
    LB_SWITCH_BACKDROP_TO,
    LB_NEXT_BACKDROP,
    LB_CHANGE_SIZE_BY,
    LB_SET_SIZE_TO,
    LB_SHOW,
    LB_HIDE,
    LB_GO_TO_LAYER,
    LB_GO_LAYERS
};
enum SoundBlockType
{
    SB_CHANGE_VOLUME_BY = 0,
    SB_SET_VOLUME_TO,
    SB_STOP_ALL_SOUNDS,
    SB_START_SOUND,
    SB_PLAY_SOUND_UNTIL_DONE
};
enum EventsBlockType
{
    EB_WHEN_FLAG_CLICKED = 0,
    EB_WHEN_KEY_PRESSED,
    EB_WHEN_SPRITE_CLICKED,
    EB_WHEN_I_RECEIVE,
    EB_BROADCAST
};
enum ControlBlockType
{
    CB_WAIT = 0,
    CB_REPEAT,
    CB_FOREVER,
    CB_IF,
    CB_WAIT_UNTIL,
    CB_IF_ELSE
};
enum SensingBlockType
{
    SENSB_TOUCHING = 0,
    SENSB_ASK_AND_WAIT,
    SENSB_KEY_PRESSED,
    SENSB_MOUSE_DOWN,
    SENSB_SET_DRAG_MODE,
    SENSB_ANSWER,
    SENSB_DISTANCE_TO,
    SENSB_COUNT
};
enum OperatorsBlockType
{
    OP_ADD = 0,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_GT,
    OP_LT,
    OP_EQ,
    OP_AND,
    OP_OR,
    OP_NOT,
    OP_JOIN,
    OP_LETTER_OF,
    OP_LENGTH_OF
};
enum VariablesBlockType
{
    VB_VARIABLE = 0,
    VB_SET,
    VB_CHANGE,
    VB_SHOW,
    VB_HIDE
};
enum TouchingTarget
{
    TOUCHING_MOUSE_POINTER = 0,
    TOUCHING_EDGE = 1,
    TOUCHING_SPRITE = 2
};
enum DragMode
{
    DRAG_DRAGGABLE = 0,
    DRAG_NOT_DRAGGABLE = 1
};
enum BlockKind
{
    BK_MOTION = 0,
    BK_LOOKS = 1,
    BK_SOUND = 2,
    BK_EVENTS = 3,
    BK_CONTROL = 4,
    BK_SENSING = 5,
    BK_OPERATORS = 6,
    BK_VARIABLES = 7
};
enum BlockFieldType
{
    BFT_INT = 0,
    BFT_TEXT = 1
};

struct BlockInstance
{
    int id = -1, subtype = 0, x = 0, y = 0, a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, opt = 0;
    BlockKind kind = BK_MOTION;
    std::string text, text2;
    int next_id = -1, parent_id = -1, child_id = -1, child2_id = -1, condition_id = -1;
    int arg0_id = -1, arg1_id = -1, arg2_id = -1;
};

enum SnapType
{
    SNAP_NONE = 0,
    SNAP_AFTER,
    SNAP_BEFORE,
    SNAP_INSIDE_1,
    SNAP_INSIDE_2,
    SNAP_CONDITION,
    SNAP_INPUT_1,
    SNAP_INPUT_2,
    SNAP_INPUT_3
};

struct DragState
{
    bool active, from_palette, snap_valid, snap_above;
    BlockKind palette_kind;
    int palette_subtype, dragged_block_id, off_x, off_y;
    std::string palette_text;
    int ghost_x, ghost_y, mouse_x, mouse_y, snap_x, snap_y, snap_target_id;
    SnapType snap_type;
    DragState() : active(false), from_palette(false), palette_kind(BK_MOTION), palette_subtype(0), dragged_block_id(-1), off_x(0), off_y(0), ghost_x(0), ghost_y(0), mouse_x(0), mouse_y(0), snap_valid(false), snap_above(false), snap_x(0), snap_y(0), snap_target_id(-1), snap_type(SNAP_NONE) {}
};

struct BlockInputState
{
    int block_id, field_index;
    BlockFieldType type;
    BlockInputState() : block_id(-1), field_index(0), type(BFT_INT) {}
};

struct Sprite
{
    std::string name;
    int x, y, direction;
    bool visible;
    int size;
    std::string say_text;
    bool is_thinking;
    unsigned int say_end_time;
    int volume;
    bool draggable;
    SDL_Texture *texture;
    std::vector<BlockInstance> blocks;
    std::vector<int> top_level_blocks;
    Sprite(std::string n, SDL_Texture *tex) : name(n), x(0), y(0), direction(90), visible(true), size(100), say_text(""), is_thinking(false), say_end_time(0), volume(100), draggable(true), texture(tex) {}
};

// ---> NEW: BACKDROP STRUCT <---
struct Backdrop
{
    std::string name;
    SDL_Texture *texture;
    Backdrop(std::string n, SDL_Texture *t) : name(n), texture(t) {}
};

struct AppState
{
    bool file_menu_open;
    int file_menu_hover;
    bool sprite_menu_open, backdrop_menu_open;
    Tab current_tab;
    bool start_hover, stop_hover, running;

    AppMode mode;

    std::vector<Sprite> sprites;
    int selected_sprite;
    bool add_sprite_hover;

    // ---> NEW: BACKDROP MEMORY <---
    std::vector<Backdrop> backdrops;
    int selected_backdrop;

    int selected_tab, selected_category;
    std::string project_name;
    DragState drag;
    int next_block_id;
    ActiveInput active_input;
    std::string input_buffer;
    BlockInputState block_input;

    std::vector<std::string> variables;
    std::unordered_map<std::string, std::string> variable_values;
    std::unordered_map<std::string, bool> variable_visible;
    bool var_modal_active;
    bool stage_drag_active;
    int stage_drag_off_x, stage_drag_off_y;

    bool ask_active;
    std::string ask_msg;
    std::string ask_reply;
    std::string global_answer;

    AppState() : file_menu_open(false), file_menu_hover(-1), sprite_menu_open(false), backdrop_menu_open(false), current_tab(TAB_CODE), start_hover(false), stop_hover(false), running(false), mode(MODE_EDITOR), selected_sprite(0), add_sprite_hover(false), selected_backdrop(0), selected_tab(TAB_CODE), selected_category(0), project_name("Untitled"), drag(), next_block_id(1), active_input(INPUT_NONE), input_buffer(""), block_input(), variables({"my variable"}), variable_values({{"my variable", "0"}}), variable_visible({{"my variable", true}}), var_modal_active(false), stage_drag_active(false), stage_drag_off_x(0), stage_drag_off_y(0), ask_active(false), ask_msg(""), ask_reply(""), global_answer("") {}
};

#endif