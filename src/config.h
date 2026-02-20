#ifndef CONFIG_H
#define CONFIG_H

/* Window (set at runtime from display) */
extern int WINDOW_WIDTH;
extern int WINDOW_HEIGHT;

/* Layout constants */
static const int NAVBAR_HEIGHT        = 70;
static const int TAB_BAR_HEIGHT       = 38;
static const int RIGHT_COLUMN_WIDTH   = 480;
static const int CATEGORY_WIDTH       = 120;
static const int PALETTE_WIDTH        = 430;
static const int STAGE_HEIGHT_RATIO   = 40;   /* percent of right column */

/* Navbar */
static const int NAVBAR_LOGO_SIZE = 70;
static const int NAVBAR_LOGO_WIDTH  = 100;
static const int NAVBAR_LOGO_HEIGHT     = 70;
static const int NAVBAR_LOGO_MARGIN   = 20;
static const int FILE_BTN_WIDTH       = 50;
static const int PROJECT_INPUT_WIDTH  = 180;
static const int PROJECT_INPUT_HEIGHT = 26;
static const int FILEMENU_WIDTH       = 160;
static const int FILEMENU_ITEM_H      = 30;

/* Tab bar */
static const int TAB_HPADDING         = 12;
static const int START_BTN_RADIUS     = 14;
static const int STOP_BTN_RADIUS      = 14;

/* Settings */
static const int SETTINGS_INPUT_W     = 55;
static const int SETTINGS_INPUT_H     = 26;
static const int VIS_ICON_SIZE        = 24;

/* Colors */
struct Color {
    int r, g, b;
};

/* Navbar */
static const Color COL_NAVBAR_BG           = {58, 73, 117};
static const Color COL_NAVBAR_TEXT         = {255, 255, 255};
static const Color COL_NAVBAR_FILE_HOVER   = {47, 60, 100};
static const Color COL_NAVBAR_INPUT_BG     = {47, 60, 100};
static const Color COL_NAVBAR_INPUT_BORDER = {80, 95, 140};
static const Color COL_NAVBAR_INPUT_TEXT   = {220, 220, 255};

/* File menu */
static const Color COL_FILEMENU_BG         = {255, 255, 255};
static const Color COL_FILEMENU_HOVER      = {230, 235, 245};
static const Color COL_FILEMENU_TEXT       = {50, 50, 50};
static const Color COL_FILEMENU_BORDER     = {200, 200, 200};

/* Tab bar */
static const Color COL_TAB_BAR_BG         = {244, 244, 244};
static const Color COL_TAB_ACTIVE_BG      = {255, 255, 255};
static const Color COL_TAB_ACTIVE_TEXT    = {95, 149, 247};
static const Color COL_TAB_INACTIVE_TEXT  = {145, 145, 145};
static const Color COL_SEPARATOR          = {220, 220, 220};

/* Start / stop */
static const Color COL_START_BTN          = {40, 180, 60};
static const Color COL_START_HOVER        = {50, 200, 70};
static const Color COL_STOP_BTN           = {220, 60, 50};
static const Color COL_STOP_HOVER         = {240, 80, 70};

/* Categories */
static const Color COL_CAT_BG             = {244, 244, 244};
static const Color COL_CAT_SELECTED_BG    = {230, 235, 255};
static const Color COL_CAT_TEXT           = {80, 80, 80};
static const Color COL_CAT_SELECTED_TEXT  = {60, 60, 60};

/* Category dot colors */
static const Color COL_CAT_MOTION         = {76, 151, 255};
static const Color COL_CAT_LOOKS          = {153, 102, 255};
static const Color COL_CAT_SOUND          = {207, 99, 207};
static const Color COL_CAT_EVENTS         = {255, 191, 38};
static const Color COL_CAT_CONTROL        = {255, 171, 25};
static const Color COL_CAT_SENSING        = {90, 188, 216};
static const Color COL_CAT_OPERATORS      = {89, 192, 89};
static const Color COL_CAT_VARIABLES      = {255, 140, 26};
static const Color COL_CAT_MYBLOCKS       = {255, 102, 128};

/* Palette */
static const Color COL_PALETTE_BG         = {255, 255, 255};
static const Color COL_PALETTE_TEXT       = {255, 255, 255};

/* Canvas / drag area */
static const Color COL_CANVAS_BG          = {255, 255, 255};
static const Color COL_CANVAS_GRID        = {240, 240, 240};

/* Stage */
static const Color COL_STAGE_BG           = {255, 255, 255};
static const Color COL_STAGE_BORDER       = {200, 200, 200};
static const Color COL_STAGE_TEXT         = {100, 100, 100};

/* Settings */
static const Color COL_SETTINGS_BG        = {244, 244, 244};
static const Color COL_SETTINGS_BORDER    = {220, 220, 220};
static const Color COL_SETTINGS_TEXT      = {80, 80, 80};
static const Color COL_SETTINGS_INPUT_BG      = {255, 255, 255};
static const Color COL_SETTINGS_INPUT_BORDER  = {200, 200, 200};
static const Color COL_SETTINGS_INPUT_ACTIVE  = {95, 149, 247};
static const Color COL_SETTINGS_INPUT_TEXT    = {50, 50, 50};

static const Color COL_WHITE = {255, 255, 255};

#endif
