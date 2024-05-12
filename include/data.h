#pragma once

#include <stddef.h>

struct data_generic {
	char * id;
	char * name_generic;
	char * name_systematic;
	enum {
		NONE = 0,
		PNG,
		SVG
	} structure_type;
	char * structure;
	size_t structure_size;
	char * molecular_formula;
	char * substitution;
	char * molecular_weight;
	char * density;
	size_t melting_point_count;
	char ** melting_points;
	size_t boiling_point_count;
	char ** boiling_points;
	size_t solubility_count;
	char ** solubilities;
	char * psychoactive_properties;
	size_t warning_count;
	char ** warnings;
};
