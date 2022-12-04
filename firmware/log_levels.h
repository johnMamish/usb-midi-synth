#ifndef _LOG_LEVELS_H
#define _LOG_LEVELS_H

#define LOG_LEVEL_TRACE   6
#define LOG_LEVEL_DEBUG   5
#define LOG_LEVEL_INFO    4
#define LOG_LEVEL_WARN    3
#define LOG_LEVEL_ERROR   2
#define LOG_LEVEL_FATAL   1

// can redefine
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_WARN
#endif

#if (LOG_LEVEL >= LOG_LEVEL_TRACE)
#define LOG_TRACE(iface, format, ...) cprintf(iface, "trace: " format, ##__VA_ARGS__)
#else
#define LOG_TRACE(iface, format, ...)
#endif

#if (LOG_LEVEL >= LOG_LEVEL_DEBUG)
#define LOG_DEBUG(iface, format, ...) cprintf(iface, "debug: " format, ##__VA_ARGS__)
#else
#define LOG_DEBUG(iface, format, ...)
#endif

#if (LOG_LEVEL >= LOG_LEVEL_INFO)
#define LOG_INFO(iface, format, ...) cprintf(iface, "info:  " format, ##__VA_ARGS__)
#else
#define LOG_INFO(iface, format, ...)
#endif

#if (LOG_LEVEL >= LOG_LEVEL_WARN)
#define LOG_WARN(iface, format, ...) cprintf(iface, "warn:  " format, ##__VA_ARGS__)
#else
#define LOG_WARN(iface, format, ...)
#endif

#if (LOG_LEVEL >= LOG_LEVEL_ERROR)
#define LOG_ERROR(iface, format, ...) cprintf(iface, "error: " format, ##__VA_ARGS__)
#else
#define LOG_ERROR(iface, format, ...)
#endif

#if (LOG_LEVEL >= LOG_LEVEL_FATAL)
#define LOG_FATAL(iface, format, ...) cprintf(iface, "fatal: " format, ##__VA_ARGS__)
#else
#define LOG_FATAL(iface, format, ...)
#endif


#endif
