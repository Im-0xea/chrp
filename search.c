#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "search.h"

int search(const int optind, const int c, char ** v)
{
	size_t search_len = 0;
	char * search_term;
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
	curl_global_init(CURL_GLOBAL_ALL);
	CURL *curl_handle = curl_easy_init();

	printf("Searching for: %s...\n\n", search_term);

	char * encoded_search_term = curl_easy_escape(curl_handle, search_term, 0);
	free(search_term);
	const size_t encoded_search_len = strlen(encoded_search_term);
	char * cas_num = malloc(32);
	*cas_num = '\0';

	if (search_pubchem(encoded_search_term, encoded_search_len, cas_num))
		printf("Not Found\n");

	printf("\n");

	if (search_chemspider(encoded_search_term, encoded_search_len))
		printf("Not Found\n");

	printf("\n");

	if (*cas_num != '\0')
		search_cas(cas_num);

	free(cas_num);
	curl_free(encoded_search_term);
	curl_easy_cleanup(curl_handle);
	curl_global_cleanup();

	return 0;
}
