#include "interpreter.h"
#include "workspace.h"
#include "audio.h" // <--- INCLUDED AUDIO MANAGER
#include "SDL.h" 
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

void interpreter_trigger_message(AppState &state, int msg_opt);

static void commit_active_typing(AppState& state) {
    if (state.active_input == INPUT_BLOCK_FIELD) {
        workspace_commit_active_input(state);
        state.active_input = INPUT_NONE;
        state.input_buffer.clear();
    }
}

// ---> ADDED "WAITING_FOR_SOUND" FLAG TO THREADS <---
struct ScriptThread {
    int cur_node;
    unsigned int wait_until;
    bool waiting_for_sound; 
};
static std::vector<ScriptThread> g_threads;

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

static std::string eval_string(AppState& state, int block_id, const std::string& text_val) {
    if (block_id != -1) {
        BlockInstance* b = workspace_find(state, block_id);
        if (b && b->kind == BK_OPERATORS) {
            if (b->subtype == OP_JOIN) return eval_string(state, b->arg0_id, b->text) + eval_string(state, b->arg1_id, b->text2);
            if (b->subtype == OP_LETTER_OF) {
                std::string s = eval_string(state, b->arg1_id, b->text2);
                int idx = (int)eval_value(state, b->arg0_id, b->a, b->text) - 1; 
                if (idx >= 0 && idx < (int)s.length()) return std::string(1, s[idx]);
                return "";
            }
            if (b->subtype == OP_LENGTH_OF) return std::to_string(eval_string(state, b->arg0_id, b->text).length());
            
            float val = eval_value(state, block_id, 0, "");
            char buf[32]; std::snprintf(buf, sizeof(buf), "%g", val); return std::string(buf);
        }
    }
    return text_val;
}

void interpreter_tick(AppState &state) {
    if (!state.running) return;

    for (size_t i = 0; i < g_threads.size(); ) {
        if (SDL_GetTicks() < g_threads[i].wait_until) {
            i++; continue; 
        }

        // ---> CHECK IF WE ARE BLOCKED BY A SOUND PLAYING <---
        if (g_threads[i].waiting_for_sound) {
            if (audio_is_playing()) {
                i++; continue; // Still playing! Yield thread.
            } else {
                g_threads[i].waiting_for_sound = false; // Finished!
            }
        }

        int cur = g_threads[i].cur_node;
        bool yielded = false; 

        while (cur != -1 && !yielded) {
            BlockInstance* b = workspace_find(state, cur);
            if (!b) { cur = -1; break; }

            if (b->kind == BK_MOTION) {
                if (b->subtype == MB_MOVE_STEPS) {
                    float steps = eval_value(state, b->arg0_id, b->a, b->text);
                    float rad = (state.sprite.direction - 90.0f) * M_PI / 180.0f;
                    state.sprite.x += (int)(steps * std::cos(rad));
                    state.sprite.y -= (int)(steps * std::sin(rad));
                } else if (b->subtype == MB_TURN_RIGHT_DEG) {
                    state.sprite.direction += (int)eval_value(state, b->arg0_id, b->a, b->text);
                } else if (b->subtype == MB_TURN_LEFT_DEG) {
                    state.sprite.direction -= (int)eval_value(state, b->arg0_id, b->a, b->text);
                } else if (b->subtype == MB_GO_TO_XY) {
                    state.sprite.x = (int)eval_value(state, b->arg0_id, b->a, b->text);
                    state.sprite.y = (int)eval_value(state, b->arg1_id, b->b, b->text2);
                } else if (b->subtype == MB_CHANGE_X_BY) {
                    state.sprite.x += (int)eval_value(state, b->arg0_id, b->a, b->text);
                } else if (b->subtype == MB_CHANGE_Y_BY) {
                    state.sprite.y += (int)eval_value(state, b->arg0_id, b->a, b->text);
                } else if (b->subtype == MB_POINT_IN_DIR) {
                    state.sprite.direction = (int)eval_value(state, b->arg0_id, b->a, b->text);
                } else if (b->subtype == MB_GO_TO_TARGET) {
                    if (b->opt == TARGET_RANDOM_POSITION) {
                        state.sprite.x = (std::rand() % 400) - 200; state.sprite.y = (std::rand() % 300) - 150;
                    } else if (b->opt == TARGET_MOUSE_POINTER) {
                        int mx, my; SDL_GetMouseState(&mx, &my);
                        state.sprite.x = mx - (1280 - 240); state.sprite.y = 180 - my + 60;
                    }
                }
                cur = b->next_id;
            } 
            else if (b->kind == BK_LOOKS) {
                if (b->subtype == LB_SAY) {
                    state.sprite.say_text = eval_string(state, b->arg0_id, b->text);
                    state.sprite.is_thinking = false; state.sprite.say_end_time = 0;
                    cur = b->next_id;
                } else if (b->subtype == LB_THINK) {
                    state.sprite.say_text = eval_string(state, b->arg0_id, b->text);
                    state.sprite.is_thinking = true; state.sprite.say_end_time = 0;
                    cur = b->next_id;
                } else if (b->subtype == LB_SAY_FOR) {
                    state.sprite.say_text = eval_string(state, b->arg0_id, b->text);
                    state.sprite.is_thinking = false;
                    float sec = eval_value(state, b->arg1_id, b->a, b->text2);
                    state.sprite.say_end_time = SDL_GetTicks() + (unsigned int)(sec * 1000);
                    g_threads[i].wait_until = state.sprite.say_end_time;
                    cur = b->next_id; yielded = true; 
                } else if (b->subtype == LB_THINK_FOR) {
                    state.sprite.say_text = eval_string(state, b->arg0_id, b->text);
                    state.sprite.is_thinking = true;
                    float sec = eval_value(state, b->arg1_id, b->a, b->text2);
                    state.sprite.say_end_time = SDL_GetTicks() + (unsigned int)(sec * 1000);
                    g_threads[i].wait_until = state.sprite.say_end_time;
                    cur = b->next_id; yielded = true;
                } else if (b->subtype == LB_CHANGE_SIZE_BY) {
                    state.sprite.size += (int)eval_value(state, b->arg0_id, b->a, b->text); cur = b->next_id;
                } else if (b->subtype == LB_SET_SIZE_TO) {
                    state.sprite.size = (int)eval_value(state, b->arg0_id, b->a, b->text); cur = b->next_id;
                } else if (b->subtype == LB_SHOW) {
                    state.sprite.visible = true; cur = b->next_id;
                } else if (b->subtype == LB_HIDE) {
                    state.sprite.visible = false; cur = b->next_id;
                }
            }
            // ---> NEW: ALL SOUND EXECUTION LOGIC <---
            else if (b->kind == BK_SOUND) {
                if (b->subtype == SB_CHANGE_VOLUME_BY) {
                    state.sprite.volume += (int)eval_value(state, b->arg0_id, b->a, b->text);
                    audio_set_volume(state.sprite.volume);
                    cur = b->next_id;
                } else if (b->subtype == SB_SET_VOLUME_TO) {
                    state.sprite.volume = (int)eval_value(state, b->arg0_id, b->a, b->text);
                    audio_set_volume(state.sprite.volume);
                    cur = b->next_id;
                } else if (b->subtype == SB_STOP_ALL_SOUNDS) {
                    audio_stop_all();
                    cur = b->next_id;
                } else if (b->subtype == SB_START_SOUND) {
                    audio_play_meow();
                    cur = b->next_id;
                } else if (b->subtype == SB_PLAY_SOUND_UNTIL_DONE) {
                    audio_play_meow();
                    g_threads[i].waiting_for_sound = true; // YIELD UNTIL SOUND FINISHES
                    cur = b->next_id; yielded = true;
                }
            }
            else if (b->kind == BK_CONTROL) {
                if (b->subtype == CB_WAIT) {
                    float sec = eval_value(state, b->arg0_id, b->a, b->text);
                    g_threads[i].wait_until = SDL_GetTicks() + (unsigned int)(sec * 1000);
                    cur = b->next_id; yielded = true; 
                } else {
                    cur = b->next_id;
                }
            }
            else if (b->kind == BK_EVENTS) {
                if (b->subtype == EB_BROADCAST) interpreter_trigger_message(state, b->opt);
                cur = b->next_id;
            } else {
                cur = b->next_id;
            }
        }

        g_threads[i].cur_node = cur;
        if (cur == -1) g_threads.erase(g_threads.begin() + i); 
        else i++;
    }
}

void interpreter_trigger_flag(AppState &state) {
    commit_active_typing(state); 
    state.running = true;
    g_threads.clear();
    for (int root_id : state.top_level_blocks) {
        BlockInstance* b = workspace_find(state, root_id);
        if (b && b->kind == BK_EVENTS && b->subtype == EB_WHEN_FLAG_CLICKED) {
            g_threads.push_back({b->next_id, 0, false});
        }
    }
}

void interpreter_trigger_key(AppState &state, SDL_Keycode sym) {
    commit_active_typing(state); 
    int opt = -1;
    if (sym == SDLK_SPACE) opt = 0; else if (sym == SDLK_UP) opt = 1; else if (sym == SDLK_DOWN) opt = 2; else if (sym == SDLK_LEFT) opt = 3; else if (sym == SDLK_RIGHT) opt = 4; else if (sym >= SDLK_a && sym <= SDLK_z) opt = 5 + (sym - SDLK_a); else if (sym >= SDLK_0 && sym <= SDLK_9) opt = 31 + (sym - SDLK_0);
    if (opt == -1) return;

    for (int root_id : state.top_level_blocks) {
        BlockInstance* b = workspace_find(state, root_id);
        if (b && b->kind == BK_EVENTS && b->subtype == EB_WHEN_KEY_PRESSED && b->opt == opt) {
            g_threads.push_back({b->next_id, 0, false});
        }
    }
}

void interpreter_trigger_sprite_click(AppState &state) {
    commit_active_typing(state); 
    for (int root_id : state.top_level_blocks) {
        BlockInstance* b = workspace_find(state, root_id);
        if (b && b->kind == BK_EVENTS && b->subtype == EB_WHEN_SPRITE_CLICKED) {
            g_threads.push_back({b->next_id, 0, false});
        }
    }
}

void interpreter_trigger_message(AppState &state, int msg_opt) {
    commit_active_typing(state); 
    for (int root_id : state.top_level_blocks) {
        BlockInstance* b = workspace_find(state, root_id);
        if (b && b->kind == BK_EVENTS && b->subtype == EB_WHEN_I_RECEIVE && b->opt == msg_opt) {
            g_threads.push_back({b->next_id, 0, false});
        }
    }
}

void interpreter_stop_all(AppState &state) {
    commit_active_typing(state);
    state.running = false;
    g_threads.clear();
    state.sprite.say_text = "";
    state.sprite.say_end_time = 0;
    audio_stop_all(); // KILLS SOUNDS IMMEDIATELY
}