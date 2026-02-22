#ifndef LOGGER_H
#define LOGGER_H

#include <string>

enum LogLevel {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
};

// تابع آماده‌سازی سیستم لاگ (ساخت پوشه و فایل)
void InitLogger();

// تابع مرکزی ثبت وقایع
void LogEvent(LogLevel level, int cycle, int line, const std::string& cmd, 
              const std::string& operation, const std::string& old_val, const std::string& new_val);

// تابع کمکی برای لاگ‌های ساده‌تر (بدون old/new value)
void LogSimple(LogLevel level, int cycle, int line, const std::string& cmd, const std::string& message);

#endif // LOGGER_H