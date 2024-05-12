#pragma once

#include "api.h"
#include "term.h"

#define FLUSH(flush_mode, database, data, print_id, json_ptr) \
	do { \
		if (flush_mode == WEB) { \
			flush_json(database, data, print_id, json_ptr); \
		} else { \
			flush_term(database, data, print_id); \
		} \
	} while (0)
