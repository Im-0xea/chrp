#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "args.h"
#include "curl_boilerplate.h"
#include "sixel_boilerplate.h"
#include "xml_boilerplate.h"

#define WIKIDATA_URL "https://www.wikidata.org"
#define WIKIDATA_URL_LEN sizeof(WIKIDATA_URL)

static struct wiki_data {
	char * name_generic;
	char * name_IUPAC;
	char * cid;
	char * structure_2d;
	size_t structure_2d_size;
	char * molecular_formula;
	char * molecular_weight;
	size_t melting_point_count;
	char ** melting_points;
	size_t boiling_point_count;
	char ** boiling_points;
	size_t solubility_count;
	char ** solubilities;
} data;
