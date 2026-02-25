#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <SDL.h>

struct Mix_Chunk;

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
    MODE_BACKDROP_LIBRARY,
    MODE_EXTENSION_LIBRARY
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
    INPUT_VAR_MODAL,
    INPUT_SOUND_NAME,
    INPUT_SOUND_VOLUME,
    INPUT_PEN_COLOR_PICKER,
    INPUT_COSTUME_NAME,
    INPUT_COSTUME_TEXT,
    INPUT_MSG_MODAL,
    INPUT_BLOCK_COLOR_PICKER_1,
    INPUT_BLOCK_COLOR_PICKER_2,
    INPUT_FUNC_MODAL_NAME,   // typing function name in Make-a-Block dialog
    INPUT_FUNC_MODAL_PARAM   // typing param name in Make-a-Block dialog
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
    LB_GO_LAYERS,
    LB_SIZE,
    LB_BACKDROP_NUM_NAME,
    LB_COSTUME_NUM_NAME
};
enum SoundBlockType
{
    SB_CHANGE_VOLUME_BY = 0,
    SB_SET_VOLUME_TO,
    SB_STOP_ALL_SOUNDS,
    SB_START_SOUND,
    SB_PLAY_SOUND_UNTIL_DONE
};
// ---> FIXED: Restored to normal events! <---
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
    CB_IF_ELSE,
    CB_REPEAT_UNTIL
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
    SENSB_TOUCHING_COLOR,
    SENSB_COLOR_IS_TOUCHING_COLOR,
    SENSB_MOUSE_X,
    SENSB_MOUSE_Y,
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
enum PenBlockType
{
    PB_ERASE_ALL = 0,
    PB_STAMP,
    PB_PEN_DOWN,
    PB_PEN_UP,
    PB_SET_COLOR_TO_PICKER,
    PB_CHANGE_ATTRIB_BY,
    PB_SET_ATTRIB_TO,
    PB_CHANGE_SIZE_BY,
    PB_SET_SIZE_TO
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
    BK_VARIABLES = 7,
    BK_PEN = 8,
    BK_MY_BLOCKS = 9  // Custom functions (My Blocks)
};

// --- My Blocks (Custom Functions) ---
enum MyBlocksBlockType
{
    MYB_DEFINE = 0,  // Hat block: defines a custom function
    MYB_CALL   = 1,  // Stack block: calls a custom function
    MYB_PARAM  = 2   // Reporter oval: reads a param value inside function body
};

enum CustomParamType
{
    CPARAM_NUMBER  = 0,
    CPARAM_TEXT    = 1,
    CPARAM_BOOLEAN = 2
};

struct CustomParam
{
    std::string    name;
    CustomParamType type;
    CustomParam() : name(""), type(CPARAM_NUMBER) {}
    CustomParam(const std::string &n, CustomParamType t) : name(n), type(t) {}
};

// One entry per user-defined function (global, shared across sprites in the project)
struct CustomFunctionDef
{
    std::string              name;
    std::vector<CustomParam> params; // max 3 params supported in UI
    CustomFunctionDef() = default;
    CustomFunctionDef(const std::string &n) : name(n) {}
};
enum BlockFieldType
{
    BFT_INT = 0,
    BFT_TEXT = 1,
    BFT_COLOR1,
    BFT_COLOR2,
    BFT_MESSAGE
};

struct BlockInstance
{
    int id = -1, subtype = 0, x = 0, y = 0, a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, opt = 0;
    BlockKind kind = BK_MOTION;
    std::string text, text2;
    int next_id = -1, parent_id = -1, child_id = -1, child2_id = -1, condition_id = -1;
    int arg0_id = -1, arg1_id = -1, arg2_id = -1;

    SDL_Color color1 = {255, 0, 0, 255};
    SDL_Color color2 = {0, 0, 255, 255};
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

enum EditTool
{
    TOOL_POINTER = 0,
    TOOL_BRUSH,
    TOOL_ERASER,
    TOOL_TEXT,
    TOOL_FILL,
    TOOL_RECT,
    TOOL_CIRCLE
};
enum ShapeType
{
    SHAPE_RECT = 0,
    SHAPE_CIRCLE,
    SHAPE_TEXT
};

struct GraphicShape
{
    ShapeType type;
    SDL_Rect rect;
    SDL_Color color;
    std::string text;
};

struct GraphicItem
{
    std::string name;
    std::string source_path;
    SDL_Texture *original_texture;
    SDL_Texture *texture;
    SDL_Texture *paint_layer;
    std::vector<GraphicShape> shapes;
    SDL_Texture *composed_texture;
    bool flip_h;
    bool flip_v;
    GraphicItem(std::string n, SDL_Texture *t, std::string sp = "") : name(n), source_path(sp), original_texture(t), texture(t), paint_layer(nullptr), composed_texture(nullptr), flip_h(false), flip_v(false) {}
};

typedef GraphicItem Costume;
typedef GraphicItem Backdrop;

struct SoundData
{
    std::string name;
    std::string source_path;
    Mix_Chunk *chunk;
    int volume;
    int prev_volume;
    SoundData(std::string n, Mix_Chunk *c, std::string sp = "") : name(n), source_path(sp), chunk(c), volume(100), prev_volume(100) {}
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
    int layer_order;
    SDL_Texture *texture;

    std::vector<SoundData> sounds;
    int selected_sound;

    std::vector<Costume> costumes;
    int selected_costume;

    bool pen_down;
    int pen_size;
    SDL_Color pen_color;
    int pen_color_val;
    int pen_saturation;
    int pen_brightness;

    std::vector<BlockInstance> blocks;
    std::vector<int> top_level_blocks;

    Sprite(std::string n, SDL_Texture *tex, std::string sp = "") : name(n), x(0), y(0), direction(90), visible(true), size(100), say_text(""), is_thinking(false), say_end_time(0), volume(100), draggable(true), layer_order(get_next_layer()), texture(tex), selected_sound(0), selected_costume(0), pen_down(false), pen_size(1), pen_color({15, 189, 140, 255}), pen_color_val(50), pen_saturation(100), pen_brightness(100)
    {
        costumes.push_back(Costume(n, tex, sp));
    }
    static int get_next_layer()
    {
        static int l = 0;
        return l++;
    }
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

    std::vector<std::string> messages;
    bool msg_modal_active;

    bool stage_drag_active;
    int stage_drag_off_x, stage_drag_off_y;
    bool ask_active;
    std::string ask_msg;
    std::string ask_reply;
    std::string global_answer;
    bool pen_extension_enabled;

    // --- Custom Functions (My Blocks) modal state ---
    std::vector<CustomFunctionDef> custom_functions; // project-wide function definitions
    bool func_modal_active;           // "Make a Block" dialog open
    int  func_modal_step;             // 0 = entering name, 1 = adding params
    std::string func_modal_name;      // function name being typed
    std::vector<CustomParam> func_modal_params; // params being built
    int  func_modal_param_type;       // selected param type for next add (CPARAM_*)
    std::string func_modal_param_name; // param name being typed

    bool editing_target_is_stage;
    EditTool active_tool;
    SDL_Color active_color;
    int active_shape_index;
    bool trigger_costume_import;
    bool new_confirm_active;

    AppState() : file_menu_open(false), file_menu_hover(-1), sprite_menu_open(false), backdrop_menu_open(false), current_tab(TAB_CODE), start_hover(false), stop_hover(false), running(false), mode(MODE_EDITOR), selected_sprite(0), add_sprite_hover(false), selected_backdrop(0), selected_tab(TAB_CODE), selected_category(0), project_name("Untitled"), drag(), next_block_id(1), active_input(INPUT_NONE), input_buffer(""), block_input(), variables({"my variable"}), variable_values({{"my variable", "0"}}), variable_visible({{"my variable", true}}), var_modal_active(false), messages({"message1"}), msg_modal_active(false), stage_drag_active(false), stage_drag_off_x(0), stage_drag_off_y(0), ask_active(false), ask_msg(""), ask_reply(""), global_answer(""), pen_extension_enabled(false), editing_target_is_stage(false), active_tool(TOOL_POINTER), active_color({0, 0, 0, 255}), active_shape_index(-1), trigger_costume_import(false),
        func_modal_active(false), func_modal_step(0), func_modal_name(""), func_modal_params(), func_modal_param_type(0), func_modal_param_name(""), new_confirm_active(false) {}
};

inline std::string copy_asset_to_project(std::string proj_name, std::string original_path)
{
    if (proj_name.empty())
        proj_name = "Untitled";
    std::string dir = "projects/" + proj_name + "/assets";
    std::filesystem::create_directories(dir);
    size_t slash = original_path.find_last_of('/');
    std::string fname = (slash == std::string::npos) ? original_path : original_path.substr(slash + 1);
    std::string target = dir + "/" + fname;
    int counter = 1;
    while (std::filesystem::exists(target))
    {
        size_t dot = fname.find_last_of('.');
        std::string base = (dot == std::string::npos) ? fname : fname.substr(0, dot);
        std::string ext = (dot == std::string::npos) ? "" : fname.substr(dot);
        target = dir + "/" + base + "_" + std::to_string(counter++) + ext;
    }
    std::filesystem::copy_file(original_path, target, std::filesystem::copy_options::overwrite_existing);
    return target;
}

inline void delete_asset_from_project(std::string path)
{
    if (path.empty())
        return;
    if (path.find("assets/") == 0 || path.find("assets\\") == 0)
        return;
    if (path.find("projects/") == 0 && std::filesystem::exists(path))
    {
        std::filesystem::remove(path);
    }
}

#endif