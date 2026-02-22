#include "filemenu.h"
#include "config.h"
#include "renderer.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include "audio.h"
#include <string>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <map>

// Target coordinates from the Retina virtual canvas
static const int RENDER_W = 480 * 4;
static const int RENDER_H = 360 * 4;

static void set_color(SDL_Renderer *r, Color c) { SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255); }

static void draw_text(SDL_Renderer *r, TTF_Font *f, const char *txt, int x, int y, Color c)
{
    SDL_Color sc = {(Uint8)c.r, (Uint8)c.g, (Uint8)c.b, 255};
    SDL_Surface *s = TTF_RenderUTF8_Blended(f, txt, sc);
    if (!s)
        return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
    SDL_Rect dst = {x, y, s->w, s->h};
    SDL_RenderCopy(r, t, NULL, &dst);
    SDL_DestroyTexture(t);
    SDL_FreeSurface(s);
}

// ---> SECURE JSON ESCAPER <---
static std::string escape_json(const std::string &s)
{
    std::string res;
    for (char c : s)
    {
        if (c == '"')
            res += "\\\"";
        else if (c == '\\')
            res += "\\\\";
        else if (c == '\n')
            res += "\\n";
        else
            res += c;
    }
    return res;
}

// ---> EXTRACT AND SAVE PAINT STROKES TO PNG <---
static void save_paint_layer(SDL_Renderer *r, SDL_Texture *paint_layer, const std::string &filepath)
{
    if (!paint_layer)
        return;

    SDL_Texture *prev_target = SDL_GetRenderTarget(r);
    SDL_SetRenderTarget(r, paint_layer);

    int w, h;
    SDL_QueryTexture(paint_layer, NULL, NULL, &w, &h);

    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ABGR8888);
    if (surf)
    {
        SDL_RenderReadPixels(r, NULL, SDL_PIXELFORMAT_ABGR8888, surf->pixels, surf->pitch);
        IMG_SavePNG(surf, filepath.c_str());
        SDL_FreeSurface(surf);
    }

    SDL_SetRenderTarget(r, prev_target);
}

// ---> MEMORY CLEANER <---
static void cleanup_project(AppState &state)
{
    for (auto &s : state.sprites)
    {
        for (auto &c : s.costumes)
        {
            if (c.original_texture)
                SDL_DestroyTexture(c.original_texture);
            if (c.paint_layer)
                SDL_DestroyTexture(c.paint_layer);
            if (c.composed_texture)
                SDL_DestroyTexture(c.composed_texture);
        }
        for (auto &snd : s.sounds)
        {
            if (snd.chunk)
                Mix_FreeChunk(snd.chunk);
        }
    }
    for (auto &b : state.backdrops)
    {
        if (b.original_texture)
            SDL_DestroyTexture(b.original_texture);
        if (b.paint_layer)
            SDL_DestroyTexture(b.paint_layer);
        if (b.composed_texture)
            SDL_DestroyTexture(b.composed_texture);
    }
    state.sprites.clear();
    state.backdrops.clear();
    state.drag.active = false;
}

// ---> NATIVE C++ MICRO JSON PARSER <---
struct JVal
{
    enum Type
    {
        NULL_,
        OBJ,
        ARR,
        STR,
        NUM,
        BOOL
    } type = NULL_;
    std::string s;
    double n = 0;
    bool b = false;
    std::vector<JVal> a;
    std::unordered_map<std::string, JVal> o;

    JVal() {}
    JVal(Type t) : type(t) {}
    JVal(const std::string &str) : type(STR), s(str) {}
    JVal(double num) : type(NUM), n(num) {}
    JVal(bool bl) : type(BOOL), b(bl) {}
};

static JVal parse_json(const std::string &str, size_t &pos)
{
    auto skip_ws = [&]()
    { while(pos < str.size() && isspace(str[pos])) pos++; };
    skip_ws();
    if (pos >= str.size())
        return JVal();

    if (str[pos] == '{')
    {
        JVal val(JVal::OBJ);
        pos++;
        while (pos < str.size())
        {
            skip_ws();
            if (str[pos] == '}')
            {
                pos++;
                break;
            }
            JVal key = parse_json(str, pos);
            skip_ws();
            if (str[pos] == ':')
                pos++;
            JVal v = parse_json(str, pos);
            val.o[key.s] = v;
            skip_ws();
            if (str[pos] == ',')
                pos++;
        }
        return val;
    }
    else if (str[pos] == '[')
    {
        JVal val(JVal::ARR);
        pos++;
        while (pos < str.size())
        {
            skip_ws();
            if (str[pos] == ']')
            {
                pos++;
                break;
            }
            val.a.push_back(parse_json(str, pos));
            skip_ws();
            if (str[pos] == ',')
                pos++;
        }
        return val;
    }
    else if (str[pos] == '"')
    {
        pos++;
        std::string res;
        while (pos < str.size() && str[pos] != '"')
        {
            if (str[pos] == '\\')
            {
                pos++;
                if (pos < str.size())
                {
                    if (str[pos] == 'n')
                        res += '\n';
                    else if (str[pos] == '"')
                        res += '"';
                    else if (str[pos] == '\\')
                        res += '\\';
                    else
                        res += str[pos];
                }
            }
            else
                res += str[pos];
            pos++;
        }
        pos++;
        return JVal(res);
    }
    else if (isalpha(str[pos]))
    {
        std::string res;
        while (pos < str.size() && isalpha(str[pos]))
        {
            res += str[pos++];
        }
        if (res == "true")
            return JVal(true);
        if (res == "false")
            return JVal(false);
        return JVal();
    }
    else
    {
        std::string res;
        while (pos < str.size() && (isdigit(str[pos]) || str[pos] == '.' || str[pos] == '-'))
        {
            res += str[pos++];
        }
        return JVal(std::atof(res.c_str()));
    }
}

void filemenu_layout(FileMenuRects &rects, int file_btn_x)
{
    rects.menu = {file_btn_x, NAVBAR_HEIGHT, 220, 10 + 3 * 30};
    int y = NAVBAR_HEIGHT + 5;
    rects.items[0] = {file_btn_x, y, 220, 30};
    y += 30;
    rects.items[1] = {file_btn_x, y, 220, 30};
    y += 30;
    rects.items[2] = {file_btn_x, y, 220, 30};
    rects.item_count = 3;
}

void filemenu_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state, const FileMenuRects &rects)
{
    if (!state.file_menu_open)
        return;
    set_color(r, COL_WHITE);
    SDL_RenderFillRect(r, const_cast<SDL_Rect *>(&rects.menu));
    set_color(r, COL_SEPARATOR);
    SDL_RenderDrawRect(r, const_cast<SDL_Rect *>(&rects.menu));

    const char *labels[3] = {"New", "Load from your computer", "Save to your computer"};
    for (int i = 0; i < 3; ++i)
    {
        if (state.file_menu_hover == i)
        {
            set_color(r, COL_FILEMENU_HOVER);
            SDL_RenderFillRect(r, const_cast<SDL_Rect *>(&rects.items[i]));
        }
        draw_text(r, font, labels[i], rects.items[i].x + 15, rects.items[i].y + 6, COL_FILEMENU_TEXT);
    }
}

bool filemenu_handle_event(const SDL_Event &e, AppState &state, const FileMenuRects &rects, SDL_Renderer *r)
{
    if (!state.file_menu_open)
        return false;

    if (e.type == SDL_MOUSEMOTION)
    {
        state.file_menu_hover = -1;
        int mx = e.motion.x, my = e.motion.y;
        for (int i = 0; i < 3; ++i)
        {
            if (mx >= rects.items[i].x && mx < rects.items[i].x + rects.items[i].w &&
                my >= rects.items[i].y && my < rects.items[i].y + rects.items[i].h)
            {
                state.file_menu_hover = i;
                return true;
            }
        }
        return true;
    }
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
    {

        if (state.file_menu_hover == 0)
        {
            cleanup_project(state);
            state.project_name = "Untitled";
            state.variables.clear();
            state.variable_values.clear();
            state.variable_visible.clear();

            // Clear messages
            state.messages.clear();
            state.messages.push_back("message1");

            state.sprites.push_back(Sprite("Sprite1", IMG_LoadTexture(r, "assets/sprites/scratch_cat.png"), "assets/sprites/scratch_cat.png"));
            Mix_Chunk *ds = audio_load_sound("assets/sounds/meow.wav");
            state.sprites.back().sounds.push_back(SoundData("meow", ds, "assets/sounds/meow.wav"));

            state.backdrops.push_back(Backdrop("backdrop1", nullptr, ""));
            
            // ---> CLEARS PEN LAYER ON NEW FILE <---
            extern void renderer_init_pen_layer(SDL_Renderer*);
            renderer_init_pen_layer(r);

            state.selected_sprite = 0;
            state.selected_backdrop = 0;
            state.next_block_id = 1;
            state.file_menu_open = false;
            std::cout << "SUCCESS: New Project Created!\n";
            return true;
        }

        if (state.file_menu_hover == 1)
        {
            char buffer[2048];
            std::string result = "";

            FILE *pipe = popen("osascript -e 'POSIX path of (choose file with prompt \"Choose Project JSON\")'", "r");
            if (pipe)
            {
                while (fgets(buffer, sizeof(buffer), pipe) != NULL)
                    result += buffer;
                pclose(pipe);
            }
            if (!result.empty() && result.back() == '\n')
                result.pop_back();

            if (!result.empty())
            {
                std::ifstream in(result);
                if (in.is_open())
                {
                    std::string json_str((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                    size_t pos = 0;
                    JVal root = parse_json(json_str, pos);

                    if (root.type == JVal::OBJ)
                    {
                        cleanup_project(state);
                        state.project_name = root.o["project_name"].s;
                        state.next_block_id = root.o["next_block_id"].n;

                        // RESTORE ACTIVE BACKDROP
                        state.selected_backdrop = (int)root.o["selected_backdrop"].n;

                        if (state.next_block_id <= 0)
                            state.next_block_id = 1;

                        state.variables.clear();
                        state.variable_values.clear();
                        state.variable_visible.clear();
                        for (auto &v_val : root.o["variables"].a)
                        {
                            std::string n = v_val.o["name"].s;
                            state.variables.push_back(n);
                            state.variable_values[n] = v_val.o["value"].s;
                            state.variable_visible[n] = v_val.o["visible"].b;
                        }

                        for (auto &b_val : root.o["backdrops"].a)
                        {
                            std::string n = b_val.o["name"].s;
                            std::string p = b_val.o["source_path"].s;
                            SDL_Texture *t = p.empty() ? nullptr : IMG_LoadTexture(r, p.c_str());
                            Backdrop b(n, t, p);

                            b.flip_h = b_val.o["flip_h"].b;
                            b.flip_v = b_val.o["flip_v"].b;
                            std::string paint_path = b_val.o["paint_path"].s;
                            if (!paint_path.empty() && std::filesystem::exists(paint_path))
                            {
                                SDL_Texture *loaded_paint = IMG_LoadTexture(r, paint_path.c_str());
                                if (loaded_paint)
                                {
                                    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
                                    b.paint_layer = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, RENDER_W, RENDER_H);
                                    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
                                    SDL_SetTextureBlendMode(b.paint_layer, SDL_BLENDMODE_BLEND);

                                    SDL_Texture *prev = SDL_GetRenderTarget(r);
                                    SDL_SetRenderTarget(r, b.paint_layer);
                                    SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
                                    SDL_RenderClear(r);
                                    SDL_RenderCopy(r, loaded_paint, NULL, NULL);
                                    SDL_SetRenderTarget(r, prev);
                                    SDL_DestroyTexture(loaded_paint);
                                }
                            }

                            for (auto &sh_val : b_val.o["shapes"].a)
                            {
                                GraphicShape sh;
                                sh.type = (ShapeType)(int)sh_val.o["type"].n;
                                sh.rect = {(int)sh_val.o["x"].n, (int)sh_val.o["y"].n, (int)sh_val.o["w"].n, (int)sh_val.o["h"].n};
                                sh.color = {(Uint8)sh_val.o["r"].n, (Uint8)sh_val.o["g"].n, (Uint8)sh_val.o["b"].n, (Uint8)sh_val.o["a"].n};
                                if (sh.type == SHAPE_TEXT)
                                    sh.text = sh_val.o["text"].s;
                                b.shapes.push_back(sh);
                            }
                            state.backdrops.push_back(b);
                        }

                        for (auto &s_val : root.o["sprites"].a)
                        {
                            std::string n = s_val.o["name"].s;
                            Sprite spr(n, nullptr, "");
                            spr.costumes.clear();

                            spr.x = s_val.o["x"].n;
                            spr.y = s_val.o["y"].n;
                            spr.direction = s_val.o["direction"].n;
                            spr.size = s_val.o["size"].n;
                            spr.visible = s_val.o["visible"].b;

                            // RESTORE ACTIVE COSTUME
                            spr.selected_costume = (int)s_val.o["selected_costume"].n;

                            for (auto &c_val : s_val.o["costumes"].a)
                            {
                                std::string cn = c_val.o["name"].s;
                                std::string cp = c_val.o["source_path"].s;
                                SDL_Texture *ct = cp.empty() ? nullptr : IMG_LoadTexture(r, cp.c_str());
                                Costume c(cn, ct, cp);

                                c.flip_h = c_val.o["flip_h"].b;
                                c.flip_v = c_val.o["flip_v"].b;
                                std::string paint_path = c_val.o["paint_path"].s;
                                if (!paint_path.empty() && std::filesystem::exists(paint_path))
                                {
                                    SDL_Texture *loaded_paint = IMG_LoadTexture(r, paint_path.c_str());
                                    if (loaded_paint)
                                    {
                                        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
                                        c.paint_layer = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, RENDER_W, RENDER_H);
                                        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
                                        SDL_SetTextureBlendMode(c.paint_layer, SDL_BLENDMODE_BLEND);

                                        SDL_Texture *prev = SDL_GetRenderTarget(r);
                                        SDL_SetRenderTarget(r, c.paint_layer);
                                        SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
                                        SDL_RenderClear(r);
                                        SDL_RenderCopy(r, loaded_paint, NULL, NULL);
                                        SDL_SetRenderTarget(r, prev);
                                        SDL_DestroyTexture(loaded_paint);
                                    }
                                }

                                for (auto &sh_val : c_val.o["shapes"].a)
                                {
                                    GraphicShape sh;
                                    sh.type = (ShapeType)(int)sh_val.o["type"].n;
                                    sh.rect = {(int)sh_val.o["x"].n, (int)sh_val.o["y"].n, (int)sh_val.o["w"].n, (int)sh_val.o["h"].n};
                                    sh.color = {(Uint8)sh_val.o["r"].n, (Uint8)sh_val.o["g"].n, (Uint8)sh_val.o["b"].n, (Uint8)sh_val.o["a"].n};
                                    if (sh.type == SHAPE_TEXT)
                                        sh.text = sh_val.o["text"].s;
                                    c.shapes.push_back(sh);
                                }
                                spr.costumes.push_back(c);
                            }

                            for (auto &snd_val : s_val.o["sounds"].a)
                            {
                                std::string sn = snd_val.o["name"].s;
                                std::string sp = snd_val.o["source_path"].s;
                                Mix_Chunk *c = sp.empty() ? nullptr : audio_load_sound(sp);
                                SoundData sd(sn, c, sp);
                                sd.volume = snd_val.o["volume"].n;
                                spr.sounds.push_back(sd);
                            }

                            for (auto &blk_val : s_val.o["blocks"].a)
                            {
                                BlockInstance blk;
                                blk.id = (int)blk_val.o["id"].n;
                                blk.kind = (BlockKind)(int)blk_val.o["kind"].n;
                                blk.subtype = (int)blk_val.o["subtype"].n;
                                blk.x = (int)blk_val.o["x"].n;
                                blk.y = (int)blk_val.o["y"].n;
                                blk.a = (int)blk_val.o["a"].n;
                                blk.b = (int)blk_val.o["b"].n;
                                blk.c = (int)blk_val.o["c"].n;
                                blk.d = (int)blk_val.o["d"].n;
                                blk.e = (int)blk_val.o["e"].n;
                                blk.f = (int)blk_val.o["f"].n;
                                blk.opt = (int)blk_val.o["opt"].n;
                                blk.text = blk_val.o["text"].s;
                                blk.text2 = blk_val.o["text2"].s;
                                blk.next_id = (int)blk_val.o["next_id"].n;
                                blk.parent_id = (int)blk_val.o["parent_id"].n;
                                blk.child_id = (int)blk_val.o["child_id"].n;
                                blk.child2_id = (int)blk_val.o["child2_id"].n;
                                blk.condition_id = (int)blk_val.o["condition_id"].n;
                                blk.arg0_id = (int)blk_val.o["arg0_id"].n;
                                blk.arg1_id = (int)blk_val.o["arg1_id"].n;
                                blk.arg2_id = (int)blk_val.o["arg2_id"].n;
                                spr.blocks.push_back(blk);
                            }

                            for (auto &tl_val : s_val.o["top_level_blocks"].a)
                            {
                                spr.top_level_blocks.push_back((int)tl_val.n);
                            }

                            if (!spr.costumes.empty() && spr.selected_costume >= 0 && spr.selected_costume < (int)spr.costumes.size())
                                spr.texture = spr.costumes[spr.selected_costume].texture;
                            else if (!spr.costumes.empty())
                                spr.texture = spr.costumes[0].texture;

                            state.sprites.push_back(spr);
                        }

                        if (state.sprites.empty())
                        {
                            state.sprites.push_back(Sprite("Sprite1", IMG_LoadTexture(r, "assets/sprites/scratch_cat.png"), "assets/sprites/scratch_cat.png"));
                        }

                        state.selected_sprite = 0;
                        if (state.selected_backdrop < 0 || state.selected_backdrop >= (int)state.backdrops.size())
                            state.selected_backdrop = 0;

                        std::cout << "SUCCESS: Workspace fully loaded from " << result << "\n";
                    }
                }
            }
            state.file_menu_open = false;
            return true;
        }

        if (state.file_menu_hover == 2)
        {
            if (state.project_name.empty())
            {
                std::cout << "Cannot save: Project name is empty.\n";
                state.file_menu_open = false;
                return true;
            }
            std::string dir = "Projects/" + state.project_name;
            std::filesystem::create_directories(dir + "/assets");
            std::string filepath = dir + "/project.json";
            std::ofstream out(filepath);

            out << "{\n";
            out << "  \"project_name\": \"" << escape_json(state.project_name) << "\",\n";
            out << "  \"next_block_id\": " << state.next_block_id << ",\n";
            out << "  \"selected_backdrop\": " << state.selected_backdrop << ",\n";

            out << "  \"variables\": [";
            for (size_t vi = 0; vi < state.variables.size(); vi++)
            {
                std::string vname = state.variables[vi];
                out << "{\"name\":\"" << escape_json(vname) << "\",\"value\":\"" << escape_json(state.variable_values.at(vname)) << "\",\"visible\":" << (state.variable_visible.at(vname) ? "true" : "false") << "}";
                if (vi < state.variables.size() - 1)
                    out << ",";
            }
            out << "],\n";

            out << "  \"backdrops\": [\n";
            for (size_t i = 0; i < state.backdrops.size(); i++)
            {
                auto &b = state.backdrops[i];

                std::string p_path = "";
                if (b.paint_layer)
                {
                    p_path = dir + "/assets/backdrop_" + std::to_string(i) + "_paint.png";
                    save_paint_layer(r, b.paint_layer, p_path);
                }

                out << "    { \"name\": \"" << escape_json(b.name) << "\", "
                    << "\"source_path\": \"" << escape_json(b.source_path) << "\", "
                    << "\"paint_path\": \"" << escape_json(p_path) << "\", "
                    << "\"flip_h\": " << (b.flip_h ? "true" : "false") << ", "
                    << "\"flip_v\": " << (b.flip_v ? "true" : "false") << ", "
                    << "\"shapes\": [";
                for (size_t sh = 0; sh < b.shapes.size(); sh++)
                {
                    auto &shape = b.shapes[sh];
                    out << "{\"type\":" << shape.type << ",\"x\":" << shape.rect.x << ",\"y\":" << shape.rect.y << ",\"w\":" << shape.rect.w << ",\"h\":" << shape.rect.h << ",\"r\":" << (int)shape.color.r << ",\"g\":" << (int)shape.color.g << ",\"b\":" << (int)shape.color.b << ",\"a\":" << (int)shape.color.a << ",\"text\":\"" << escape_json(shape.text) << "\"}";
                    if (sh < b.shapes.size() - 1)
                        out << ",";
                }
                out << "] }";
                if (i < state.backdrops.size() - 1)
                    out << ",";
                out << "\n";
            }
            out << "  ],\n";

            out << "  \"sprites\": [\n";
            for (size_t i = 0; i < state.sprites.size(); i++)
            {
                auto &s = state.sprites[i];
                out << "    {\n";
                out << "      \"name\": \"" << escape_json(s.name) << "\",\n";
                out << "      \"x\": " << s.x << ",\n";
                out << "      \"y\": " << s.y << ",\n";
                out << "      \"direction\": " << s.direction << ",\n";
                out << "      \"size\": " << s.size << ",\n";
                out << "      \"visible\": " << (s.visible ? "true" : "false") << ",\n";
                out << "      \"selected_costume\": " << s.selected_costume << ",\n";

                out << "      \"costumes\": [\n";
                for (size_t c = 0; c < s.costumes.size(); c++)
                {
                    auto &cost = s.costumes[c];

                    std::string p_path = "";
                    if (cost.paint_layer)
                    {
                        p_path = dir + "/assets/sprite_" + std::to_string(i) + "_costume_" + std::to_string(c) + "_paint.png";
                        save_paint_layer(r, cost.paint_layer, p_path);
                    }

                    out << "        { \"name\": \"" << escape_json(cost.name) << "\", "
                        << "\"source_path\": \"" << escape_json(cost.source_path) << "\", "
                        << "\"paint_path\": \"" << escape_json(p_path) << "\", "
                        << "\"flip_h\": " << (cost.flip_h ? "true" : "false") << ", "
                        << "\"flip_v\": " << (cost.flip_v ? "true" : "false") << ", "
                        << "\"shapes\": [";
                    for (size_t sh = 0; sh < cost.shapes.size(); sh++)
                    {
                        auto &shape = cost.shapes[sh];
                        out << "{\"type\":" << shape.type << ",\"x\":" << shape.rect.x << ",\"y\":" << shape.rect.y << ",\"w\":" << shape.rect.w << ",\"h\":" << shape.rect.h << ",\"r\":" << (int)shape.color.r << ",\"g\":" << (int)shape.color.g << ",\"b\":" << (int)shape.color.b << ",\"a\":" << (int)shape.color.a << ",\"text\":\"" << escape_json(shape.text) << "\"}";
                        if (sh < cost.shapes.size() - 1)
                            out << ",";
                    }
                    out << "] }";
                    if (c < s.costumes.size() - 1)
                        out << ",";
                    out << "\n";
                }
                out << "      ],\n";

                out << "      \"sounds\": [\n";
                for (size_t sd = 0; sd < s.sounds.size(); sd++)
                {
                    auto &snd = s.sounds[sd];
                    out << "        { \"name\": \"" << escape_json(snd.name) << "\", \"source_path\": \"" << escape_json(snd.source_path) << "\", \"volume\": " << snd.volume << " }";
                    if (sd < s.sounds.size() - 1)
                        out << ",";
                    out << "\n";
                }
                out << "      ],\n";

                out << "      \"blocks\": [\n";
                for (size_t bi = 0; bi < s.blocks.size(); bi++)
                {
                    auto &blk = s.blocks[bi];
                    out << "        {\"id\":" << blk.id << ",\"kind\":" << (int)blk.kind << ",\"subtype\":" << blk.subtype
                        << ",\"x\":" << blk.x << ",\"y\":" << blk.y << ",\"a\":" << blk.a << ",\"b\":" << blk.b
                        << ",\"c\":" << blk.c << ",\"d\":" << blk.d << ",\"e\":" << blk.e << ",\"f\":" << blk.f
                        << ",\"opt\":" << blk.opt << ",\"text\":\"" << escape_json(blk.text)
                        << "\",\"text2\":\"" << escape_json(blk.text2) << "\",\"next_id\":" << blk.next_id
                        << ",\"parent_id\":" << blk.parent_id << ",\"child_id\":" << blk.child_id
                        << ",\"child2_id\":" << blk.child2_id << ",\"condition_id\":" << blk.condition_id
                        << ",\"arg0_id\":" << blk.arg0_id << ",\"arg1_id\":" << blk.arg1_id
                        << ",\"arg2_id\":" << blk.arg2_id << "}";
                    if (bi < s.blocks.size() - 1)
                        out << ",";
                    out << "\n";
                }
                out << "      ],\n";

                out << "      \"top_level_blocks\": [";
                for (size_t ti = 0; ti < s.top_level_blocks.size(); ti++)
                {
                    out << s.top_level_blocks[ti];
                    if (ti < s.top_level_blocks.size() - 1)
                        out << ",";
                }
                out << "]\n";

                out << "    }";
                if (i < state.sprites.size() - 1)
                    out << ",";
                out << "\n";
            }
            out << "  ]\n";
            out << "}\n";
            out.close();
            std::cout << "SUCCESS: Workspace exported perfectly to " << filepath << "\n";

            state.file_menu_open = false;
            return true;
        }

        state.file_menu_open = false;
        return true;
    }
    return false;
}