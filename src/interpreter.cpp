#include "interpreter.h"
#include "workspace.h"
#include "audio.h"
#include "SDL.h"
#include "config.h" // <--- CRITICAL FIX: Allows engine to see actual window dimensions!
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

void interpreter_trigger_message(AppState &state, int msg_opt);

static std::unordered_map<std::string, float> global_vars;

static void commit_active_typing(AppState &state)
{
    if (state.active_input == INPUT_BLOCK_FIELD)
    {
        workspace_commit_active_input(state);
        state.active_input = INPUT_NONE;
        state.input_buffer.clear();
    }
}

struct StackFrame
{
    int cur_node;
    int loop_block_id;
    int loop_count;
};

struct ScriptThread
{
    std::vector<StackFrame> stack;
    unsigned int wait_until;
    bool waiting_for_sound;
    bool waiting_for_ask;
};
static std::vector<ScriptThread> g_threads;

// ---> RESTORED: Translates backend Stage math to exact UI pixels <---
static void get_sprite_screen_rect(const AppState &state, int &cx, int &cy, int &w, int &h)
{
    int col_x = WINDOW_WIDTH - RIGHT_COLUMN_WIDTH;
    int col_h = WINDOW_HEIGHT - NAVBAR_HEIGHT;
    int stage_h = col_h * STAGE_HEIGHT_RATIO / 100;

    int margin = 8;
    int stage_area_x = col_x + margin;
    int stage_area_y = NAVBAR_HEIGHT + margin;
    int stage_area_w = RIGHT_COLUMN_WIDTH - margin * 2;
    int stage_area_h = stage_h - margin * 2;

    cx = stage_area_x + stage_area_w / 2 + state.sprite.x;
    cy = stage_area_y + stage_area_h / 2 - state.sprite.y;

    int base_w = 100; // Base size of the cat texture
    int base_h = 100;
    w = (base_w * state.sprite.size) / 100;
    h = (base_h * state.sprite.size) / 100;
}

static float eval_value(AppState &state, int block_id, float default_val, const std::string &text_val)
{
    if (block_id != -1)
    {
        BlockInstance *b = workspace_find(state, block_id);
        if (b && b->kind == BK_OPERATORS)
        {
            float a = eval_value(state, b->arg0_id, b->a, b->text);
            float b_val = eval_value(state, b->arg1_id, b->b, b->text2);
            if (b->subtype == OP_ADD)
                return a + b_val;
            if (b->subtype == OP_SUB)
                return a - b_val;
            if (b->subtype == OP_MUL)
                return a * b_val;
            if (b->subtype == OP_DIV)
                return (b_val != 0) ? (a / b_val) : 0;
        }
        if (b && b->kind == BK_VARIABLES && b->subtype == VB_VARIABLE)
            return global_vars[b->text];

        if (b && b->kind == BK_SENSING)
        {
            if (b->subtype == SENSB_ANSWER)
            {
                return std::atof(state.global_answer.c_str());
            }
            if (b->subtype == SENSB_DISTANCE_TO)
            {
                // ---> RESTORED: PERFECT PIXEL MATH <---
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                int cx, cy, w, h;
                get_sprite_screen_rect(state, cx, cy, w, h);
                float dx = mx - cx;
                float dy = my - cy;
                return std::sqrt(dx * dx + dy * dy);
            }
        }
    }
    if (!text_val.empty())
        return std::atof(text_val.c_str());
    return default_val;
}

static std::string eval_string(AppState &state, int block_id, const std::string &text_val)
{
    if (block_id != -1)
    {
        BlockInstance *b = workspace_find(state, block_id);
        if (b && b->kind == BK_VARIABLES && b->subtype == VB_VARIABLE)
        {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%g", global_vars[b->text]);
            return std::string(buf);
        }
        if (b && b->kind == BK_SENSING)
        {
            if (b->subtype == SENSB_ANSWER)
                return state.global_answer;
            if (b->subtype == SENSB_DISTANCE_TO)
            {
                char buf[32];
                std::snprintf(buf, sizeof(buf), "%g", eval_value(state, block_id, 0, ""));
                return std::string(buf);
            }
        }
        if (b && b->kind == BK_OPERATORS)
        {
            if (b->subtype == OP_JOIN)
                return eval_string(state, b->arg0_id, b->text) + eval_string(state, b->arg1_id, b->text2);
            if (b->subtype == OP_LETTER_OF)
            {
                std::string s = eval_string(state, b->arg1_id, b->text2);
                int idx = (int)eval_value(state, b->arg0_id, b->a, b->text) - 1;
                if (idx >= 0 && idx < (int)s.length())
                    return std::string(1, s[idx]);
                return "";
            }
            if (b->subtype == OP_LENGTH_OF)
                return std::to_string(eval_string(state, b->arg0_id, b->text).length());

            float val = eval_value(state, block_id, 0, "");
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%g", val);
            return std::string(buf);
        }
    }
    return text_val;
}

static bool eval_bool(AppState &state, int block_id)
{
    if (block_id == -1)
        return false;
    BlockInstance *b = workspace_find(state, block_id);
    if (!b)
        return false;

    if (b->kind == BK_OPERATORS)
    {
        float v0 = eval_value(state, b->arg0_id, b->a, b->text);
        float v1 = eval_value(state, b->arg1_id, b->b, b->text2);

        if (b->subtype == OP_GT)
            return v0 > v1;
        if (b->subtype == OP_LT)
            return v0 < v1;
        if (b->subtype == OP_EQ)
        {
            std::string s0 = eval_string(state, b->arg0_id, b->text);
            std::string s1 = eval_string(state, b->arg1_id, b->text2);
            if (s0 == s1 && !s0.empty())
                return true;
            return std::abs(v0 - v1) < 0.001f;
        }
        if (b->subtype == OP_AND)
            return eval_bool(state, b->arg0_id) && eval_bool(state, b->arg1_id);
        if (b->subtype == OP_OR)
            return eval_bool(state, b->arg0_id) || eval_bool(state, b->arg1_id);
        if (b->subtype == OP_NOT)
            return !eval_bool(state, b->arg0_id);
    }
    if (b->kind == BK_SENSING)
    {
        if (b->subtype == SENSB_KEY_PRESSED)
        {
            SDL_PumpEvents();
            const Uint8 *keys = SDL_GetKeyboardState(NULL);
            int opt = b->opt;
            bool pressed = false;

            if (opt == 0)
                pressed = keys[SDL_SCANCODE_SPACE];
            else if (opt == 1)
                pressed = keys[SDL_SCANCODE_UP];
            else if (opt == 2)
                pressed = keys[SDL_SCANCODE_DOWN];
            else if (opt == 3)
                pressed = keys[SDL_SCANCODE_LEFT];
            else if (opt == 4)
                pressed = keys[SDL_SCANCODE_RIGHT];
            else if (opt >= 5 && opt <= 30)
                pressed = keys[SDL_SCANCODE_A + (opt - 5)];
            else if (opt == 31)
                pressed = keys[SDL_SCANCODE_0];
            else if (opt >= 32 && opt <= 40)
                pressed = keys[SDL_SCANCODE_1 + (opt - 32)];

            return pressed;
        }
        if (b->subtype == SENSB_MOUSE_DOWN)
        {
            SDL_PumpEvents();
            return (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
        }
        if (b->subtype == SENSB_TOUCHING)
        {
            // ---> RESTORED: PIXEL PERFECT MOUSE HOVER MATH! <---
            if (b->opt == TOUCHING_MOUSE_POINTER)
            {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                int cx, cy, w, h;
                get_sprite_screen_rect(state, cx, cy, w, h);
                float half_w = w / 2.0f;
                float half_h = h / 2.0f;

                return (mx >= cx - half_w && mx <= cx + half_w &&
                        my >= cy - half_h && my <= cy + half_h);
            }
            else if (b->opt == TOUCHING_EDGE)
            {
                float half_w = (100 * state.sprite.size) / 200.0f;
                float half_h = (100 * state.sprite.size) / 200.0f;
                return (state.sprite.x - half_w <= -240 ||
                        state.sprite.x + half_w >= 240 ||
                        state.sprite.y - half_h <= -180 ||
                        state.sprite.y + half_h >= 180);
            }
            else if (b->opt == TOUCHING_SPRITE)
            {
                return false; // Engine currently only has 1 sprite, cannot touch itself!
            }
        }
    }
    return false;
}

void interpreter_tick(AppState &state)
{
    if (!state.running)
        return;

    for (size_t i = 0; i < g_threads.size();)
    {
        if (!state.running)
            break;

        if (SDL_GetTicks() < g_threads[i].wait_until)
        {
            i++;
            continue;
        }

        if (g_threads[i].waiting_for_sound)
        {
            if (audio_is_playing())
            {
                i++;
                continue;
            }
            else
            {
                g_threads[i].waiting_for_sound = false;
            }
        }

        if (g_threads[i].waiting_for_ask)
        {
            if (state.ask_active)
            {
                i++;
                continue;
            }
            else
            {
                g_threads[i].waiting_for_ask = false;
            }
        }

        bool yielded = false;

        while (!g_threads[i].stack.empty() && !yielded && state.running)
        {
            StackFrame &frame = g_threads[i].stack.back();
            int cur = frame.cur_node;

            if (cur == -1)
            {
                if (frame.loop_block_id != -1)
                {
                    BlockInstance *loop_b = workspace_find(state, frame.loop_block_id);
                    if (loop_b)
                    {
                        if (frame.loop_count > 0 || frame.loop_count == -1)
                        {
                            if (frame.loop_count > 0)
                                frame.loop_count--;
                            frame.cur_node = loop_b->child_id;
                            yielded = true;
                            break;
                        }
                    }
                }
                g_threads[i].stack.pop_back();
                continue;
            }

            BlockInstance *b = workspace_find(state, cur);
            if (!b)
            {
                frame.cur_node = -1;
                continue;
            }

            if (b->kind == BK_MOTION)
            {
                if (b->subtype == MB_MOVE_STEPS)
                {
                    float steps = eval_value(state, b->arg0_id, b->a, b->text);
                    float rad = (state.sprite.direction - 90.0f) * M_PI / 180.0f;
                    state.sprite.x += (int)(steps * std::cos(rad));
                    state.sprite.y -= (int)(steps * std::sin(rad));
                }
                else if (b->subtype == MB_TURN_RIGHT_DEG)
                    state.sprite.direction += (int)eval_value(state, b->arg0_id, b->a, b->text);
                else if (b->subtype == MB_TURN_LEFT_DEG)
                    state.sprite.direction -= (int)eval_value(state, b->arg0_id, b->a, b->text);
                else if (b->subtype == MB_GO_TO_XY)
                {
                    state.sprite.x = (int)eval_value(state, b->arg0_id, b->a, b->text);
                    state.sprite.y = (int)eval_value(state, b->arg1_id, b->b, b->text2);
                }
                else if (b->subtype == MB_CHANGE_X_BY)
                    state.sprite.x += (int)eval_value(state, b->arg0_id, b->a, b->text);
                else if (b->subtype == MB_CHANGE_Y_BY)
                    state.sprite.y += (int)eval_value(state, b->arg0_id, b->a, b->text);
                else if (b->subtype == MB_POINT_IN_DIR)
                    state.sprite.direction = (int)eval_value(state, b->arg0_id, b->a, b->text);
                else if (b->subtype == MB_GO_TO_TARGET)
                {
                    if (b->opt == TARGET_RANDOM_POSITION)
                    {
                        state.sprite.x = (std::rand() % 400) - 200;
                        state.sprite.y = (std::rand() % 300) - 150;
                    }
                    else if (b->opt == TARGET_MOUSE_POINTER)
                    {
                        int mx, my;
                        SDL_GetMouseState(&mx, &my);
                        state.sprite.x = mx - (1280 - 240);
                        state.sprite.y = 180 - my + 60;
                    }
                }
                frame.cur_node = b->next_id;
            }
            else if (b->kind == BK_LOOKS)
            {
                if (b->subtype == LB_SAY)
                {
                    state.sprite.say_text = eval_string(state, b->arg0_id, b->text);
                    state.sprite.is_thinking = false;
                    state.sprite.say_end_time = 0;
                    frame.cur_node = b->next_id;
                }
                else if (b->subtype == LB_THINK)
                {
                    state.sprite.say_text = eval_string(state, b->arg0_id, b->text);
                    state.sprite.is_thinking = true;
                    state.sprite.say_end_time = 0;
                    frame.cur_node = b->next_id;
                }
                else if (b->subtype == LB_SAY_FOR)
                {
                    state.sprite.say_text = eval_string(state, b->arg0_id, b->text);
                    state.sprite.is_thinking = false;
                    float sec = eval_value(state, b->arg1_id, b->a, b->text2);
                    state.sprite.say_end_time = SDL_GetTicks() + (unsigned int)(sec * 1000);
                    g_threads[i].wait_until = state.sprite.say_end_time;
                    frame.cur_node = b->next_id;
                    yielded = true;
                }
                else if (b->subtype == LB_THINK_FOR)
                {
                    state.sprite.say_text = eval_string(state, b->arg0_id, b->text);
                    state.sprite.is_thinking = true;
                    float sec = eval_value(state, b->arg1_id, b->a, b->text2);
                    state.sprite.say_end_time = SDL_GetTicks() + (unsigned int)(sec * 1000);
                    g_threads[i].wait_until = state.sprite.say_end_time;
                    frame.cur_node = b->next_id;
                    yielded = true;
                }
                else if (b->subtype == LB_CHANGE_SIZE_BY)
                {
                    state.sprite.size += (int)eval_value(state, b->arg0_id, b->a, b->text);
                    frame.cur_node = b->next_id;
                }
                else if (b->subtype == LB_SET_SIZE_TO)
                {
                    state.sprite.size = (int)eval_value(state, b->arg0_id, b->a, b->text);
                    frame.cur_node = b->next_id;
                }
                else if (b->subtype == LB_SHOW)
                {
                    state.sprite.visible = true;
                    frame.cur_node = b->next_id;
                }
                else if (b->subtype == LB_HIDE)
                {
                    state.sprite.visible = false;
                    frame.cur_node = b->next_id;
                }
            }
            else if (b->kind == BK_SOUND)
            {
                if (b->subtype == SB_CHANGE_VOLUME_BY)
                {
                    state.sprite.volume += (int)eval_value(state, b->arg0_id, b->a, b->text);
                    audio_set_volume(state.sprite.volume);
                    frame.cur_node = b->next_id;
                }
                else if (b->subtype == SB_SET_VOLUME_TO)
                {
                    state.sprite.volume = (int)eval_value(state, b->arg0_id, b->a, b->text);
                    audio_set_volume(state.sprite.volume);
                    frame.cur_node = b->next_id;
                }
                else if (b->subtype == SB_STOP_ALL_SOUNDS)
                {
                    audio_stop_all();
                    frame.cur_node = b->next_id;
                }
                else if (b->subtype == SB_START_SOUND)
                {
                    audio_play_meow();
                    frame.cur_node = b->next_id;
                }
                else if (b->subtype == SB_PLAY_SOUND_UNTIL_DONE)
                {
                    audio_play_meow();
                    g_threads[i].waiting_for_sound = true;
                    frame.cur_node = b->next_id;
                    yielded = true;
                }
            }
            else if (b->kind == BK_CONTROL)
            {
                if (b->subtype == CB_WAIT)
                {
                    float sec = eval_value(state, b->arg0_id, b->a, b->text);
                    g_threads[i].wait_until = SDL_GetTicks() + (unsigned int)(sec * 1000);
                    frame.cur_node = b->next_id;
                    yielded = true;
                }
                else if (b->subtype == CB_REPEAT)
                {
                    int count = (int)eval_value(state, b->arg0_id, b->a, b->text);
                    frame.cur_node = b->next_id;
                    if (count > 0 && b->child_id != -1)
                        g_threads[i].stack.push_back({b->child_id, b->id, count - 1});
                }
                else if (b->subtype == CB_FOREVER)
                {
                    frame.cur_node = b->next_id;
                    if (b->child_id != -1)
                        g_threads[i].stack.push_back({b->child_id, b->id, -1});
                    else
                        yielded = true;
                }
                else if (b->subtype == CB_IF)
                {
                    bool cond = eval_bool(state, b->condition_id);
                    frame.cur_node = b->next_id;
                    if (cond && b->child_id != -1)
                        g_threads[i].stack.push_back({b->child_id, -1, 0});
                }
                else if (b->subtype == CB_IF_ELSE)
                {
                    bool cond = eval_bool(state, b->condition_id);
                    frame.cur_node = b->next_id;
                    if (cond && b->child_id != -1)
                        g_threads[i].stack.push_back({b->child_id, -1, 0});
                    else if (!cond && b->child2_id != -1)
                        g_threads[i].stack.push_back({b->child2_id, -1, 0});
                }
                else if (b->subtype == CB_WAIT_UNTIL)
                {
                    bool cond = eval_bool(state, b->condition_id);
                    if (!cond)
                        yielded = true;
                    else
                        frame.cur_node = b->next_id;
                }
            }
            else if (b->kind == BK_SENSING)
            {
                if (b->subtype == SENSB_ASK_AND_WAIT)
                {
                    state.ask_active = true;
                    state.ask_msg = eval_string(state, b->arg0_id, b->text);
                    state.ask_reply = "";
                    g_threads[i].waiting_for_ask = true;
                    frame.cur_node = b->next_id;
                    yielded = true;
                }
                else if (b->subtype == SENSB_SET_DRAG_MODE)
                {
                    state.sprite.draggable = (b->opt == 0);
                    frame.cur_node = b->next_id;
                }
                else
                {
                    frame.cur_node = b->next_id;
                }
            }
            else if (b->kind == BK_VARIABLES)
            {
                if (b->subtype == VB_SET && b->opt >= 0 && b->opt < (int)state.variables.size())
                    global_vars[state.variables[b->opt]] = eval_value(state, b->arg0_id, b->a, b->text);
                if (b->subtype == VB_CHANGE && b->opt >= 0 && b->opt < (int)state.variables.size())
                    global_vars[state.variables[b->opt]] += eval_value(state, b->arg0_id, b->a, b->text);
                frame.cur_node = b->next_id;
            }
            else if (b->kind == BK_EVENTS)
            {
                if (b->subtype == EB_BROADCAST)
                    interpreter_trigger_message(state, b->opt);
                frame.cur_node = b->next_id;
            }
            else
            {
                frame.cur_node = b->next_id;
            }
        }

        if (i < g_threads.size())
        {
            if (g_threads[i].stack.empty())
                g_threads.erase(g_threads.begin() + i);
            else
                i++;
        }
    }
}

void interpreter_trigger_flag(AppState &state)
{
    commit_active_typing(state);
    state.running = true;
    g_threads.clear();
    for (int root_id : state.top_level_blocks)
    {
        BlockInstance *b = workspace_find(state, root_id);
        if (b && b->kind == BK_EVENTS && b->subtype == EB_WHEN_FLAG_CLICKED)
            g_threads.push_back({{{b->next_id, -1, 0}}, 0, false, false});
    }
}

void interpreter_trigger_key(AppState &state, SDL_Keycode sym)
{
    commit_active_typing(state);
    int opt = -1;
    if (sym == SDLK_SPACE)
        opt = 0;
    else if (sym == SDLK_UP)
        opt = 1;
    else if (sym == SDLK_DOWN)
        opt = 2;
    else if (sym == SDLK_LEFT)
        opt = 3;
    else if (sym == SDLK_RIGHT)
        opt = 4;
    else if (sym >= SDLK_a && sym <= SDLK_z)
        opt = 5 + (sym - SDLK_a);
    else if (sym >= SDLK_0 && sym <= SDLK_9)
        opt = 31 + (sym - SDLK_0);
    if (opt == -1)
        return;
    for (int root_id : state.top_level_blocks)
    {
        BlockInstance *b = workspace_find(state, root_id);
        if (b && b->kind == BK_EVENTS && b->subtype == EB_WHEN_KEY_PRESSED && b->opt == opt)
            g_threads.push_back({{{b->next_id, -1, 0}}, 0, false, false});
    }
}

void interpreter_trigger_sprite_click(AppState &state)
{
    commit_active_typing(state);
    for (int root_id : state.top_level_blocks)
    {
        BlockInstance *b = workspace_find(state, root_id);
        if (b && b->kind == BK_EVENTS && b->subtype == EB_WHEN_SPRITE_CLICKED)
            g_threads.push_back({{{b->next_id, -1, 0}}, 0, false, false});
    }
}

void interpreter_trigger_message(AppState &state, int msg_opt)
{
    commit_active_typing(state);
    for (int root_id : state.top_level_blocks)
    {
        BlockInstance *b = workspace_find(state, root_id);
        if (b && b->kind == BK_EVENTS && b->subtype == EB_WHEN_I_RECEIVE && b->opt == msg_opt)
            g_threads.push_back({{{b->next_id, -1, 0}}, 0, false, false});
    }
}

void interpreter_stop_all(AppState &state)
{
    commit_active_typing(state);
    state.running = false;
    g_threads.clear();
    state.sprite.say_text = "";
    state.sprite.say_end_time = 0;
    state.ask_active = false;
    audio_stop_all();
}