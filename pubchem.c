#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <curl/curl.h>

#include "args.h"
#include "curl_boilerplate.h"

#define PUBCHEM_URL "https://pubchem.ncbi.nlm.nih.gov/rest/pug"
#define PUBCHEM_URL_LEN sizeof(PUBCHEM_URL)

const char * pubchem_properties_list[] = {
	"MolecularFormula",
	"MolecularWeight",
	"IUPACName"
	//"Odor",
	//"Taste",
	//"MeltingPoint",
	//"BoilingPoint",
	//"FlashPoint",
	//"Solubility",
	//"Density",
	//"Decomposition",
	//"DissociationConstants"
};
const size_t pubchem_properties_count = 3;

int search_pubchem(char * encoded_search_term, const size_t encoded_search_len)
{
	printf("PubChem:\n");
	CURL *curl_handle = curl_easy_init();

	size_t pubchem_search_len = PUBCHEM_URL_LEN + encoded_search_len;
	for (size_t ppn = 0; ppn < pubchem_properties_count; ++ppn) {
		pubchem_search_len += strlen(pubchem_properties_list[ppn]);
	}
	pubchem_search_len += pubchem_properties_count - 1;
	char * pubchem_properties = malloc(pubchem_search_len);
	*pubchem_properties = '\0';
	for (int ppn = 0; ppn < pubchem_properties_count; ++ppn) {
		strcat(pubchem_properties, pubchem_properties_list[ppn]);
		if (ppn < pubchem_properties_count - 1)
			strcat(pubchem_properties, ",");
	}

	char * pubchem_search = malloc(pubchem_search_len + 128);

	strcpy(pubchem_search, PUBCHEM_URL);
	strcat(pubchem_search, "/compound");
	strcat(pubchem_search, "/name/");
	strcat(pubchem_search, encoded_search_term);
	strcat(pubchem_search, "/property/");
	strcat(pubchem_search, pubchem_properties);
	strcat(pubchem_search, "/XML");

	if (debug)
		printf("curling: %s\n", pubchem_search);
	struct CURLResponse response = GetRequest(curl_handle, pubchem_search);
	free(pubchem_search);

	if (debug)
		printf("xml: %s\n", response.html);

	xmlDocPtr doc;

	doc = xmlReadMemory(response.html, strlen(response.html), "noname.xml", NULL, 0);
	if (doc == NULL) {
		fprintf(stderr, "Failed to parse document\n");
		goto pubchem_fail;
	}

	xmlNodePtr root_node = xmlDocGetRootElement(doc);
	if (root_node == NULL || strcmp((char *)root_node->name, "Fault") == 0) {
		xmlFreeDoc(doc);
		goto pubchem_fail;
	}

	for (xmlNodePtr prop_node = root_node->children; prop_node; prop_node = prop_node->next) {
		if (prop_node->type != XML_ELEMENT_NODE || strcmp((char *)prop_node->name, "Properties") != 0)
			continue;

		for (xmlNodePtr current_node = prop_node->children; current_node; current_node = current_node->next) {
			for (int ppn = 0; ppn < pubchem_properties_count; ++ppn) {
				if (current_node->type != XML_ELEMENT_NODE || strcmp((char *)current_node->name, pubchem_properties_list[ppn]) != 0)
					continue;
	
				xmlChar * node_value = xmlNodeGetContent(current_node);
				if (node_value != NULL) {
					printf("%s: %s\n", pubchem_properties_list[ppn], node_value);
					xmlFree(node_value);
				}
				break;
			}
		}
		break;
	}

	xmlFreeDoc(doc);
	free(response.html);
	curl_easy_cleanup(curl_handle);
	return 0;

	pubchem_fail:
	free(response.html);
	curl_easy_cleanup(curl_handle);
	return 1;
}

