#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"
#include "search.h"
#include "sixel_boilerplate.h"
#include "term.h"
#include "xml_boilerplate.h"

int flush_term(const char * database, struct data_generic * data, bool print_id)
{
	printf("%s:\n", database);
	if (!data->id) {
		printf("not found\n\n");
		return 1;
	}

	if (data->structure) {
		if (data->structure_type == PNG) {
			if (print_png((unsigned char *) data->structure, data->structure_size, 300, 300)) {
				printf("failed to render structure\n");
			}
			free(data->structure);
		} else if (data->structure_type == SVG) {
			if (print_svg(data->structure, data->structure_size, 300, 300)) {
				printf("failed to render structure\n");
			}
			free(data->structure);
		}
	}

	if (print_id) {
		printf("Id: %s\n", data->id);
		free(data->id);
	}

	if (data->name_generic) {
		printf("Name: %s\n", strip_property(data->name_generic));
		free(data->name_generic);
	}

	if (data->name_systematic) {
		printf("Systematic IUPAC name: %s\n", strip_property(data->name_systematic));
		free(data->name_systematic);
	}

	if (data->substitution) {
		printf("Chemical Class: %s\n", data->substitution);
		free(data->substitution);
	}

	if (data->molecular_formula) {
		printf("Molecular Formula: %s\n", strip_property(data->molecular_formula));
		free(data->molecular_formula);
	}

	if (data->molecular_weight) {
		printf("Molecular Weight: %s\n", strip_property(data->molecular_weight));
		free(data->molecular_weight);
	}

	if (data->melting_point_count) {
		printf("Melting Point(s):\n");
		for (int x = 0; x < data->melting_point_count; ++x) {
			printf("\t%s\n", strip_property(data->melting_points[x]));
			free(data->melting_points[x]);
		}
	}
	if (data->melting_points)
		free(data->melting_points);

	if (data->boiling_point_count) {
		printf("Boiling Point(s):\n");
		for (int x = 0; x < data->boiling_point_count; ++x) {
			printf("\t%s\n", strip_property(data->boiling_points[x]));
			free(data->boiling_points[x]);
		}
	}
	if (data->boiling_points)
		free(data->boiling_points);

	if (data->solubility_count) {
		printf("Solubility:\n");
		for (int x = 0; x < data->solubility_count; ++x) {
			printf("\t%s\n", strip_property(data->solubilities[x]));
			free(data->solubilities[x]);
		}
	}
	if (data->solubilities)
		free(data->solubilities);

	if (data->warning_count) {
		printf("Warning:\n");
		for (int x = 0; x < data->warning_count; ++x) {
			printf("\t%s\n", strip_property(data->warnings[x]));
			free(data->warnings[x]);
		}
	}

	if (data->warnings)
		free(data->warnings);

	if (data->psychoactive_properties) {
		printf("Psychoactive Class: %s\n", data->psychoactive_properties);
		free(data->psychoactive_properties);
	}

	printf("\n");
	return 0;
}

int search_term(const int optind, const int c, char ** v)
{
	size_t search_len = 0;
	char * search_term = NULL;
	if (optind < c) {
		for (int st = optind; st < c; ++st)
			search_len += strlen(v[st]) + 1;

		search_term = malloc(search_len);
		*search_term = '\0';

		for (int st = optind; st < c; ++st) {
			strcat(search_term, v[st]);
			if (st < c - 1)
				strcat(search_term, " ");
		}
	} else {
		search_term = malloc(1024);
		if (fgets(search_term, 1024, stdin) == NULL)
			return 1;

		search_len = strlen(search_term) - 1;

		if (search_term[search_len] != '\n') {
			fprintf(stderr, "search input limit reached(1024)\n");
			return 1;
		}
		search_term[search_len] = '\0';
	}

	int ret = search_raw(search_term, search_len, NULL);
	free(search_term);
	return ret;
}
