#include "../include/logging.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MAX_LOG_LENGTH 1024
#define TIMESTAMP_FORMAT "%Y-%m-%d %H:%M:%S"

static FILE* log_file = NULL;
static int log_to_console = 1;

void init_logging() {
    log_file = fopen("server.log", "a");
    if (!log_file) {
        printf("Warning: Could not open log file, logging to console only\n");
        log_to_console = 1;
    } else {
        log_to_console = 1;
    }
}

void cleanup_logging() {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}

const char* level_to_string(log_level_t level) {
    switch (level) {
        case LOG_INFO: return "INFO";
        case LOG_WARNING: return "WARNING";
        case LOG_ERROR: return "ERROR";
        case LOG_DEBUG: return "DEBUG";
        default: return "UNKNOWN";
    }
}

void log_message(log_level_t level, const char* format, ...) {
    char buffer[MAX_LOG_LENGTH];
    char timestamp[64];
    time_t now;
    struct tm* timeinfo;
    
    time(&now);
    timeinfo = localtime(&now);
    strftime(timestamp, sizeof(timestamp), TIMESTAMP_FORMAT, timeinfo);
    
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    char escaped_buffer[MAX_LOG_LENGTH * 2];
    int j = 0;
    for (int i = 0; buffer[i] != '\0' && j < sizeof(escaped_buffer) - 1; i++) {
        if (buffer[i] == '"' || buffer[i] == '\\') {
            escaped_buffer[j++] = '\\';
        }
        escaped_buffer[j++] = buffer[i];
    }
    escaped_buffer[j] = '\0';
    
    // Calculate maximum possible size: timestamp(64) + level(10) + escaped message(2048) + JSON overhead(~50)
    char json_log[2200];
    snprintf(json_log, sizeof(json_log),
             "{\"timestamp\": \"%s\", \"level\": \"%s\", \"message\": \"%s\"}",
             timestamp, level_to_string(level), escaped_buffer);
    
    if (log_to_console) {
        printf("%s\n", json_log);
        fflush(stdout);
    }
    
    if (log_file) {
        fprintf(log_file, "%s\n", json_log);
        fflush(log_file);
    }
}

void log_request(const char* method, const char* path, const char* backend, int status_code) {
    char timestamp[64];
    time_t now;
    struct tm* timeinfo;
    
    time(&now);
    timeinfo = localtime(&now);
    strftime(timestamp, sizeof(timestamp), TIMESTAMP_FORMAT, timeinfo);
    
    char escaped_path[512];
    int j = 0;
    for (int i = 0; path[i] != '\0' && j < sizeof(escaped_path) - 1; i++) {
        if (path[i] == '"' || path[i] == '\\') {
            escaped_path[j++] = '\\';
        }
        escaped_path[j++] = path[i];
    }
    escaped_path[j] = '\0';
    
    char json_log[1200];
    snprintf(json_log, sizeof(json_log),
             "{\"timestamp\": \"%s\", \"level\": \"INFO\", \"type\": \"request\", "
             "\"method\": \"%s\", \"path\": \"%s\", \"backend\": \"%s\", \"status\": %d}",
             timestamp, method, escaped_path, backend ? backend : "none", status_code);
    
    if (log_to_console) {
        printf("%s\n", json_log);
        fflush(stdout);
    }
    
    if (log_file) {
        fprintf(log_file, "%s\n", json_log);
        fflush(log_file);
    }
}

void log_error(const char* component, const char* error_type, const char* details) {
    char timestamp[64];
    time_t now;
    struct tm* timeinfo;
    
    time(&now);
    timeinfo = localtime(&now);
    strftime(timestamp, sizeof(timestamp), TIMESTAMP_FORMAT, timeinfo);
    
    char escaped_details[512];
    int j = 0;
    for (int i = 0; details[i] != '\0' && j < sizeof(escaped_details) - 1; i++) {
        if (details[i] == '"' || details[i] == '\\') {
            escaped_details[j++] = '\\';
        }
        escaped_details[j++] = details[i];
    }
    escaped_details[j] = '\0';
    
    char json_log[1200];
    snprintf(json_log, sizeof(json_log),
             "{\"timestamp\": \"%s\", \"level\": \"ERROR\", \"component\": \"%s\", "
             "\"error_type\": \"%s\", \"details\": \"%s\"}",
             timestamp, component, error_type, escaped_details);
    
    if (log_to_console) {
        printf("%s\n", json_log);
        fflush(stdout);
    }
    
    if (log_file) {
        fprintf(log_file, "%s\n", json_log);
        fflush(log_file);
    }
}