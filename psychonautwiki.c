#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include <cjson/cJSON.h>

#include "args.h"
#include "curl_boilerplate.h"
#include "sixel_boilerplate.h"

#define PSYCHONAUTWIKI_API_URL "https://api.psychonautwiki.org"
#define PSYCHONAUTWIKI_API_URL_LEN sizeof(PSYCHONAUTWIKI_API_URL)

#define PSYCHONAUTWIKI_POST "{\"query\":\"{substances(query:\\\"%s\\\"){name images{image}class{chemical psychoactive}effects{name}}}\"}"
#define PSYCHONAUTWIKI_POST_LEN sizeof(PSYCHONAUTWIKI_POST)

struct psychonautwiki_data {
	char * name_generic;
	char * structure_2d;
	size_t structure_2d_size;
	char * chemical_class;
	char * psychoactive_class;
	char ** effects;
	size_t effects_size;
	char ** dangerous;
	size_t dangerous_size;
	char * toxicity;
} data;
int structure_psychonautwiki(const char * mol_link)
{
	size_t mol_link_len = strlen(mol_link);
	if (mol_link_len < 4 || mol_link[mol_link_len - 4] != '.' || mol_link[mol_link_len - 3] != 's' || mol_link[mol_link_len - 2] != 'v' || mol_link[mol_link_len - 1] != 'g')
		return 1;
	CURL *curl_handle = curl_easy_init();
	struct CURLResponse response = GetRequest(curl_handle, mol_link);
	data.structure_2d = response.html;
	data.structure_2d_size = response.size;
	
	curl_easy_cleanup(curl_handle);
	return 0;
}
int query_psychonautwiki(char * search_term, const size_t search_len)
{
	CURL *curl_handle = curl_easy_init();

	size_t post_len = (PSYCHONAUTWIKI_POST_LEN - 2) + search_len;
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

	data.name_generic = malloc(strlen(name->valuestring) + 1);
	strcpy(data.name_generic, name->valuestring);
	
	cJSON * images = cJSON_GetObjectItemCaseSensitive(substance, "images");
	cJSON * mol = cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(images, 0), "image");
	if (mol && cJSON_IsString(mol) && mol->valuestring)
		structure_psychonautwiki(mol->valuestring);

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
			data.chemical_class = malloc(chemical_classes_len + 1);
			*data.chemical_class = '\0';
			int c = 0;
			chemical_class = NULL;
			for (int cc_i = 0; cc_i < cJSON_GetArraySize(chemical_classes); ++cc_i) {
				chemical_class = cJSON_GetArrayItem(chemical_classes, cc_i);
				if (c)
					strcat(data.chemical_class, ", ");
				strcat(data.chemical_class, chemical_class->valuestring);
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
			data.psychoactive_class = malloc(psychoactive_classes_len + 1);
			*data.psychoactive_class = '\0';
			int c = 0;
			psychoactive_class = NULL;
			for (int pc_i = 0; pc_i < cJSON_GetArraySize(psychoactive_classes); ++pc_i) {
				psychoactive_class = cJSON_GetArrayItem(psychoactive_classes, pc_i);
				if (c)
					strcat(data.psychoactive_class, ", ");
				strcat(data.psychoactive_class, psychoactive_class->valuestring);
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
int flush_psychonautwiki()
{
	printf("PsychonautWiki:\n");

	if (!data.name_generic) {
		printf("not found\n\n");
		return 1;
	}

	if (data.structure_2d) {
		print_svg(data.structure_2d, data.structure_2d_size, 300, 300);
		free(data.structure_2d);
	}
	if (data.name_generic) {
		printf("Name: %s\n", data.name_generic);
		free(data.name_generic);
	}
	if (data.chemical_class) {
		printf("Chemical Class: %s\n", data.chemical_class);
		free(data.chemical_class);
	}
	if (data.psychoactive_class) {
		printf("Psychoactive Class: %s\n", data.psychoactive_class);
		free(data.psychoactive_class);
	}

	printf("\n");
	return 0;
}
int search_psychonautwiki(char * search_term, const size_t search_len)
{
	query_psychonautwiki(search_term, search_len);

	flush_psychonautwiki();

	return 0;
}
