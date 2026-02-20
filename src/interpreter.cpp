#include "interpreter.h"
#include "workspace.h"
#include "audio.h"
#include "SDL.h"
#include "config.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

void interpreter_trigger_message(AppState &state, int msg_opt);

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
    std::string sprite_name;
};
static std::vector<ScriptThread> g_threads;

static BlockInstance *interpreter_find_block(Sprite &spr, int id)
{
    for (auto &b : spr.blocks)
        if (b.id == id)
            return &b;
    return nullptr;
}

static void get_sprite_screen_rect(Sprite &spr, int &cx, int &cy, int &w, int &h)
{
    int col_x = WINDOW_WIDTH - RIGHT_COLUMN_WIDTH;
    int col_h = WINDOW_HEIGHT - NAVBAR_HEIGHT;
    int stage_h = col_h * STAGE_HEIGHT_RATIO / 100;
    int margin = 8;
    int stage_area_x = col_x + margin;
    int stage_area_y = NAVBAR_HEIGHT + margin;
    int stage_area_w = RIGHT_COLUMN_WIDTH - margin * 2;
    int stage_area_h = stage_h - margin * 2;
    cx = stage_area_x + stage_area_w / 2 + spr.x;
    cy = stage_area_y + stage_area_h / 2 - spr.y;
    int base_w = 100;
    int base_h = 100;
    w = (base_w * spr.size) / 100;
    h = (base_h * spr.size) / 100;
}

static float eval_value(AppState &state, Sprite &spr, int block_id, float default_val, const std::string &text_val);
static std::string eval_string(AppState &state, Sprite &spr, int block_id, const std::string &text_val);
static bool eval_bool(AppState &state, Sprite &spr, int block_id);

static int compare_scratch(const std::string &s0, const std::string &s1)
{
    if (s0.empty() && s1.empty())
        return 0;
    std::string ts0 = s0.empty() ? "0" : s0;
    std::string ts1 = s1.empty() ? "0" : s1;
    char *end0;
    char *end1;
    float v0 = std::strtof(ts0.c_str(), &end0);
    float v1 = std::strtof(ts1.c_str(), &end1);
    bool num0 = (end0 != ts0.c_str() && *end0 == '\0');
    bool num1 = (end1 != ts1.c_str() && *end1 == '\0');
    if (num0 && num1)
    {
        if (std::abs(v0 - v1) < 0.0001f)
            return 0;
        return (v0 < v1) ? -1 : 1;
    }
    std::string ls0 = ts0;
    std::transform(ls0.begin(), ls0.end(), ls0.begin(), ::tolower);
    std::string ls1 = ts1;
    std::transform(ls1.begin(), ls1.end(), ls1.begin(), ::tolower);
    if (ls0 == ls1)
        return 0;
    return (ls0 < ls1) ? -1 : 1;
}

static float eval_value(AppState &state, Sprite &spr, int block_id, float default_val, const std::string &text_val)
{
    if (block_id != -1)
    {
        BlockInstance *b = interpreter_find_block(spr, block_id);
        if (!b)
            return default_val;
        if (b->kind == BK_OPERATORS)
        {
            if (b->subtype == OP_ADD)
                return eval_value(state, spr, b->arg0_id, b->a, b->text) + eval_value(state, spr, b->arg1_id, b->b, b->text2);
            if (b->subtype == OP_SUB)
                return eval_value(state, spr, b->arg0_id, b->a, b->text) - eval_value(state, spr, b->arg1_id, b->b, b->text2);
            if (b->subtype == OP_MUL)
                return eval_value(state, spr, b->arg0_id, b->a, b->text) * eval_value(state, spr, b->arg1_id, b->b, b->text2);
            if (b->subtype == OP_DIV)
            {
                float denom = eval_value(state, spr, b->arg1_id, b->b, b->text2);
                return (denom != 0) ? (eval_value(state, spr, b->arg0_id, b->a, b->text) / denom) : 0;
            }
            if (b->subtype == OP_GT || b->subtype == OP_LT || b->subtype == OP_EQ || b->subtype == OP_AND || b->subtype == OP_OR || b->subtype == OP_NOT)
                return eval_bool(state, spr, block_id) ? 1.0f : 0.0f;
            if (b->subtype == OP_JOIN || b->subtype == OP_LETTER_OF || b->subtype == OP_LENGTH_OF)
                return std::atof(eval_string(state, spr, block_id, "").c_str());
        }
        if (b->kind == BK_VARIABLES && b->subtype == VB_VARIABLE)
            return std::atof(state.variable_values[b->text].c_str());
        if (b->kind == BK_SENSING)
        {
            if (b->subtype == SENSB_ANSWER)
                return std::atof(state.global_answer.c_str());
            if (b->subtype == SENSB_DISTANCE_TO)
            {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                int cx, cy, w, h;
                get_sprite_screen_rect(spr, cx, cy, w, h);
                float dx = mx - cx;
                float dy = my - cy;
                return std::sqrt(dx * dx + dy * dy);
            }
            if (b->subtype == SENSB_TOUCHING || b->subtype == SENSB_KEY_PRESSED || b->subtype == SENSB_MOUSE_DOWN)
                return eval_bool(state, spr, block_id) ? 1.0f : 0.0f;
        }
    }
    if (!text_val.empty())
        return std::atof(text_val.c_str());
    return default_val;
}

static std::string eval_string(AppState &state, Sprite &spr, int block_id, const std::string &text_val)
{
    if (block_id != -1)
    {
        BlockInstance *b = interpreter_find_block(spr, block_id);
        if (!b)
            return text_val;
        if (b->kind == BK_VARIABLES && b->subtype == VB_VARIABLE)
            return state.variable_values[b->text];
        if (b->kind == BK_SENSING)
        {
            if (b->subtype == SENSB_ANSWER)
                return state.global_answer;
            if (b->subtype == SENSB_DISTANCE_TO)
            {
                float val = eval_value(state, spr, block_id, 0, "");
                char buf[32];
                std::snprintf(buf, sizeof(buf), "%g", val);
                return std::string(buf);
            }
            if (b->subtype == SENSB_TOUCHING || b->subtype == SENSB_KEY_PRESSED || b->subtype == SENSB_MOUSE_DOWN)
                return eval_bool(state, spr, block_id) ? "true" : "false";
        }
        if (b->kind == BK_OPERATORS)
        {
            if (b->subtype == OP_JOIN)
                return eval_string(state, spr, b->arg0_id, b->text) + eval_string(state, spr, b->arg1_id, b->text2);
            if (b->subtype == OP_LETTER_OF)
            {
                std::string s = eval_string(state, spr, b->arg1_id, b->text2);
                int idx = (int)eval_value(state, spr, b->arg0_id, b->a, b->text) - 1;
                if (idx >= 0 && idx < (int)s.length())
                    return std::string(1, s[idx]);
                return "";
            }
            if (b->subtype == OP_LENGTH_OF)
                return std::to_string(eval_string(state, spr, b->arg0_id, b->text).length());
            if (b->subtype == OP_GT || b->subtype == OP_LT || b->subtype == OP_EQ || b->subtype == OP_AND || b->subtype == OP_OR || b->subtype == OP_NOT)
                return eval_bool(state, spr, block_id) ? "true" : "false";
            float val = eval_value(state, spr, block_id, 0, "");
            if (val == std::floor(val))
                return std::to_string((int)val);
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%g", val);
            return std::string(buf);
        }
    }
    return text_val;
}

static bool eval_bool(AppState &state, Sprite &spr, int block_id)
{
    if (block_id == -1)
        return false;
    BlockInstance *b = interpreter_find_block(spr, block_id);
    if (!b)
        return false;
    if (b->kind == BK_OPERATORS)
    {
        if (b->subtype == OP_GT)
            return compare_scratch(eval_string(state, spr, b->arg0_id, b->text), eval_string(state, spr, b->arg1_id, b->text2)) > 0;
        if (b->subtype == OP_LT)
            return compare_scratch(eval_string(state, spr, b->arg0_id, b->text), eval_string(state, spr, b->arg1_id, b->text2)) < 0;
        if (b->subtype == OP_EQ)
            return compare_scratch(eval_string(state, spr, b->arg0_id, b->text), eval_string(state, spr, b->arg1_id, b->text2)) == 0;
        if (b->subtype == OP_AND)
            return eval_bool(state, spr, b->arg0_id) && eval_bool(state, spr, b->arg1_id);
        if (b->subtype == OP_OR)
            return eval_bool(state, spr, b->arg0_id) || eval_bool(state, spr, b->arg1_id);
        if (b->subtype == OP_NOT)
            return !eval_bool(state, spr, b->arg0_id);
        std::string s = eval_string(state, spr, block_id, "");
        std::string ls = s;
        std::transform(ls.begin(), ls.end(), ls.begin(), ::tolower);
        if (ls == "true")
            return true;
        if (ls == "false")
            return false;
        return std::atof(s.c_str()) != 0.0f;
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
            if (b->opt == TOUCHING_MOUSE_POINTER)
            {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                int cx, cy, w, h;
                get_sprite_screen_rect(spr, cx, cy, w, h);
                float half_w = w / 2.0f;
                float half_h = h / 2.0f;
                return (mx >= cx - half_w && mx <= cx + half_w && my >= cy - half_h && my <= cy + half_h);
            }
            else if (b->opt == TOUCHING_EDGE)
            {
                float half_w = (100 * spr.size) / 200.0f;
                float half_h = (100 * spr.size) / 200.0f;
                return (spr.x - half_w <= -240 || spr.x + half_w >= 240 || spr.y - half_h <= -180 || spr.y + half_h >= 180);
            }
            else if (b->opt == TOUCHING_SPRITE)
                return false;
        }
    }
    if (b->kind == BK_VARIABLES || (b->kind == BK_SENSING && (b->subtype == SENSB_ANSWER || b->subtype == SENSB_DISTANCE_TO)))
    {
        std::string s = eval_string(state, spr, block_id, "");
        std::string ls = s;
        std::transform(ls.begin(), ls.end(), ls.begin(), ::tolower);
        if (ls == "true")
            return true;
        if (ls == "false")
            return false;
        return std::atof(s.c_str()) != 0.0f;
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
                g_threads[i].waiting_for_sound = false;
        }
        if (g_threads[i].waiting_for_ask)
        {
            if (state.ask_active)
            {
                i++;
                continue;
            }
            else
                g_threads[i].waiting_for_ask = false;
        }

        Sprite *spr_ptr = nullptr;
        for (auto &s : state.sprites)
            if (s.name == g_threads[i].sprite_name)
            {
                spr_ptr = &s;
                break;
            }
        if (!spr_ptr)
        {
            g_threads.erase(g_threads.begin() + i);
            continue;
        }
        Sprite &spr = *spr_ptr;

        bool yielded = false;
        while (!g_threads[i].stack.empty() && !yielded && state.running)
        {
            StackFrame &frame = g_threads[i].stack.back();
            int cur = frame.cur_node;
            if (cur == -1)
            {
                if (frame.loop_block_id != -1)
                {
                    BlockInstance *loop_b = interpreter_find_block(spr, frame.loop_block_id);
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

            BlockInstance *b = interpreter_find_block(spr, cur);
            if (!b)
            {
                frame.cur_node = -1;
                continue;
            }

            if (b->kind == BK_MOTION)
            {
                if (b->subtype == MB_MOVE_STEPS)
                {
                    float steps = eval_value(state, spr, b->arg0_id, b->a, b->text);
                    float rad = (spr.direction - 90.0f) * M_PI / 180.0f;
                    spr.x += (int)(steps * std::cos(rad));
                    spr.y -= (int)(steps * std::sin(rad));
                }
                else if (b->subtype == MB_TURN_RIGHT_DEG)
                    spr.direction += (int)eval_value(state, spr, b->arg0_id, b->a, b->text);
                else if (b->subtype == MB_TURN_LEFT_DEG)
                    spr.direction -= (int)eval_value(state, spr, b->arg0_id, b->a, b->text);
                else if (b->subtype == MB_GO_TO_XY)
                {
                    spr.x = (int)eval_value(state, spr, b->arg0_id, b->a, b->text);
                    spr.y = (int)eval_value(state, spr, b->arg1_id, b->b, b->text2);
                }
                else if (b->subtype == MB_CHANGE_X_BY)
                    spr.x += (int)eval_value(state, spr, b->arg0_id, b->a, b->text);
                else if (b->subtype == MB_CHANGE_Y_BY)
                    spr.y += (int)eval_value(state, spr, b->arg0_id, b->a, b->text);
                else if (b->subtype == MB_POINT_IN_DIR)
                    spr.direction = (int)eval_value(state, spr, b->arg0_id, b->a, b->text);
                else if (b->subtype == MB_GO_TO_TARGET)
                {
                    if (b->opt == TARGET_RANDOM_POSITION)
                    {
                        spr.x = (std::rand() % 400) - 200;
                        spr.y = (std::rand() % 300) - 150;
                    }
                    else if (b->opt == TARGET_MOUSE_POINTER)
                    {
                        int mx, my;
                        SDL_GetMouseState(&mx, &my);
                        int cx, cy, tw, th;
                        get_sprite_screen_rect(spr, cx, cy, tw, th);
                        spr.x += (mx - cx);
                        spr.y += (cy - my);
                    }
                }
                frame.cur_node = b->next_id;
            }
            else if (b->kind == BK_LOOKS)
            {
                if (b->subtype == LB_SAY)
                {
                    spr.say_text = eval_string(state, spr, b->arg0_id, b->text);
                    spr.is_thinking = false;
                    spr.say_end_time = 0;
                    frame.cur_node = b->next_id;
                }
                else if (b->subtype == LB_THINK)
                {
                    spr.say_text = eval_string(state, spr, b->arg0_id, b->text);
                    spr.is_thinking = true;
                    spr.say_end_time = 0;
                    frame.cur_node = b->next_id;
                }
                else if (b->subtype == LB_SAY_FOR)
                {
                    spr.say_text = eval_string(state, spr, b->arg0_id, b->text);
                    spr.is_thinking = false;
                    float sec = eval_value(state, spr, b->arg1_id, b->a, b->text2);
                    spr.say_end_time = SDL_GetTicks() + (unsigned int)(sec * 1000);
                    g_threads[i].wait_until = spr.say_end_time;
                    frame.cur_node = b->next_id;
                    yielded = true;
                }
                else if (b->subtype == LB_THINK_FOR)
                {
                    spr.say_text = eval_string(state, spr, b->arg0_id, b->text);
                    spr.is_thinking = true;
                    float sec = eval_value(state, spr, b->arg1_id, b->a, b->text2);
                    spr.say_end_time = SDL_GetTicks() + (unsigned int)(sec * 1000);
                    g_threads[i].wait_until = spr.say_end_time;
                    frame.cur_node = b->next_id;
                    yielded = true;
                }
                else if (b->subtype == LB_CHANGE_SIZE_BY)
                {
                    spr.size += (int)eval_value(state, spr, b->arg0_id, b->a, b->text);
                    frame.cur_node = b->next_id;
                }
                else if (b->subtype == LB_SET_SIZE_TO)
                {
                    spr.size = (int)eval_value(state, spr, b->arg0_id, b->a, b->text);
                    frame.cur_node = b->next_id;
                }
                else if (b->subtype == LB_SHOW)
                {
                    spr.visible = true;
                    frame.cur_node = b->next_id;
                }
                else if (b->subtype == LB_HIDE)
                {
                    spr.visible = false;
                    frame.cur_node = b->next_id;
                }
            }
            else if (b->kind == BK_SOUND)
            {
                if (b->subtype == SB_CHANGE_VOLUME_BY)
                {
                    spr.volume += (int)eval_value(state, spr, b->arg0_id, b->a, b->text);
                    audio_set_volume(spr.volume);
                    frame.cur_node = b->next_id;
                }
                else if (b->subtype == SB_SET_VOLUME_TO)
                {
                    spr.volume = (int)eval_value(state, spr, b->arg0_id, b->a, b->text);
                    audio_set_volume(spr.volume);
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
                    float sec = eval_value(state, spr, b->arg0_id, b->a, b->text);
                    g_threads[i].wait_until = SDL_GetTicks() + (unsigned int)(sec * 1000);
                    frame.cur_node = b->next_id;
                    yielded = true;
                }
                else if (b->subtype == CB_REPEAT)
                {
                    int count = (int)eval_value(state, spr, b->arg0_id, b->a, b->text);
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
                    bool cond = eval_bool(state, spr, b->condition_id);
                    frame.cur_node = b->next_id;
                    if (cond && b->child_id != -1)
                        g_threads[i].stack.push_back({b->child_id, -1, 0});
                }
                else if (b->subtype == CB_IF_ELSE)
                {
                    bool cond = eval_bool(state, spr, b->condition_id);
                    frame.cur_node = b->next_id;
                    if (cond && b->child_id != -1)
                        g_threads[i].stack.push_back({b->child_id, -1, 0});
                    else if (!cond && b->child2_id != -1)
                        g_threads[i].stack.push_back({b->child2_id, -1, 0});
                }
                else if (b->subtype == CB_WAIT_UNTIL)
                {
                    bool cond = eval_bool(state, spr, b->condition_id);
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
                    state.ask_msg = eval_string(state, spr, b->arg0_id, b->text);
                    state.ask_reply = "";
                    g_threads[i].waiting_for_ask = true;
                    frame.cur_node = b->next_id;
                    yielded = true;
                }
                else if (b->subtype == SENSB_SET_DRAG_MODE)
                {
                    spr.draggable = (b->opt == 0);
                    frame.cur_node = b->next_id;
                }
                else
                    frame.cur_node = b->next_id;
            }
            else if (b->kind == BK_VARIABLES)
            {
                if (b->opt >= 0 && b->opt < (int)state.variables.size())
                {
                    std::string vname = state.variables[b->opt];
                    if (b->subtype == VB_SET)
                        state.variable_values[vname] = eval_string(state, spr, b->arg0_id, b->text);
                    else if (b->subtype == VB_CHANGE)
                    {
                        float val = std::atof(state.variable_values[vname].c_str());
                        val += eval_value(state, spr, b->arg0_id, b->a, b->text);
                        if (val == std::floor(val))
                            state.variable_values[vname] = std::to_string((int)val);
                        else
                        {
                            char buf[32];
                            std::snprintf(buf, sizeof(buf), "%g", val);
                            state.variable_values[vname] = buf;
                        }
                    }
                    else if (b->subtype == VB_SHOW)
                        state.variable_visible[vname] = true;
                    else if (b->subtype == VB_HIDE)
                        state.variable_visible[vname] = false;
                }
                frame.cur_node = b->next_id;
            }
            else if (b->kind == BK_EVENTS)
            {
                if (b->subtype == EB_BROADCAST)
                    interpreter_trigger_message(state, b->opt);
                frame.cur_node = b->next_id;
            }
            else
                frame.cur_node = b->next_id;
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
    for (auto &spr : state.sprites)
    {
        for (int root_id : spr.top_level_blocks)
        {
            BlockInstance *b = interpreter_find_block(spr, root_id);
            if (b && b->kind == BK_EVENTS && b->subtype == EB_WHEN_FLAG_CLICKED)
                g_threads.push_back({{{{b->next_id, -1, 0}}}, 0, false, false, spr.name});
        }
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
    for (auto &spr : state.sprites)
    {
        for (int root_id : spr.top_level_blocks)
        {
            BlockInstance *b = interpreter_find_block(spr, root_id);
            if (b && b->kind == BK_EVENTS && b->subtype == EB_WHEN_KEY_PRESSED && b->opt == opt)
                g_threads.push_back({{{{b->next_id, -1, 0}}}, 0, false, false, spr.name});
        }
    }
}

void interpreter_trigger_sprite_click(AppState &state)
{
    commit_active_typing(state);
    if (state.selected_sprite >= 0 && state.selected_sprite < (int)state.sprites.size())
    {
        auto &spr = state.sprites[state.selected_sprite];
        for (int root_id : spr.top_level_blocks)
        {
            BlockInstance *b = interpreter_find_block(spr, root_id);
            if (b && b->kind == BK_EVENTS && b->subtype == EB_WHEN_SPRITE_CLICKED)
                g_threads.push_back({{{{b->next_id, -1, 0}}}, 0, false, false, spr.name});
        }
    }
}

void interpreter_trigger_message(AppState &state, int msg_opt)
{
    commit_active_typing(state);
    for (auto &spr : state.sprites)
    {
        for (int root_id : spr.top_level_blocks)
        {
            BlockInstance *b = interpreter_find_block(spr, root_id);
            if (b && b->kind == BK_EVENTS && b->subtype == EB_WHEN_I_RECEIVE && b->opt == msg_opt)
                g_threads.push_back({{{{b->next_id, -1, 0}}}, 0, false, false, spr.name});
        }
    }
}

void interpreter_stop_all(AppState &state)
{
    commit_active_typing(state);
    state.running = false;
    g_threads.clear();
    for (auto &spr : state.sprites)
    {
        spr.say_text = "";
        spr.say_end_time = 0;
    }
    state.ask_active = false;
    audio_stop_all();
}