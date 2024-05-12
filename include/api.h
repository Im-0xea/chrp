#pragma once

#include <stdbool.h>

#include <cjson/cJSON.h>

#include "data.h"

int flush_json(const char * database, struct data_generic * data, bool print_id, cJSON ** json_ptr);
