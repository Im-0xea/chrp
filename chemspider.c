#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <curl/curl.h>

#include "args.h"
#include "curl_boilerplate.h"

#define CHEMSPIDER_URL "https://www.chemspider.com"
#define CHEMSPIDER_URL_LEN sizeof(CHEMSPIDER_URL)

int search_chemspider(char * encoded_search_term, const size_t encoded_search_len)
{
	printf("ChemSpider:\n");
	CURL *curl_handle = curl_easy_init();

	size_t chemspider_search_len = CHEMSPIDER_URL_LEN + sizeof("/Search.aspx?q=") + encoded_search_len;
	char * chemspider_search = malloc(chemspider_search_len);
	strcpy(chemspider_search, CHEMSPIDER_URL "/Search.aspx?q=");
	strcat(chemspider_search, encoded_search_term);
	if (debug)
		printf("curling: %s\n", chemspider_search);
	struct CURLResponse query_response = GetRequest(curl_handle, chemspider_search);
	free(chemspider_search);
	if (debug)
		printf("xml: %s\n", query_response.html);

	htmlDocPtr doc = htmlReadMemory(query_response.html, query_response.size, NULL, NULL, HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
	free(query_response.html);
	// fail check

	xmlNodePtr root_node = xmlDocGetRootElement(doc);
	if (root_node == NULL) {
		xmlFreeDoc(doc);
		goto chemspider_fail;
	}

	xmlChar *redirect_url = NULL;
	xmlNodePtr body_node, h2_node, a_node;
	for (body_node = root_node->children; body_node; body_node = body_node->next) {
		if (body_node->type == XML_ELEMENT_NODE && strcmp((char *)body_node->name, "body") == 0) {
			for (h2_node = body_node->children; h2_node; h2_node = h2_node->next) {
				if (h2_node->type == XML_ELEMENT_NODE && strcmp((char *)h2_node->name, "h2") == 0) {
					xmlNodePtr text_node = h2_node->children;
					if (text_node != NULL && text_node->type == XML_TEXT_NODE &&
						strstr((char *)text_node->content, "Object moved to") != NULL) {
						for (a_node = h2_node->children; a_node; a_node = a_node->next) {
							if (a_node->type == XML_ELEMENT_NODE && strcmp((char *)a_node->name, "a") == 0) {
								redirect_url = xmlGetProp(a_node, (const xmlChar *)"href");
								break;
							}
						}
						break;
					}
				}
			}
			break;
		}
	}
	xmlFreeDoc(doc);

	if (redirect_url == NULL)
		goto chemspider_fail;

	size_t fetch_len = CHEMSPIDER_URL_LEN + strlen((char *) redirect_url);
	char * fetch_url = malloc(fetch_len);
	strcpy(fetch_url, CHEMSPIDER_URL);
	strcat(fetch_url, (char *) redirect_url);
	if (debug)
		printf("curling: %s\n", fetch_url);
	struct CURLResponse fetch_response = GetRequest(curl_handle, fetch_url);
	free(fetch_url);
	if (debug)
		printf("xml: %s\n", fetch_response.html);
	free(fetch_response.html);
	xmlFree(redirect_url);

	curl_easy_cleanup(curl_handle);
	return 0;

	chemspider_fail:
	curl_easy_cleanup(curl_handle);
	return 1;
}
