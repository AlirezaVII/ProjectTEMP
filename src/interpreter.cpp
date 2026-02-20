#include "interpreter.h"
#include "workspace.h"
#include "SDL.h" 
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

// Forward declaration so broadcast can trigger messages
void interpreter_trigger_message(AppState &state, int msg_opt);

// ---> NEW FIX: FORCE SAVE ANY ACTIVE TYPING BEFORE RUNNING <---
static void commit_active_typing(AppState& state) {
    if (state.active_input == INPUT_BLOCK_FIELD) {
        workspace_commit_active_input(state);
        state.active_input = INPUT_NONE;
        state.input_buffer.clear();
    }
}

// Evaluates a capsule input (returns the nested reporter block value, OR the typed text/number)
static float eval_value(AppState& state, int block_id, float default_val, const std::string& text_val) {
    if (block_id != -1) {
        BlockInstance* b = workspace_find(state, block_id);
        if (b && b->kind == BK_OPERATORS) {
            float a = eval_value(state, b->arg0_id, b->a, b->text);
            float b_val = eval_value(state, b->arg1_id, b->b, b->text2);
            if (b->subtype == OP_ADD) return a + b_val;
            if (b->subtype == OP_SUB) return a - b_val;
            if (b->subtype == OP_MUL) return a * b_val;
            if (b->subtype == OP_DIV) return (b_val != 0) ? (a / b_val) : 0;
        }
    }
    if (!text_val.empty()) return std::atof(text_val.c_str());
    return default_val;
}

// Executes a connected stack of blocks linearly
static void execute_chain(AppState& state, int start_id) {
    int cur = start_id;
    while (cur != -1) {
        BlockInstance* b = workspace_find(state, cur);
        if (!b) break;

        if (b->kind == BK_MOTION) {
            if (b->subtype == MB_MOVE_STEPS) {
                float steps = eval_value(state, b->arg0_id, b->a, b->text);
                float rad = (state.sprite.direction - 90.0f) * M_PI / 180.0f;
                state.sprite.x += (int)(steps * std::cos(rad));
                state.sprite.y -= (int)(steps * std::sin(rad));
            } 
            else if (b->subtype == MB_TURN_RIGHT_DEG) {
                float deg = eval_value(state, b->arg0_id, b->a, b->text);
                state.sprite.direction += (int)deg;
            } 
            else if (b->subtype == MB_TURN_LEFT_DEG) {
                float deg = eval_value(state, b->arg0_id, b->a, b->text);
                state.sprite.direction -= (int)deg;
            } 
            else if (b->subtype == MB_GO_TO_XY) {
                float vx = eval_value(state, b->arg0_id, b->a, b->text);
                float vy = eval_value(state, b->arg1_id, b->b, b->text2);
                state.sprite.x = (int)vx;
                state.sprite.y = (int)vy;
            } 
            else if (b->subtype == MB_CHANGE_X_BY) {
                state.sprite.x += (int)eval_value(state, b->arg0_id, b->a, b->text);
            } 
            else if (b->subtype == MB_CHANGE_Y_BY) {
                state.sprite.y += (int)eval_value(state, b->arg0_id, b->a, b->text);
            } 
            else if (b->subtype == MB_POINT_IN_DIR) {
                state.sprite.direction = (int)eval_value(state, b->arg0_id, b->a, b->text);
            }
            else if (b->subtype == MB_GO_TO_TARGET) {
                if (b->opt == TARGET_RANDOM_POSITION) {
                    state.sprite.x = (std::rand() % 400) - 200;
                    state.sprite.y = (std::rand() % 300) - 150;
                } else if (b->opt == TARGET_MOUSE_POINTER) {
                    int mx, my;
                    SDL_GetMouseState(&mx, &my);
                    state.sprite.x = mx - (1280 - 240); 
                    state.sprite.y = 180 - my + 60;
                }
            }
        } 
        else if (b->kind == BK_LOOKS) {
            if (b->subtype == LB_CHANGE_SIZE_BY) {
                state.sprite.size += (int)eval_value(state, b->arg0_id, b->a, b->text);
            } else if (b->subtype == LB_SET_SIZE_TO) {
                state.sprite.size = (int)eval_value(state, b->arg0_id, b->a, b->text);
            } else if (b->subtype == LB_SHOW) {
                state.sprite.visible = true;
            } else if (b->subtype == LB_HIDE) {
                state.sprite.visible = false;
            }
        }
        else if (b->kind == BK_EVENTS) {
            if (b->subtype == EB_BROADCAST) {
                interpreter_trigger_message(state, b->opt);
            }
        }
        
        cur = b->next_id;
    }
}

// 1. Green Flag
void interpreter_trigger_flag(AppState &state) {
    commit_active_typing(state); // <--- FIX APPLIED HERE
    state.running = true;
    for (int root_id : state.top_level_blocks) {
        BlockInstance* b = workspace_find(state, root_id);
        if (b && b->kind == BK_EVENTS && b->subtype == EB_WHEN_FLAG_CLICKED) {
            execute_chain(state, b->next_id);
        }
    }
}

// 2. Keyboard Press
void interpreter_trigger_key(AppState &state, SDL_Keycode sym) {
    commit_active_typing(state); // <--- FIX APPLIED HERE
    int opt = -1;
    if (sym == SDLK_SPACE) opt = 0;
    else if (sym == SDLK_UP) opt = 1;
    else if (sym == SDLK_DOWN) opt = 2;
    else if (sym == SDLK_LEFT) opt = 3;
    else if (sym == SDLK_RIGHT) opt = 4;
    else if (sym >= SDLK_a && sym <= SDLK_z) opt = 5 + (sym - SDLK_a);
    else if (sym >= SDLK_0 && sym <= SDLK_9) opt = 31 + (sym - SDLK_0);

    if (opt == -1) return;

    for (int root_id : state.top_level_blocks) {
        BlockInstance* b = workspace_find(state, root_id);
        if (b && b->kind == BK_EVENTS && b->subtype == EB_WHEN_KEY_PRESSED && b->opt == opt) {
            execute_chain(state, b->next_id);
        }
    }
}

// 3. Sprite Clicked
void interpreter_trigger_sprite_click(AppState &state) {
    commit_active_typing(state); // <--- FIX APPLIED HERE
    for (int root_id : state.top_level_blocks) {
        BlockInstance* b = workspace_find(state, root_id);
        if (b && b->kind == BK_EVENTS && b->subtype == EB_WHEN_SPRITE_CLICKED) {
            execute_chain(state, b->next_id);
        }
    }
}

// 4. When I Receive (Broadcast message)
void interpreter_trigger_message(AppState &state, int msg_opt) {
    commit_active_typing(state); // <--- FIX APPLIED HERE
    for (int root_id : state.top_level_blocks) {
        BlockInstance* b = workspace_find(state, root_id);
        if (b && b->kind == BK_EVENTS && b->subtype == EB_WHEN_I_RECEIVE && b->opt == msg_opt) {
            execute_chain(state, b->next_id);
        }
    }
}

void interpreter_stop_all(AppState &state) {
    commit_active_typing(state);
    state.running = false;
}