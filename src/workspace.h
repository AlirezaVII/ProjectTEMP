#ifndef WORKSPACE_H
#define WORKSPACE_H

#include "SDL.h"
#include "SDL_ttf.h"
#include "types.h"
#include "textures.h"
#include "config.h" // ---> FIXED: Included config.h for the Color struct <---

BlockInstance* workspace_find(AppState& state, int id);
const BlockInstance* workspace_find_const(const AppState& state, int id);

void workspace_draw(SDL_Renderer* r, TTF_Font* font, const Textures& tex, const AppState& state, const SDL_Rect& workspace_rect, Color bg);
bool workspace_handle_event(const SDL_Event& e, AppState& state, const SDL_Rect& workspace_rect, const SDL_Rect& palette_rect, TTF_Font* font);

int chain_height(const AppState& state, int root_id);
void workspace_layout_chain(AppState& state, int root_id);

int workspace_add_top_level(AppState& state, const BlockInstance& b);
int workspace_root_id(const AppState& state, int id);

void workspace_commit_active_input(AppState& state);

BlockInstance workspace_make_default(MotionBlockType type);
BlockInstance workspace_make_default_looks(LooksBlockType type);
BlockInstance workspace_make_default_sound(SoundBlockType type);
BlockInstance workspace_make_default_events(EventsBlockType type);
BlockInstance workspace_make_default_sensing(SensingBlockType type);
BlockInstance workspace_make_default_operators(OperatorsBlockType type);
BlockInstance workspace_make_default_variables(VariablesBlockType type, const std::string& var_name);

// ---> FIXED: THIS EXPORTS THE PEN BUILDER TO PALETTE.CPP <---
BlockInstance workspace_make_default_pen(PenBlockType type);

bool workspace_save_txt(const AppState& state, const char* path);
bool workspace_load_txt(AppState& state, const char* path);

#endif