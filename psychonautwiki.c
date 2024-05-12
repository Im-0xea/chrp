#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include <cjson/cJSON.h>

#include "args.h"
#include "curl_boilerplate.h"
#include "data.h"
#include "flush.h"
#include "sixel_boilerplate.h"
#include "term.h"

#define DATABASE_NAME "PsychonautWiki"

#define PSYCHONAUTWIKI_API_URL "https://api.psychonautwiki.org"
#define PSYCHONAUTWIKI_API_URL_LEN sizeof(PSYCHONAUTWIKI_API_URL)

#define PSYCHONAUTWIKI_POST "{\"query\":\"{substances(query:\\\"%s\\\"){name images{image}class{chemical psychoactive}}}\"}"
#define PSYCHONAUTWIKI_POST_LEN sizeof(PSYCHONAUTWIKI_POST)

int structure_psychonautwiki(struct data_generic * data, const char * mol_link)
{
	size_t mol_link_len = strlen(mol_link);
	if (mol_link_len < 4 || mol_link[mol_link_len - 4] != '.' || mol_link[mol_link_len - 3] != 's' || mol_link[mol_link_len - 2] != 'v' || mol_link[mol_link_len - 1] != 'g')
		return 1;
	CURL *curl_handle = curl_easy_init();
	struct CURLResponse response = GetRequest(curl_handle, mol_link);
	if (mol_link_len >= 4) {
		if (!strcmp(mol_link + mol_link_len - 4, ".png")) {
			data->structure_type = PNG;
		} else if (!strcmp(mol_link + mol_link_len - 4, ".svg")) {
			data->structure_type = SVG;
		} else {
			data->structure_type = NONE;
		}
	} else {
		data->structure_type = SVG;
	}
	data->structure = response.html;
	data->structure_size = response.size;
	
	curl_easy_cleanup(curl_handle);
	return 0;
}
int query_psychonautwiki(struct data_generic * data, char * search_term, const size_t search_len)
{
	CURL *curl_handle = curl_easy_init();

	size_t post_len = PSYCHONAUTWIKI_API_URL_LEN + (PSYCHONAUTWIKI_POST_LEN - 2) + search_len;
	char * post = malloc(post_len);
	snprintf(post, post_len, PSYCHONAUTWIKI_POST, search_term);

	if (debug)
		printf("curling: %s\npost: %s\n", PSYCHONAUTWIKI_API_URL, post);
	struct CURLResponse response = GetPost(curl_handle, PSYCHONAUTWIKI_API_URL, post);
	free(post);
	//if (debug)
	//	printf("json: %s\n", response.html);

	cJSON * fetch_json = cJSON_Parse(response.html);
	if (!fetch_json) {
		free(response.html);
		curl_easy_cleanup(curl_handle);
		printf("received invalid json\n");
		return 1;
	}

	cJSON * json_data = cJSON_GetObjectItemCaseSensitive(fetch_json, "data");
	if (!json_data)
		goto fail_json;
	cJSON * substances = cJSON_GetObjectItemCaseSensitive(json_data, "substances");
	if (!substances || !cJSON_GetArraySize(substances))
		goto fail_json;

	cJSON * substance = cJSON_GetArrayItem(substances, 0);
	if (!substance) {
		goto fail_json;
	}
	cJSON * name = cJSON_GetObjectItemCaseSensitive(substance, "name");
	if (!cJSON_IsString(name) || !name->valuestring)
		goto fail_json;

	size_t name_len = strlen(name->valuestring) + 1;
	char * name_ptr = name->valuestring;
	if (name_len >= 5 && !strncmp(name->valuestring, "Talk:", 5)) {
		name_len -= 5;
		name_ptr += 5;
	}
	data->name_generic = malloc(name_len);
	data->id = malloc(name_len);
	strcpy(data->name_generic, name_ptr);
	strcpy(data->id, name_ptr);
	
	cJSON * images = cJSON_GetObjectItemCaseSensitive(substance, "images");
	//cJSON * mol = cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(images, 0), "image");
	for (int m_i = 0; m_i < cJSON_GetArraySize(images); ++m_i) {
		cJSON * mol = cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(images, m_i), "image");
		if (strcmp(mol->valuestring, "https://psychonautwiki.org/w/images/6/65/Lock-green.svg") &&
		    strcmp(mol->valuestring, "https://psychonautwiki.org/w/images/c/c7/Skull_and_crossbones_darktextred2.png") &&
		    strcmp(mol->valuestring, "https://psychonautwiki.org/w/images/9/9a/Dialog-warning.svg.png") &&
		    strcmp(mol->valuestring, "https://psychonautwiki.org/w/images/e/e0/Yellow-warning-sign1.svg")) {
			structure_psychonautwiki(data, mol->valuestring);
			break;
		}
	}

	cJSON * class = cJSON_GetObjectItemCaseSensitive(substance, "class");
	if (class) {
		cJSON * chemical_classes = cJSON_GetObjectItemCaseSensitive(class, "chemical");
		cJSON * chemical_class = NULL;
		size_t chemical_classes_len = 0;
		for (int cc_i = 0; cc_i < cJSON_GetArraySize(chemical_classes); ++cc_i) {
			chemical_class = cJSON_GetArrayItem(chemical_classes, cc_i);
			chemical_classes_len += strlen(chemical_class->valuestring) + 2;
		}
		if (chemical_classes_len) {
			data->substitution = malloc(chemical_classes_len + 1);
			*data->substitution = '\0';
			int c = 0;
			chemical_class = NULL;
			for (int cc_i = 0; cc_i < cJSON_GetArraySize(chemical_classes); ++cc_i) {
				chemical_class = cJSON_GetArrayItem(chemical_classes, cc_i);
				if (c)
					strcat(data->substitution, ", ");
				strcat(data->substitution, chemical_class->valuestring);
				++c;
			}
		}
		cJSON * psychoactive_classes = cJSON_GetObjectItemCaseSensitive(class, "psychoactive");
		cJSON * psychoactive_class = NULL;
		size_t psychoactive_classes_len = 0;
		for (int pc_i = 0; pc_i < cJSON_GetArraySize(psychoactive_classes); ++pc_i) {
			psychoactive_class = cJSON_GetArrayItem(psychoactive_classes, pc_i);
			psychoactive_classes_len += strlen(psychoactive_class->valuestring) + 2;
		}
		if (psychoactive_classes_len) {
			data->psychoactive_properties = malloc(psychoactive_classes_len + 1);
			*data->psychoactive_properties = '\0';
			int c = 0;
			psychoactive_class = NULL;
			for (int pc_i = 0; pc_i < cJSON_GetArraySize(psychoactive_classes); ++pc_i) {
				psychoactive_class = cJSON_GetArrayItem(psychoactive_classes, pc_i);
				if (c)
					strcat(data->psychoactive_properties, ", ");
				strcat(data->psychoactive_properties, psychoactive_class->valuestring);
				++c;
			}
		}
	}

	cJSON_Delete(fetch_json);

	free(response.html);

	return 0;

	fail_json:
	cJSON_Delete(fetch_json);
	free(response.html);
	return 1;
}
int search_psychonautwiki(char * search_term, const size_t search_len, cJSON ** json_ptr)
{
	struct data_generic data;
	memset(&data, 0, sizeof(data));

	query_psychonautwiki(&data, search_term, search_len);

	FLUSH(mode, DATABASE_NAME, &data, false, json_ptr);

	return 0;
}
