#pragma once

#include <cjson/cJSON.h>

int search_term(const int optind, const int c, char ** v);
int search_daemon(const int optind, const int c, char **v, int port);
int search_raw(char * search_term, size_t search_len, cJSON * json_results[4]);
int search_pubchem(char * encoded_search_term, const size_t encoded_search_len, char * cas_num, cJSON ** json_ptr);
int search_cas(char * cas_num, cJSON ** json_ptr);
int search_chemspider(char * encoded_search_term, const size_t encoded_search_len, cJSON ** json_ptr);
int search_psychonautwiki(char * search_term, const size_t search_len, cJSON ** json_ptr);
