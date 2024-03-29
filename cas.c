#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

#include <curl/curl.h>

#include <cjson/cJSON.h>

#include "args.h"
#include "curl_boilerplate.h"
#include "sixel_boilerplate.h"
#include "xml_boilerplate.h"

#define CAS_URL "https://commonchemistry.cas.org"
#define CAS_URL_LEN sizeof(CAS_URL)

#define CAS_API_URL "https://rboq1qukh0.execute-api.us-east-2.amazonaws.com"
#define CAS_API_URL_LEN sizeof(CAS_API_URL)

struct cas_data {
	char * name_generic;
	char * cas_num;
	char * structure_2d;
	size_t structure_2d_size;
	char * molecular_formula;
	char * molecular_weight;
	char * density;
	size_t melting_point_count;
	char ** melting_points;
	size_t boiling_point_count;
	char ** boiling_points;
};
static struct cas_data data;

int fetch_cas()
{
	CURL *curl_handle = curl_easy_init();

	size_t search_len = CAS_API_URL_LEN + sizeof("/default/detail?cas_rn=") + strlen(data.cas_num);
	char * search = malloc(search_len);
	strcpy(search, CAS_API_URL "/default/detail?cas_rn=");
	strcat(search, data.cas_num);
	if (debug)
		printf("curling: %s\n", search);
	struct CURLResponse response = GetRequest(curl_handle, search);
	free(search);
	//if (debug)
	//	printf("json: %s\n", response.html);

	cJSON * fetch_json = cJSON_Parse(response.html);
	if (!fetch_json) {
		printf("received invalid json\n");
		return 1;
	}

	cJSON * name_json = cJSON_GetObjectItemCaseSensitive(fetch_json, "name");
	if (cJSON_IsString(name_json) && name_json->valuestring != NULL) {
		data.name_generic = malloc(strlen(name_json->valuestring) + 1);
		strcpy(data.name_generic, name_json->valuestring);
	}
	cJSON * molecular_formula = cJSON_GetObjectItemCaseSensitive(fetch_json, "molecularFormula");
	if (cJSON_IsString(molecular_formula) && molecular_formula->valuestring != NULL) {
		data.molecular_formula = malloc(strlen(molecular_formula->valuestring) + 1);
		strcpy(data.molecular_formula, molecular_formula->valuestring);
	}
	cJSON * molecular_weight = cJSON_GetObjectItemCaseSensitive(fetch_json, "molecularMass");
	if (cJSON_IsString(molecular_weight) && molecular_weight->valuestring != NULL) {
		data.molecular_weight = malloc(strlen(molecular_weight->valuestring) + 1);
		strcpy(data.molecular_weight, molecular_weight->valuestring);
	}
	cJSON * image = cJSON_GetObjectItemCaseSensitive(fetch_json, "image");
	if (cJSON_IsString(image) && image->valuestring != NULL) {
		size_t len = strlen(image->valuestring);
		data.structure_2d = malloc(len + 1);
		strcpy(data.structure_2d, image->valuestring);
		data.structure_2d_size = len;
	}
	cJSON * exp_properties = cJSON_GetObjectItemCaseSensitive(fetch_json, "experimentalProperties");
	cJSON * exp_property = NULL;
	cJSON_ArrayForEach(exp_properties, exp_property) {
		cJSON * property_name = cJSON_GetObjectItemCaseSensitive(exp_property, "name");
		if (cJSON_IsString(property_name) && property_name->valuestring != NULL) {
			if (strcmp("Melting Point", exp_property->valuestring) == 0) {
				cJSON * melt = cJSON_GetObjectItemCaseSensitive(exp_property, "property");
				if (cJSON_IsString(melt) && melt->valuestring != NULL) {
					data.melting_points = malloc(1 * sizeof(char *));
					data.melting_points[0] = malloc(strlen(melt->valuestring) + 1);
					strcpy(data.melting_points[0], melt->valuestring);
					data.melting_point_count = 1;
				}
			} else if (strcmp("Boiling Point", exp_property->valuestring) == 0) {
				cJSON * boil = cJSON_GetObjectItemCaseSensitive(exp_property, "property");
				if (cJSON_IsString(boil) && boil->valuestring != NULL) {
					data.boiling_points = malloc(1 * sizeof(char *));
					data.boiling_points[0] = malloc(strlen(boil->valuestring));
					strcpy(data.boiling_points[0], boil->valuestring);
					data.boiling_point_count = 1;
				}
			} else if (strcmp("Density", exp_property->valuestring) == 0) {
				cJSON * density = cJSON_GetObjectItemCaseSensitive(exp_property, "property");
				if (cJSON_IsString(density) && density->valuestring != NULL) {
					data.density = malloc(strlen(density->valuestring));
					strcpy(data.density, density->valuestring);
				}
			}

		}
	}
	cJSON_Delete(fetch_json);

	free(response.html);

	curl_easy_cleanup(curl_handle);
	return 0;
}
int query_cas()
{
	return 0;
}
int flush_cas()
{
	printf("CAS:\n");
	if (data.structure_2d) {
		print_svg(data.structure_2d, data.structure_2d_size, 300, 300);
		free(data.structure_2d);
	}
	if (data.cas_num)
		printf("CAS number: %s\n", data.cas_num);
	if (data.name_generic) {
		printf("CAS name: %s\n", data.name_generic);
		free(data.name_generic);
	}
	if (data.molecular_formula) {
		char * tmp = html_format(strip_property(data.molecular_formula));
		printf("Molecular Formula: %s\n", tmp);
		free(tmp);
		free(data.molecular_formula);
	}
	if (data.molecular_weight) {
		printf("Molecular Weight: %s\n", strip_property(data.molecular_weight));
		free(data.molecular_weight);
	}
	if (data.density) {
		printf("Density: %s\n", strip_property(data.density));
		free(data.density);
	}

	if (data.melting_point_count) {
		printf("Melting Point(s):\n");
		for (int x = 0; x < data.melting_point_count; ++x) {
			printf("\t%s\n", strip_property(data.melting_points[x]));
			free(data.melting_points[x]);
		}
		free(data.melting_points);
	}

	if (data.boiling_point_count) {
		printf("Boiling Point(s):\n");
		for (int x = 0; x < data.boiling_point_count; ++x) {
			printf("\t%s\n", strip_property(data.boiling_points[x]));
			free(data.boiling_points[x]);
		}
		free(data.boiling_points);
	}
	return 0;
}
int search_cas(char * cas_num)
{
	if (!cas_num)
		query_cas();
	else
		data.cas_num = cas_num;
	
	if (!data.cas_num)
		return 1;

	fetch_cas();

	flush_cas();
	return 0;
}
