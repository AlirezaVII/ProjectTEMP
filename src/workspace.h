#ifndef WORKSPACE_H
#define WORKSPACE_H

#include "SDL.h"
#include "SDL_ttf.h"
#include "types.h"
#include "textures.h"
#include "config.h"

/* Create a new block instance with default parameters for the given type */
BlockInstance workspace_make_default(MotionBlockType type);

/* Looks defaults */
BlockInstance workspace_make_default_looks(LooksBlockType type);

/* Sound defaults */
BlockInstance workspace_make_default_sound(SoundBlockType type);

/* Events defaults */
BlockInstance workspace_make_default_events(EventsBlockType type);

/* Sensing defaults */
BlockInstance workspace_make_default_sensing(SensingBlockType type);

/* Add a new top-level block to workspace, returns its id */
int workspace_add_top_level(AppState &state, const BlockInstance &b);

/* Find block by id, returns pointer or nullptr */
BlockInstance* workspace_find(AppState &state, int id);
const BlockInstance* workspace_find_const(const AppState &state, int id);

/* Return the root id of a chain containing 'id' (walk parent links) */
int workspace_root_id(const AppState &state, int id);

/* Layout positions for a chain starting at root_id (uses each block's x/y as base for root) */
void workspace_layout_chain(AppState &state, int root_id);

/* Draw all blocks in workspace (including snap shadow + dragged ghost) */
void workspace_draw(SDL_Renderer *r, TTF_Font *font, const Textures &tex,
                    const AppState &state, const SDL_Rect &workspace_rect, Color bg);

/* Workspace event handling (drag/drop + input editing + dropdown) */
bool workspace_handle_event(const SDL_Event &e, AppState &state,
                            const SDL_Rect &workspace_rect,
                            const SDL_Rect &palette_rect,
                            TTF_Font *font);

/* Commit input buffer to the currently edited block field (if any) */
void workspace_commit_active_input(AppState &state);

/* Save/load workspace to/from a text file (not used now) */
bool workspace_save_txt(const AppState &state, const char *path);
bool workspace_load_txt(AppState &state, const char *path);

#endif
