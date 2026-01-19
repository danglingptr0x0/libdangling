#ifndef LDG_LOG_LOG_H
#define LDG_LOG_LOG_H

#include <yder.h>

#define LDG_LOG(level, fmt, ...) y_log_message(Y_LOG_LEVEL_ ## level, fmt, ## __VA_ARGS__)

#define LDG_LOG_DEBUG(fmt, ...) y_log_message(Y_LOG_LEVEL_DEBUG, fmt, ## __VA_ARGS__)
#define LDG_LOG_INFO(fmt, ...) y_log_message(Y_LOG_LEVEL_INFO, fmt, ## __VA_ARGS__)
#define LDG_LOG_WARNING(fmt, ...) y_log_message(Y_LOG_LEVEL_WARNING, fmt, ## __VA_ARGS__)
#define LDG_LOG_ERROR(fmt, ...) y_log_message(Y_LOG_LEVEL_ERROR, fmt, ## __VA_ARGS__)

#endif
