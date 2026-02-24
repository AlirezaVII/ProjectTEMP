#ifndef BLOCK_UI_H
#define BLOCK_UI_H

#include "SDL.h"
#include "SDL_ttf.h"
#include "types.h"
#include "textures.h"
#include "config.h"

struct MotionBlockMetrics {
    int h;
    int radius;
    int notch_w;
    int notch_h;
    int notch_x;
    int notch_y;
    int overlap; 
};

MotionBlockMetrics motion_block_metrics();
int motion_block_width(MotionBlockType type);

void motion_block_draw(SDL_Renderer *r, TTF_Font *font, const Textures &tex,
                       MotionBlockType type, int x, int y,
                       int a, int b, GoToTarget target,
                       bool ghost, Color panel_bg,
                       int selected_field);

void motion_block_draw(SDL_Renderer *r, TTF_Font *font, const Textures &tex,
                       MotionBlockType type, int x, int y,
                       int a, int b, GoToTarget target,
                       bool ghost, Color panel_bg,
                       int selected_field,
                       const char *override_field0_text,
                       const char *override_field1_text,
                       int extra_w0 = 0, int extra_w1 = 0);

int motion_block_hittest_field(TTF_Font *font, MotionBlockType type,
                               int x, int y,
                               int a, int b, GoToTarget target,
                               int px, int py,
                               int extra_w0 = 0, int extra_w1 = 0);

SDL_Rect motion_block_rect(MotionBlockType type, int x, int y);

int looks_block_width(LooksBlockType type);
SDL_Rect looks_block_rect(LooksBlockType type, int x, int y);
void looks_block_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state, LooksBlockType type, int x, int y, const std::string &text, int a, int b, int opt, bool ghost, Color panel_bg, int selected_field, const char *override_field0_text, const char *override_field1_text, int extra_w0 = 0, int extra_w1 = 0);
int looks_block_hittest_field(TTF_Font *font, const AppState &state, LooksBlockType type, int x, int y, const std::string &text, int a, int b, int opt, int px, int py);

int sound_block_width(SoundBlockType type);
SDL_Rect sound_block_rect(SoundBlockType type, int x, int y, int a, int opt);
void sound_block_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state, SoundBlockType type, int x, int y, int a, int opt, bool ghost, Color panel_bg, int selected_field, const char *override_field0_text, int extra_w = 0);
int sound_block_hittest_field(TTF_Font *font, const AppState &state, SoundBlockType type, int x, int y, int a, int opt, int px, int py);

int events_block_width(EventsBlockType type);
SDL_Rect events_block_rect(EventsBlockType type, int x, int y, int opt);
void events_block_draw(SDL_Renderer *r, TTF_Font *font, const Textures &tex, const AppState &state,
                       EventsBlockType type, int x, int y, int opt,
                       bool ghost, Color panel_bg, int selected_field);
                       
int events_block_hittest_field(TTF_Font *font, const AppState &state, EventsBlockType type, int x, int y, int opt, int px, int py);

int  sensing_block_width(SensingBlockType type);
SDL_Rect sensing_block_rect(SensingBlockType type, int x, int y);
void sensing_block_draw(SDL_Renderer *r, TTF_Font *font, SensingBlockType type, int x, int y, int a, int b, int opt, int r1, int g1, int b1, int r2, int g2, int b2, const std::string &text, bool ghost, Color panel_bg, int selected_field, const char *override_field0_text);
int  sensing_block_hittest_field(TTF_Font *font, SensingBlockType type, int x, int y, int opt, int px, int py);

int  sensing_boolean_block_width(SensingBlockType type);
SDL_Rect sensing_boolean_block_rect(SensingBlockType type, int x, int y, int opt);
void sensing_boolean_block_draw(SDL_Renderer *r, TTF_Font *font, SensingBlockType type, int x, int y, int opt, int a, int b, int c, int d, int e, int f, bool ghost, Color panel_bg, int selected_field, const char *override_field0_text);
int  sensing_boolean_block_hittest_field(TTF_Font *font, SensingBlockType type, int x, int y, int opt, int a, int b, int c, int d, int e, int f, int px, int py);

int  sensing_stack_block_width(SensingBlockType type);
SDL_Rect sensing_stack_block_rect(SensingBlockType type, int x, int y, int opt);
void sensing_stack_block_draw(SDL_Renderer *r, TTF_Font *font, SensingBlockType type, int x, int y, const std::string &text, int opt, bool ghost, Color panel_bg, int selected_field, const char *override_field0_text);
int  sensing_stack_block_hittest_field(TTF_Font *font, SensingBlockType type, int x, int y, const std::string &text, int opt, int px, int py);

int control_block_width(ControlBlockType type);
SDL_Rect control_block_rect(ControlBlockType type, int x, int y, int inner1_h, int inner2_h);
void control_block_draw(SDL_Renderer *r, TTF_Font *font, ControlBlockType type, int x, int y, int inner1_h, int inner2_h, int a, bool has_condition, bool ghost, Color panel_bg, int selected_field, const char *override_field0_text);
int control_block_hittest_field(TTF_Font *font, ControlBlockType type, int x, int y, int inner1_h, int inner2_h, int a, int px, int py);

int operators_block_width(OperatorsBlockType type);
SDL_Rect operators_block_rect(OperatorsBlockType type, int x, int y);
void operators_block_draw(SDL_Renderer *r, TTF_Font *font, OperatorsBlockType type, int x, int y, const std::string &str_a, const std::string &str_b, int a, int b, bool ghost, Color panel_bg, int selected_field, const char *override_field0_text, const char *override_field1_text);
int operators_block_hittest_field(TTF_Font *font, OperatorsBlockType type, int x, int y, int px, int py);
void operators_block_draw_dynamic(SDL_Renderer *r, TTF_Font *font, OperatorsBlockType type, int x, int y, int total_w, const std::string &str_a, const std::string &str_b, int cap0_w, int cap1_w, bool ghost, Color panel_bg, int selected_field, const char *override_field0_text, const char *override_field1_text);
int operators_block_hittest_dynamic(TTF_Font *font, OperatorsBlockType type, int x, int y, int total_w, int cap0_w, int cap1_w, int px, int py);

SDL_Rect variables_block_rect(VariablesBlockType type, int x, int y, const std::string& var_name);
void variables_block_draw(SDL_Renderer *r, TTF_Font *font, const AppState& state, VariablesBlockType type, int x, int y, const std::string& var_name, const std::string& input_text, int input_num, int opt, bool ghost, int selected_field, const char *override_field0_text);
int variables_block_hittest_field(TTF_Font *font, const AppState& state, VariablesBlockType type, int x, int y, const std::string& var_name, int opt, int px, int py);

SDL_Rect sensing_reporter_block_rect(SensingBlockType type, int x, int y);
void sensing_reporter_block_draw(SDL_Renderer *r, TTF_Font *font, SensingBlockType type, int x, int y, bool ghost);
int sensing_reporter_block_hittest_field(TTF_Font *font, SensingBlockType type, int x, int y, int px, int py);

// ---> FIXED: THIS EXPORTS THE PEN DRAWING FUNCTIONS <---
int pen_block_width(PenBlockType type);
SDL_Rect pen_block_rect(PenBlockType type, int x, int y);
void pen_block_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state,
                    PenBlockType type, int x, int y,
                    int a, int opt, bool ghost, Color panel_bg,
                    int selected_field, const char *override_field0_text,
                    int extra_w = 0);
int pen_block_hittest_field(TTF_Font *font, PenBlockType type, int x, int y, int opt, int px, int py);

#endif