#include "logger.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>

static const std::string LOG_DIR = "logs";
static const std::string LOG_FILE = "logs/logs.txt";

void InitLogger() {
    if (!std::filesystem::exists(LOG_DIR)) {
        std::filesystem::create_directory(LOG_DIR);
        std::cout << "\033[94m[SYSTEM] Logs directory created.\033[0m" << std::endl;
    }

    std::ofstream file(LOG_FILE, std::ios::out | std::ios::trunc);
    if (file.is_open()) {
        file << "=== System Logger Initialized ===" << std::endl;
        file.close();
    } else {
        std::cerr << "\033[31m[SYSTEM ERROR] Could not create log file!\033[0m" << std::endl;
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

// ---> ADDED: TERMINAL COLORS <---
static std::string GetTerminalColor(LogLevel level) {
    switch (level) {
        case LOG_INFO: return "\033[94m";       // Blue
        case LOG_WARNING: return "\033[33m";    // Yellow
        case LOG_ERROR: return "\033[31m";      // Red
        default: return "\033[38;5;214m";       // Orange
    }
}
static const std::string COLOR_RESET = "\033[0m";

void LogEvent(LogLevel level, int cycle, int line, const std::string& cmd, 
              const std::string& operation, const std::string& old_val, const std::string& new_val) {
    
    // Clean message for the text file
    std::string clean_msg = "[" + LevelToString(level) + "] " +
                          "[Cycle: " + std::to_string(cycle) + "] " +
                          "[Line: " + std::to_string(line) + "] " +
                          "[CMD: " + cmd + "] -> " + 
                          operation + " from " + old_val + " to " + new_val;

    // Colored message for the terminal
    std::string color_msg = GetTerminalColor(level) + clean_msg + COLOR_RESET;

    if (level == LOG_ERROR) std::cerr << color_msg << std::endl;
    else std::cout << color_msg << std::endl;

    std::ofstream file(LOG_FILE, std::ios::out | std::ios::app);
    if (file.is_open()) {
        file << clean_msg << std::endl;
        file.close();
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
    if (file.is_open()) {
        file << clean_msg << std::endl;
        file.close();
    }
}