#ifndef CONFIG_H
#define CONFIG_H

#include "backend.h"

int load_config(const char *config_file, struct BackendPool *pool);

#endif

