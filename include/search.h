#pragma once

int search(const int optind, const int c, char ** v);
int search_pubchem(char * encoded_search_term, const size_t encoded_search_len, char * cas_num);
int search_cas(char * cas_num);
int search_chemspider(char * encoded_search_term, const size_t encoded_search_len);
int search_psychonautwiki(char * search_term, const size_t search_len);
