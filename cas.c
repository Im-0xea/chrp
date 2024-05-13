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
#include "data.h"
#include "flush.h"
#include "sixel_boilerplate.h"
#include "term.h"
#include "xml_boilerplate.h"

#define DATABASE_NAME "CAS"

#define CAS_URL "https://commonchemistry.cas.org"
#define CAS_URL_LEN sizeof(CAS_URL)

#define CAS_API_URL "https://rboq1qukh0.execute-api.us-east-2.amazonaws.com"
#define CAS_API_URL_LEN sizeof(CAS_API_URL)

int fetch_cas(struct data_generic * data)
{
	CURL *curl_handle = curl_easy_init();

	size_t search_len = CAS_API_URL_LEN + sizeof("/default/detail?cas_rn=") + strlen(data->id);
	char * search = malloc(search_len);
	strcpy(search, CAS_API_URL "/default/detail?cas_rn=");
	strcat(search, data->id);
	struct CURLResponse response = GetRequest(curl_handle, search);
	free(search);
	if (!response.size)
		return 1;
	//if (debug)
	//	printf("json: %s\n", response.html);

	cJSON * fetch_json = cJSON_Parse(response.html);
	if (!fetch_json) {
		free(response.html);
		curl_easy_cleanup(curl_handle);
		printf("received invalid json\n");
		return 1;
	}

	cJSON * name_json = cJSON_GetObjectItemCaseSensitive(fetch_json, "name");
	if (cJSON_IsString(name_json) && name_json->valuestring != NULL) {
		//data->name_generic = malloc(strlen(name_json->valuestring) + 1);
		//strcpy(data->name_generic, name_json->valuestring);
		html_format(name_json->valuestring, &data->name_generic);
	}
	cJSON * molecular_formula = cJSON_GetObjectItemCaseSensitive(fetch_json, "molecularFormula");
	if (cJSON_IsString(molecular_formula) && molecular_formula->valuestring != NULL) {
		//data->molecular_formula = malloc(strlen(molecular_formula->valuestring) + 1);
		//strcpy(data->molecular_formula, molecular_formula->valuestring);
		html_format(molecular_formula->valuestring, &data->molecular_formula);
	}
	cJSON * molecular_weight = cJSON_GetObjectItemCaseSensitive(fetch_json, "molecularMass");
	if (cJSON_IsString(molecular_weight) && molecular_weight->valuestring != NULL) {
		//data->molecular_weight = malloc(strlen(molecular_weight->valuestring) + 1);
		//strcpy(data->molecular_weight, molecular_weight->valuestring);
		html_format(molecular_weight->valuestring, &data->molecular_weight);
	}
	cJSON * image = cJSON_GetObjectItemCaseSensitive(fetch_json, "image");
	if (cJSON_IsString(image) && image->valuestring != NULL) {
		size_t len = strlen(image->valuestring);
		if (len) {
			data->structure = malloc(len + 1);
			strcpy(data->structure, image->valuestring);
			data->structure_size = len;
			data->structure_type = SVG;
		}
	}
	cJSON * exp_properties = cJSON_GetObjectItemCaseSensitive(fetch_json, "experimentalProperties");
	cJSON * exp_property = NULL;
	for (int ep_i = 0; ep_i < cJSON_GetArraySize(exp_properties); ++ep_i) {
		exp_property = cJSON_GetArrayItem(exp_properties, ep_i);
		if (!exp_property)
			break;
		cJSON * property_name = cJSON_GetObjectItemCaseSensitive(exp_property, "name");
		if (cJSON_IsString(property_name) && property_name->valuestring != NULL) {
			if (strcmp("Melting Point", property_name->valuestring) == 0) {
				cJSON * melt = cJSON_GetObjectItemCaseSensitive(exp_property, "property");
				if (cJSON_IsString(melt) && melt->valuestring != NULL) {
					data->melting_points = malloc(1 * sizeof(char *));
					data->melting_points[0] = malloc(strlen(melt->valuestring) + 1);
					strcpy(data->melting_points[0], melt->valuestring);
					data->melting_point_count = 1;
				}
			} else if (strcmp("Boiling Point", property_name->valuestring) == 0) {
				cJSON * boil = cJSON_GetObjectItemCaseSensitive(exp_property, "property");
				if (cJSON_IsString(boil) && boil->valuestring != NULL) {
					data->boiling_points = malloc(1 * sizeof(char *));
					data->boiling_points[0] = malloc(strlen(boil->valuestring) + 1);
					strcpy(data->boiling_points[0], boil->valuestring);
					data->boiling_point_count = 1;
				}
			} else if (strcmp("Density", property_name->valuestring) == 0) {
				cJSON * density = cJSON_GetObjectItemCaseSensitive(exp_property, "property");
				if (cJSON_IsString(density) && density->valuestring != NULL) {
					data->density = malloc(strlen(density->valuestring) + 1);
					strcpy(data->density, density->valuestring);
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
int search_cas(char * cas_num, cJSON ** json_ptr)
{
	struct data_generic data;
	memset(&data, 0, sizeof(data));

	if (!cas_num)
		query_cas(&data);
	else
		data.id = cas_num;
	
	if (data.id)
		if (fetch_cas(&data))
			data.id = NULL;

	FLUSH(mode, DATABASE_NAME, &data, true, json_ptr);

	return 0;
}
