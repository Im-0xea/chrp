#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "args.h"
#include "search.h"

int search_raw(char * search_term, size_t search_len, cJSON ** json_results)
{
	if (!search_term)
		return 1;
	curl_global_init(CURL_GLOBAL_ALL);
	CURL *curl_handle = curl_easy_init();

	if (mode == TUI)
		printf("Searching for: %s...\n\n", search_term);

	char * encoded_search_term = curl_easy_escape(curl_handle, search_term, 0);
	const size_t encoded_search_len = strlen(encoded_search_term);
	char * cas_num = malloc(32);
	*cas_num = '\0';

	search_pubchem(encoded_search_term, encoded_search_len, cas_num, &json_results[0]);
	search_chemspider(encoded_search_term, encoded_search_len, &json_results[1]);
	search_cas(cas_num, &json_results[2]);
	search_psychonautwiki(search_term, search_len, &json_results[3]);

	if (mode == WEB) {
	}

	curl_free(encoded_search_term);
	curl_easy_cleanup(curl_handle);
	curl_global_cleanup();

	return 0;
}
