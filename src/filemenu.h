#ifndef FILEMENU_H
#define FILEMENU_H

#include "SDL.h"
#include "SDL_ttf.h"
#include "types.h"

struct FileMenuRects {
    SDL_Rect menu;
    SDL_Rect items[5];
    int item_count;
};

void filemenu_layout(FileMenuRects &rects, int file_btn_x);
void filemenu_draw(SDL_Renderer *r, TTF_Font *font, const AppState &state,
                   const FileMenuRects &rects);
bool filemenu_handle_event(const SDL_Event &e, AppState &state,
                           const FileMenuRects &rects);

#endif
