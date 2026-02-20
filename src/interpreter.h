#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "types.h"
#include "SDL.h"

// Trigger functions for all the Events blocks!
void interpreter_trigger_flag(AppState &state);
void interpreter_trigger_key(AppState &state, SDL_Keycode key);
void interpreter_trigger_sprite_click(AppState &state);
void interpreter_trigger_message(AppState &state, int msg_opt);

// Stops execution
void interpreter_stop_all(AppState &state);

#endif