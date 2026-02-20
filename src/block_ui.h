#ifndef BLOCK_UI_H
#define BLOCK_UI_H

#include "SDL.h"
#include "SDL_ttf.h"
#include "types.h"
#include "textures.h"
#include "config.h"

/* Layout constants for Motion blocks */
struct MotionBlockMetrics {
    int h;
    int radius;
    int notch_w;
    int notch_h;
    int notch_x;
    int notch_y;
    int overlap; /* vertical overlap between connected blocks */
};

MotionBlockMetrics motion_block_metrics();
int motion_block_width(MotionBlockType type);

/*
    OLD API (keeps drag/palette intact)
*/
void motion_block_draw(SDL_Renderer *r, TTF_Font *font, const Textures &tex,
                       MotionBlockType type, int x, int y,
                       int a, int b, GoToTarget target,
                       bool ghost, Color panel_bg,
                       int selected_field);

/*
    NEW API (workspace typing overlay support)
*/
void motion_block_draw(SDL_Renderer *r, TTF_Font *font, const Textures &tex,
                       MotionBlockType type, int x, int y,
                       int a, int b, GoToTarget target,
                       bool ghost, Color panel_bg,
                       int selected_field,
                       const char *override_field0_text,
                       const char *override_field1_text);

/* returns:
   0/1 => click on numeric capsule field index
   -2  => click on dropdown (MB_GO_TO_TARGET)
   -1  => body / none
*/
int motion_block_hittest_field(TTF_Font *font, MotionBlockType type,
                               int x, int y,
                               int a, int b, GoToTarget target,
                               int px, int py);

SDL_Rect motion_block_rect(MotionBlockType type, int x, int y);

/* ---------------- Looks UI (NO reporters) ---------------- */

int looks_block_width(LooksBlockType type);
SDL_Rect looks_block_rect(LooksBlockType type, int x, int y);

/*
  Looks draw:
  - text used by say/think blocks
  - a used by seconds/size/layers
  - opt used by dropdowns (costume/backdrop/front-back/forward-backward)
*/
void looks_block_draw(SDL_Renderer *r, TTF_Font *font,
                      LooksBlockType type,
                      int x, int y,
                      const std::string &text,
                      int a, int b, int opt,
                      bool ghost, Color panel_bg,
                      int selected_field,
                      const char *override_field0_text,
                      const char *override_field1_text);

/* returns:
   0/1 => click on input capsule (field0/field1)
   -2  => click on dropdown (costume/backdrop/front/back/forward/backward)
   -1  => body / none
*/
int looks_block_hittest_field(TTF_Font *font,
                              LooksBlockType type,
                              int x, int y,
                              const std::string &text,
                              int a, int b, int opt,
                              int px, int py);

/* ---------------- Sound UI ---------------- */

int sound_block_width(SoundBlockType type);

/* NOTE: a/opt included for API symmetry (future: dynamic sizing) */
SDL_Rect sound_block_rect(SoundBlockType type, int x, int y, int a, int opt);

/*
  Sound draw:
  - a is numeric input (volume delta or absolute volume)
  - opt is dropdown selection (only Meow for now)
*/
void sound_block_draw(SDL_Renderer *r, TTF_Font *font,
                      SoundBlockType type,
                      int x, int y,
                      int a, int opt,
                      bool ghost, Color panel_bg,
                      int selected_field,
                      const char *override_field0_text);

/* returns:
   0   => click on numeric capsule (field0)
   -2  => click on dropdown (Meow)
   -1  => body / none
*/
int sound_block_hittest_field(TTF_Font *font,
                              SoundBlockType type,
                              int x, int y,
                              int a, int opt,
                              int px, int py);

int events_block_width(EventsBlockType type);
SDL_Rect events_block_rect(EventsBlockType type, int x, int y, int opt);

void events_block_draw(SDL_Renderer *r, TTF_Font *font, const Textures &tex,
                       EventsBlockType type,
                       int x, int y,
                       int opt,
                       bool ghost, Color panel_bg,
                       int selected_field);

int events_block_hittest_field(TTF_Font *font,
                               EventsBlockType type,
                               int x, int y, int opt,
                               int px, int py);

/* ---------------- Sensing ---------------- */

int  sensing_block_width(SensingBlockType type);

SDL_Rect sensing_block_rect(SensingBlockType type, int x, int y);

void sensing_block_draw(SDL_Renderer *r, TTF_Font *font,
                        SensingBlockType type,
                        int x, int y,
                        int a, int b, int opt,
                        int r1, int g1, int b1,
                        int r2, int g2, int b2,
                        const std::string &text,
                        bool ghost, Color panel_bg,
                        int selected_field,
                        const char *override_field0_text);

int  sensing_block_hittest_field(TTF_Font *font,
                                 SensingBlockType type,
                                 int x, int y,
                                 int opt,
                                 int px, int py);

// اضافه کنید به انتهای block_ui.h (قبل از #endif):

// --- Sensing Boolean blocks ---
int  sensing_boolean_block_width(SensingBlockType type);
SDL_Rect sensing_boolean_block_rect(SensingBlockType type, int x, int y, int opt);
void sensing_boolean_block_draw(SDL_Renderer *r, TTF_Font *font,
                                SensingBlockType type,
                                int x, int y,
                                int opt, int a, int b, int c,
                                int d, int e, int f,
                                bool ghost, Color panel_bg,
                                int selected_field,
                                const char *override_field0_text);
int  sensing_boolean_block_hittest_field(TTF_Font *font,
                                         SensingBlockType type,
                                         int x, int y,
                                         int opt, int a, int b, int c,
                                         int d, int e, int f,
                                         int px, int py);

// --- Sensing Stack blocks ---
int  sensing_stack_block_width(SensingBlockType type);
SDL_Rect sensing_stack_block_rect(SensingBlockType type, int x, int y, int opt);
void sensing_stack_block_draw(SDL_Renderer *r, TTF_Font *font,
                              SensingBlockType type,
                              int x, int y,
                              const std::string &text, int opt,
                              bool ghost, Color panel_bg,
                              int selected_field,
                              const char *override_field0_text);
int  sensing_stack_block_hittest_field(TTF_Font *font,
                                       SensingBlockType type,
                                       int x, int y,
                                       const std::string &text, int opt,
                                       int px, int py);

#endif
