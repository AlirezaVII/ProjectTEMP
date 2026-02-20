#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>

/* ---------------- Tabs ---------------- */
enum Tab {
    TAB_CODE     = 0,
    TAB_COSTUMES = 1,
    TAB_SOUNDS   = 2
};

/* ---------------- Active input ---------------- */
enum ActiveInput {
    INPUT_NONE = 0,
    INPUT_PROJECT_NAME,
    INPUT_SPRITE_NAME,
    INPUT_X,
    INPUT_Y,
    INPUT_DIRECTION,
    INPUT_SIZE,
    INPUT_BLOCK_FIELD
};

struct LegacySprite {
    std::string name;
    int x;
    int y;
    int direction;
    bool visible;
    int size;

    LegacySprite()
        : name("Sprite1"),
          x(0), y(0),
          direction(90),
          visible(true),
          size(100) {}
};

struct SpriteSt {
    std::string name;
    int x;
    int y;
    int dir;
    int size;

    SpriteSt()
        : name("Sprite1"),
          x(0), y(0),
          dir(90),
          size(100) {}
};

enum MotionBlockType { MB_MOVE_STEPS = 0, MB_TURN_RIGHT_DEG, MB_TURN_LEFT_DEG, MB_GO_TO_XY, MB_CHANGE_X_BY, MB_CHANGE_Y_BY, MB_POINT_IN_DIR, MB_GO_TO_TARGET };
enum GoToTarget { TARGET_RANDOM_POSITION = 0, TARGET_MOUSE_POINTER = 1 };
enum LooksBlockType { LB_SAY_FOR = 0, LB_SAY, LB_THINK_FOR, LB_THINK, LB_SWITCH_COSTUME_TO, LB_NEXT_COSTUME, LB_SWITCH_BACKDROP_TO, LB_NEXT_BACKDROP, LB_CHANGE_SIZE_BY, LB_SET_SIZE_TO, LB_SHOW, LB_HIDE, LB_GO_TO_LAYER, LB_GO_LAYERS };
enum SoundBlockType { SB_CHANGE_VOLUME_BY = 0, SB_SET_VOLUME_TO, SB_STOP_ALL_SOUNDS, SB_START_SOUND, SB_PLAY_SOUND_UNTIL_DONE };
enum EventsBlockType { EB_WHEN_FLAG_CLICKED = 0, EB_WHEN_KEY_PRESSED, EB_WHEN_SPRITE_CLICKED, EB_WHEN_I_RECEIVE, EB_BROADCAST };
enum ControlBlockType { CB_WAIT = 0, CB_REPEAT, CB_FOREVER, CB_IF, CB_WAIT_UNTIL, CB_IF_ELSE };
enum SensingBlockType { SENSB_TOUCHING = 0, SENSB_ASK_AND_WAIT, SENSB_KEY_PRESSED, SENSB_MOUSE_DOWN, SENSB_SET_DRAG_MODE, SENSB_COUNT };
// ADD THIS ENUM (Right after SensingBlockType is fine)
enum OperatorsBlockType {
    OP_ADD = 0, OP_SUB, OP_MUL, OP_DIV,
    OP_RANDOM,
    OP_GT, OP_LT, OP_EQ,
    OP_AND, OP_OR, OP_NOT,
    OP_JOIN, OP_LETTER_OF, OP_LENGTH_OF, OP_CONTAINS
};
// --- ADDED OPERATORS HERE ---
enum OperatorBlockType { OB_ADD = 0, OB_SUB, OB_MUL, OB_DIV, OB_GT, OB_LT, OB_EQ, OB_AND, OB_OR, OB_NOT, OB_JOIN, OB_LETTER_OF, OB_LENGTH_OF };

enum TouchingTarget { TOUCHING_MOUSE_POINTER = 0, TOUCHING_EDGE = 1, TOUCHING_SPRITE = 2 };
enum DragMode { DRAG_DRAGGABLE = 0, DRAG_NOT_DRAGGABLE = 1 };

// --- ADDED BK_OPERATORS = 6 ---
enum BlockKind { BK_MOTION = 0, BK_LOOKS = 1, BK_SOUND = 2, BK_EVENTS = 3, BK_CONTROL = 4, BK_SENSING = 5, BK_OPERATORS = 6 }; 
enum BlockFieldType { BFT_INT = 0, BFT_TEXT = 1 };

struct BlockInstance {
    int id = -1;
    BlockKind kind = BK_MOTION;
    int subtype = 0;
    int x = 0, y = 0;
    int a = 0, b = 0;
    int c = 0, d = 0, e = 0, f = 0; 
    int opt = 0;
    std::string text;
    
    /* AST Pointers */
    int next_id = -1;
    int parent_id = -1;
    int child_id = -1;      // First block inside C-shape (or IF)
    int child2_id = -1;     // First block inside ELSE shape
    int condition_id = -1;  // Hexagonal boolean block attached here
    
    // NEW AST Pointers for Reporter Blocks dropping into Inputs!
    int arg0_id = -1;
    int arg1_id = -1;
    int arg2_id = -1;
};

enum SnapType {
    SNAP_NONE = 0,
    SNAP_AFTER,       // Bottom notch
    SNAP_BEFORE,      // Top notch
    SNAP_INSIDE_1,    // Inside first C-shape mouth
    SNAP_INSIDE_2,    // Inside ELSE mouth
    SNAP_CONDITION,   // Hex slot
    // --- ADDED SNAP TYPES ---
    SNAP_INPUT_1,
    SNAP_INPUT_2,
    SNAP_INPUT_3
};

struct DragState {
    bool active;
    bool from_palette;
    BlockKind palette_kind;
    int palette_subtype;
    int dragged_block_id;
    int off_x; int off_y;
    int ghost_x; int ghost_y;
    int mouse_x; int mouse_y;
    bool snap_valid;
    bool snap_above; 
    int snap_x; int snap_y;
    int snap_target_id;
    SnapType snap_type;

    DragState() : active(false), from_palette(false), palette_kind(BK_MOTION), palette_subtype(0), dragged_block_id(-1), off_x(0), off_y(0), ghost_x(0), ghost_y(0), mouse_x(0), mouse_y(0), snap_valid(false), snap_above(false), snap_x(0), snap_y(0), snap_target_id(-1), snap_type(SNAP_NONE) {}
};

struct BlockInputState {
    int block_id;
    int field_index;
    BlockFieldType type;
    BlockInputState() : block_id(-1), field_index(0), type(BFT_INT) {}
};

struct AppState {
    bool file_menu_open;
    int file_menu_hover;
    bool sprite_menu_open;
    bool backdrop_menu_open;
    Tab current_tab;
    bool start_hover;
    bool stop_hover;
    bool running;
    LegacySprite sprite;
    std::vector<SpriteSt> sprites;
    int selected_sprite;
    int selected_tab;
    int selected_category;
    std::string project_name;
    DragState drag;
    std::vector<BlockInstance> blocks;
    std::vector<int> top_level_blocks;
    int next_block_id;
    ActiveInput active_input;
    std::string input_buffer;
    BlockInputState block_input;

    /* Stage Dragging State */
    bool stage_drag_active;
    int stage_drag_off_x;
    int stage_drag_off_y;

    AppState() : file_menu_open(false), file_menu_hover(-1), sprite_menu_open(false), backdrop_menu_open(false), current_tab(TAB_CODE), start_hover(false), stop_hover(false), running(false), sprite(), selected_sprite(0), selected_tab(TAB_CODE), selected_category(0), project_name("Untitled"), next_block_id(1), active_input(INPUT_NONE), stage_drag_active(false), stage_drag_off_x(0), stage_drag_off_y(0) {}
};

#endif