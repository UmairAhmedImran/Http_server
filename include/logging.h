#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>
#include <time.h>
#include <string.h>

typedef enum {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_DEBUG
} log_level_t;

void init_logging();
void log_message(log_level_t level, const char* format, ...);
void log_request(const char* method, const char* path, const char* backend, int status_code);
void log_error(const char* component, const char* error_type, const char* details);
void cleanup_logging();

#endif