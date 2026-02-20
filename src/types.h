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
    /* workspace block field editing (int OR text depending on context) */
    INPUT_BLOCK_FIELD
};

/* ---------------- Legacy single sprite (for main.cpp compatibility) ----------------
   main.cpp expects:
     state.sprite.name
     state.sprite.x
     state.sprite.y
     state.sprite.direction
*/
struct LegacySprite {
    std::string name;
    int x;
    int y;
    int direction;
    bool visible;

    LegacySprite()
        : name("Sprite1"),
          x(0), y(0),
          direction(90),
          visible(true) {}
};


/* ---------------- Sprite list (used elsewhere in project) ---------------- */
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

/* ---------------- Motion ---------------- */
enum MotionBlockType {
    MB_MOVE_STEPS = 0,
    MB_TURN_RIGHT_DEG,
    MB_TURN_LEFT_DEG,
    MB_GO_TO_XY,
    MB_CHANGE_X_BY,
    MB_CHANGE_Y_BY,
    MB_POINT_IN_DIR,
    MB_GO_TO_TARGET /* dropdown: random position / mouse pointer */
};

enum GoToTarget {
    TARGET_RANDOM_POSITION = 0,
    TARGET_MOUSE_POINTER   = 1
};

/* ---------------- Looks ---------------- */
enum LooksBlockType {
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
    LB_GO_TO_LAYER,  /* dropdown front/back */
    LB_GO_LAYERS     /* dropdown forward/backward + (n) */
};

/* ---------------- Sound ---------------- */
enum SoundBlockType {
    SB_CHANGE_VOLUME_BY = 0,
    SB_SET_VOLUME_TO,
    SB_STOP_ALL_SOUNDS,
    SB_START_SOUND,
    SB_PLAY_SOUND_UNTIL_DONE
};

/* ---------------- Events ---------------- */
enum EventsBlockType {
    EB_WHEN_FLAG_CLICKED = 0,
    EB_WHEN_KEY_PRESSED,
    EB_WHEN_SPRITE_CLICKED,
    EB_WHEN_I_RECEIVE,
    EB_BROADCAST
};
/* ---------------- Sensing ---------------- */
enum SensingBlockType {
    SENSB_TOUCHING = 0,
    SENSB_ASK_AND_WAIT,
    SENSB_KEY_PRESSED,
    SENSB_MOUSE_DOWN,
    SENSB_SET_DRAG_MODE,
    SENSB_COUNT
};

enum TouchingTarget {
    TOUCHING_MOUSE_POINTER = 0,
    TOUCHING_EDGE          = 1,
    TOUCHING_SPRITE        = 2
};

enum DragMode {
    DRAG_DRAGGABLE     = 0,
    DRAG_NOT_DRAGGABLE = 1
};

enum BlockKind {
    BK_MOTION = 0,
    BK_LOOKS  = 1,
    BK_SOUND  = 2,
    BK_EVENTS = 3,
    BK_SENSING = 5
};

enum BlockFieldType {
    BFT_INT  = 0,
    BFT_TEXT = 1
};

/*
  Unified block instance for Motion / Looks / Sound stack blocks.
  - kind/subtype select which family & which block.
  - a/b are numeric params.
  - opt is dropdown selection.
*/
struct BlockInstance {
    int id = -1;
    BlockKind kind = BK_MOTION;
    int subtype = 0;
    int x = 0, y = 0;
    int a = 0, b = 0;
    int c = 0, d = 0, e = 0, f = 0;  // ← c,d,e,f اضافه شود
    int opt = 0;
    std::string text;
    int next_id = -1;
    int parent_id = -1;
};

struct DragState {
    bool active;
    bool from_palette;

    /* palette drag: stack blocks */
    BlockKind palette_kind;
    int palette_subtype;

    /* if from_workspace */
    int dragged_block_id; /* root of dragged chain */

    /* mouse offset from block origin for smooth dragging */
    int off_x;
    int off_y;

    /* current ghost position */
    int ghost_x;
    int ghost_y;

    /* last mouse position (needed for drop checks) */
    int mouse_x;
    int mouse_y;

    /* snap preview (ONLY for stack blocks) */
    bool snap_valid;
    bool snap_above; // true => insert before target (snap to top), false => snap below (append)
    int snap_x;
    int snap_y;
    int snap_target_id;

    DragState()
        : active(false),
          from_palette(false),
          palette_kind(BK_MOTION),
          palette_subtype(0),
          dragged_block_id(-1),
          off_x(0), off_y(0),
          ghost_x(0), ghost_y(0),
          mouse_x(0), mouse_y(0),
          snap_valid(false),
          snap_above(false),
          snap_x(0), snap_y(0),
          snap_target_id(-1) {}
};

struct BlockInputState {
    int block_id;
    int field_index;
    BlockFieldType type;

    BlockInputState()
        : block_id(-1),
          field_index(0),
          type(BFT_INT) {}
};

struct AppState {
    /* --- legacy fields used by main.cpp / navbar.cpp / tab_bar.cpp --- */
    bool file_menu_open;
    int file_menu_hover;
    /* sprite / backdrop panel */
    bool sprite_menu_open;
    bool backdrop_menu_open;

    Tab current_tab;
    bool start_hover;
    bool stop_hover;
    bool running;

    LegacySprite sprite;

    /* sprites */
    std::vector<SpriteSt> sprites;
    int selected_sprite;

    /* tabs (new code may use this) */
    int selected_tab;

    /* categories in Code tab */
    int selected_category;

    /* project name */
    std::string project_name;

    /* drag/drop state */
    DragState drag;

    /* blocks in workspace */
    std::vector<BlockInstance> blocks;
    std::vector<int> top_level_blocks;
    int next_block_id;

    /* input handling */
    ActiveInput active_input;
    std::string input_buffer;

    /* workspace block-field input */
    BlockInputState block_input;

    AppState()
        : file_menu_open(false),
          file_menu_hover(-1),
          current_tab(TAB_CODE),
          start_hover(false),
          stop_hover(false),
          running(false),
          sprite(),
          selected_sprite(0),
          selected_tab(TAB_CODE),
          selected_category(0),
          project_name("Untitled"),
          next_block_id(1),
          active_input(INPUT_NONE) {}
};


#endif
