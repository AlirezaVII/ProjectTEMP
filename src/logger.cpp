#include "logger.h"
#include "config.h" // ---> Added to access WINDOW_WIDTH
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <SDL.h>
#include <SDL_ttf.h>
#include <algorithm> // For std::min

static const std::string LOG_DIR = "logs";
static const std::string LOG_FILE = "logs/logs.txt";

// ساختار پیام‌های Toast
struct ToastMessage {
    std::string text;
    LogLevel level;
    Uint32 expire_time;
};
static std::vector<ToastMessage> g_toasts;

void InitLogger() {
    if (!std::filesystem::exists(LOG_DIR)) {
        std::filesystem::create_directory(LOG_DIR);
    }
    std::ofstream file(LOG_FILE, std::ios::out | std::ios::trunc);
    if (file.is_open()) {
        file << "=== System Logger Initialized ===" << std::endl;
        file.close();
    }
}

static std::string LevelToString(LogLevel level) {
    switch (level) {
        case LOG_INFO: return "INFO";
        case LOG_WARNING: return "WARNING";
        case LOG_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

static std::string GetTerminalColor(LogLevel level) {
    switch (level) {
        case LOG_INFO: return "\033[94m";       // آبی
        case LOG_WARNING: return "\033[33m";    // زرد
        case LOG_ERROR: return "\033[31m";      // قرمز
        default: return "\033[38;5;208m";       // نارنجی
    }
}
static const std::string COLOR_RESET = "\033[0m";

// تابع نمایش Toast
void ShowToast(LogLevel level, const std::string& message) {
    // پیام‌ها برای ۳.۵ ثانیه روی صفحه می‌مانند
    g_toasts.push_back({message, level, SDL_GetTicks() + 3500}); 
}

// تابع رندر کردن Toast ها روی صفحه اصلی
void RenderToasts(SDL_Renderer* r, TTF_Font* font) {
    if (!font || g_toasts.empty()) return;
    Uint32 now = SDL_GetTicks();
    
    // ---> FIXED: Start lower so it doesn't overlap the Navbar
    int y_offset = NAVBAR_HEIGHT + 20; 

    // ---> FIXED: Use the logical window width instead of the physical monitor size
    int win_w = WINDOW_WIDTH;

    for (auto it = g_toasts.begin(); it != g_toasts.end(); ) {
        if (now > it->expire_time) {
            it = g_toasts.erase(it); // حذف پیام‌های منقضی شده
        } else {
            int tw = 0, th = 0;
            TTF_SizeUTF8(font, it->text.c_str(), &tw, &th);
            
            // ---> FIXED: Prevent the toast from being wider than the screen itself
            tw = std::min(tw, win_w - 80); 

            int pad_x = 20, pad_y = 12;
            int rect_w = tw + pad_x * 2;
            int rect_h = th + pad_y * 2;
            int rect_x = (win_w - rect_w) / 2; // به طور دقیق در مرکز قرار می‌گیرد

            SDL_Rect bg_rect = {rect_x, y_offset, rect_w, rect_h};
            
            // تعیین رنگ پس‌زمینه اخطار
            if (it->level == LOG_ERROR) SDL_SetRenderDrawColor(r, 220, 53, 69, 240);       // قرمز ارور
            else if (it->level == LOG_WARNING) SDL_SetRenderDrawColor(r, 255, 193, 7, 240); // زرد هشدار
            else SDL_SetRenderDrawColor(r, 40, 167, 69, 240);                               // سبز موفقیت
            
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
            SDL_RenderFillRect(r, &bg_rect);
            
            // رسم متن
            SDL_Color tc = {255, 255, 255, 255};
            if (it->level == LOG_WARNING) tc = {0, 0, 0, 255}; // متن مشکی برای هشدار زرد

            SDL_Surface* surf = TTF_RenderUTF8_Blended(font, it->text.c_str(), tc);
            if (surf) {
                SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
                
                // Crop text rendering if it's too long
                SDL_Rect src_rect = {0, 0, tw, th}; 
                SDL_Rect txt_rect = {rect_x + pad_x, y_offset + pad_y, tw, th};
                
                SDL_RenderCopy(r, tex, &src_rect, &txt_rect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(surf);
            }

            y_offset += rect_h + 10; // فاصله بین چند ارور همزمان
            ++it;
        }
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void LogEvent(LogLevel level, int cycle, int line, const std::string& cmd, 
              const std::string& operation, const std::string& old_val, const std::string& new_val) {
    
    std::string clean_msg = "[" + LevelToString(level) + "] " +
                          "[Cycle: " + std::to_string(cycle) + "] " +
                          "[Line: " + std::to_string(line) + "] " +
                          "[CMD: " + cmd + "] -> " + 
                          operation + " from " + old_val + " to " + new_val;

    std::string color_msg = GetTerminalColor(level) + clean_msg + COLOR_RESET;

    if (level == LOG_ERROR) std::cerr << color_msg << std::endl;
    else std::cout << color_msg << std::endl;

    std::ofstream file(LOG_FILE, std::ios::out | std::ios::app);
    if (file.is_open()) { file << clean_msg << std::endl; file.close(); }

    // نمایش اتوماتیک در برنامه
    if (level == LOG_ERROR || level == LOG_WARNING) {
        ShowToast(level, cmd + ": " + operation + " -> " + new_val);
    }
}

void LogSimple(LogLevel level, int cycle, int line, const std::string& cmd, const std::string& message) {
    std::string clean_msg = "[" + LevelToString(level) + "] " +
                          "[Cycle: " + std::to_string(cycle) + "] " +
                          "[Line: " + std::to_string(line) + "] " +
                          "[CMD: " + cmd + "] -> " + message;

    std::string color_msg = GetTerminalColor(level) + clean_msg + COLOR_RESET;

    if (level == LOG_ERROR) std::cerr << color_msg << std::endl;
    else std::cout << color_msg << std::endl;

    std::ofstream file(LOG_FILE, std::ios::out | std::ios::app);
    if (file.is_open()) { file << clean_msg << std::endl; file.close(); }

    // نمایش اتوماتیک در برنامه
    if (level == LOG_ERROR || level == LOG_WARNING) {
        ShowToast(level, message);
    }
}