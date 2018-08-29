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

#define _FILE strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__
#define LOG_FMT          "==> %-8s | %-8s |%s:%d|  "
#define LOG_ARGS(logTag) logTag, _FILE, __FUNCTION__, __LINE__

#if (LOG_LEVEL >= LOG_ERROR)
#define LOGE(message, args...) printf(LOG_FMT message NEWLINE, LOG_ARGS(ERROR_TAG), ## args)
#else
#define LOGE(message, args...)
#endif
#if (LOG_LEVEL >= LOG_WARNING)
#define LOGW(message, args...) printf(LOG_FMT message NEWLINE, LOG_ARGS(WARNING_TAG), ## args)
#else
#define LOGW(message, args...)
#endif
#if (LOG_LEVEL >= LOG_INFO)
#define LOGI(message, args...) printf(LOG_FMT message NEWLINE, LOG_ARGS(INFO_TAG), ## args)
#else
#define LOGI(message, args...)
#endif
#if (LOG_LEVEL >= LOG_DEBUG)
#define LOGD(message, args...) printf(LOG_FMT message NEWLINE, LOG_ARGS(DEBUG_TAG), ## args)
#else
#define LOGD(message, args...)
#endif

#endif // LOGGING_HPP
