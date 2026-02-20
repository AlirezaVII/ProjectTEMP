#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "types.h"
#include "SDL.h"

void interpreter_trigger_flag(AppState &state);
void interpreter_trigger_key(AppState &state, SDL_Keycode key);
void interpreter_trigger_sprite_click(AppState &state);
void interpreter_trigger_message(AppState &state, int msg_opt);

void interpreter_stop_all(AppState &state);

// ---> NEW: Process running scripts every frame <---
void interpreter_tick(AppState &state);

#endif