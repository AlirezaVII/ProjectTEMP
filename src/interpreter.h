#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "types.h"

// Triggers any "when flag clicked" blocks in the workspace
void interpreter_trigger_flag(AppState &state);

// Stops execution
void interpreter_stop_all(AppState &state);

#endif