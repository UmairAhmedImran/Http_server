#include "../include/backend.h"
#include "../include/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 256
#define MAX_BACKENDS 10

int load_config(const char *config_file, struct BackendPool *pool) {
    FILE *file = fopen(config_file, "r");
    if (!file) {
        log_error("config", "file_open_failed", 
                  "Could not open config file. Using default configuration.");
        return -1;
    }

    pool->count = 0;
    pool->current_index = 0;
    
    char line[MAX_LINE_LENGTH];
    
    while (fgets(line, sizeof(line), file) && pool->count < MAX_BACKENDS) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        if (len > 0 && line[len - 1] == '\r') {
            line[len - 1] = '\0';
        }

        char *trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') {
            trimmed++;
        }

        if (*trimmed == '\0' || *trimmed == '#') {
            continue;
        }

        char *host = trimmed;
        char *colon1 = strchr(host, ':');
        if (!colon1) {
            log_message(LOG_WARNING, "Invalid config line (missing port): %s", trimmed);
            continue;
        }
        *colon1 = '\0';

        char *port_str = colon1 + 1;
        char *colon2 = strchr(port_str, ':');
        if (!colon2) {
            log_message(LOG_WARNING, "Invalid config line (missing weight): %s", trimmed);
            continue;
        }
        *colon2 = '\0';

        char *weight_str = colon2 + 1;

        int port = atoi(port_str);
        int weight = atoi(weight_str);

        if (port <= 0 || port > 65535) {
            log_message(LOG_WARNING, "Invalid port in config line: %s", trimmed);
            continue;
        }

        if (weight <= 0 || weight > 100) {
            log_message(LOG_WARNING, "Invalid weight in config line (must be 1-100): %s", trimmed);
            continue;
        }

        if (strlen(host) >= sizeof(pool->backends[pool->count].host)) {
            log_message(LOG_WARNING, "Host too long in config line: %s", trimmed);
            continue;
        }

        strncpy(pool->backends[pool->count].host, host, sizeof(pool->backends[pool->count].host) - 1);
        pool->backends[pool->count].host[sizeof(pool->backends[pool->count].host) - 1] = '\0';
        pool->backends[pool->count].port = port;
        pool->backends[pool->count].weight = weight;
        pool->backends[pool->count].active_connections = 0;
        pool->backends[pool->count].is_active = true;
        pool->current_weights[pool->count] = weight;

        pool->count++;
    }

    fclose(file);

    if (pool->count == 0) {
        log_error("config", "no_backends", "No valid backends found in config file");
        return -1;
    }

    log_message(LOG_INFO, "Loaded %d backend(s) from config file", pool->count);
    return 0;
}

