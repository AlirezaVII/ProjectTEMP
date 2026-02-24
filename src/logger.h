#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <SDL.h>
#include <SDL_ttf.h>

enum LogLevel {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
};

// توابع لاگر
void InitLogger();
void LogEvent(LogLevel level, int cycle, int line, const std::string& cmd, 
              const std::string& operation, const std::string& old_val, const std::string& new_val);
void LogSimple(LogLevel level, int cycle, int line, const std::string& cmd, const std::string& message);

// توابع سیستم نمایش اخطار در محیط برنامه (Toastify System)
void ShowToast(LogLevel level, const std::string& message);
void RenderToasts(SDL_Renderer* r, TTF_Font* font);

#endif // LOGGER_H