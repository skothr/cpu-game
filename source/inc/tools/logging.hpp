#ifndef LOGGING_HPP
#define LOGGING_HPP

#include <cstdio>
#include <cstring>

#define LOG_NONE     0
#define LOG_ERROR    1
#define LOG_WARNING  2
#define LOG_INFO     3
#define LOG_DEBUG    4

#define LOG_LEVEL LOG_DEBUG

#define ERROR_TAG   "ERROR"
#define WARNING_TAG "WARNING"
#define INFO_TAG    "INFO"
#define DEBUG_TAG   "DEBUG"
#define NEWLINE     "\n"

#define COLOR_RED printf("\033[1;31m")
#define COLOR_RED printf("\033[1;31m")
#define COLOR_GREEN printf("\033[0;32m")
#define COLOR_YELLOW printf("\033[1;33m")
#define COLOR_BLUE printf("\033[1;34m")
#define COLOR_MAGENTA printf("\033[1;35m")
#define COLOR_CYAN printf("\033[1;36m")
#define END_COLOR printf("\033[0m;")

#define _FILE strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__
#define LOG_FMT          ">%-7s|%16s:%-4d|  "
#define LOG_ARGS(logTag) logTag, _FILE, __LINE__

#if (LOG_LEVEL >= LOG_ERROR)
#define LOGE(message, args...) COLOR_RED; printf(LOG_FMT message NEWLINE, LOG_ARGS(ERROR_TAG), ## args); END_COLOR
#else
#define LOGE(message, args...)
#endif
#if (LOG_LEVEL >= LOG_WARNING)
#define LOGW(message, args...) COLOR_YELLOW; printf(LOG_FMT message NEWLINE, LOG_ARGS(WARNING_TAG), ## args); END_COLOR
#else
#define LOGW(message, args...)
#endif
#if (LOG_LEVEL >= LOG_INFO)
#define LOGI(message, args...) printf(LOG_FMT message NEWLINE, LOG_ARGS(INFO_TAG), ## args)
#define LOGIC(color, message, args...) color; printf(LOG_FMT message NEWLINE, LOG_ARGS(INFO_TAG), ## args); END_COLOR
#else
#define LOGI(message, args...)
#define LOGIC(color, message, args...)
#endif
#if (LOG_LEVEL >= LOG_DEBUG)
#define LOGD(message, args...) printf(LOG_FMT message NEWLINE, LOG_ARGS(DEBUG_TAG), ## args)
#define LOGDC(color, message, args...) color; printf(LOG_FMT message NEWLINE, LOG_ARGS(DEBUG_TAG), ## args); END_COLOR
#else
#define LOGD(message, args...)
#define LOGDC(color, message, args...)
#endif

#endif // LOGGING_HPP
