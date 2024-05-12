#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <curl/curl.h>

#include <cjson/cJSON.h>

#include "args.h"
#include "curl_boilerplate.h"
#include "sixel_boilerplate.h"
#include "xml_boilerplate.h"
#include "data.h"
#include "flush.h"

#define DATABASE_NAME "PubChem"

#define PUBCHEM_URL "https://pubchem.ncbi.nlm.nih.gov"
#define PUBCHEM_URL_LEN sizeof(PUBCHEM_URL)

int query_pubchem(struct data_generic * data, char * encoded_search_term, const size_t encoded_search_len)
{
	CURL * curl_handle = curl_easy_init();

	size_t pubchem_search_len = PUBCHEM_URL_LEN + encoded_search_len + sizeof("/rest/pug" "/compound" "/name/" "/property/Title,MolecularFormula,MolecularWeight,IUPACName/XML");

	char * pubchem_search = malloc(pubchem_search_len);

	strcpy(pubchem_search, PUBCHEM_URL "/rest/pug");
	strcat(pubchem_search, "/compound");
	strcat(pubchem_search, "/name/");
	strcat(pubchem_search, encoded_search_term);
	strcat(pubchem_search, "/property/Title,MolecularFormula,MolecularWeight,IUPACName/XML");

	struct CURLResponse response = GetRequest(curl_handle, pubchem_search);
	free(pubchem_search);

	//if (debug)
	//	printf("xml: %s\n", response.html);

	xmlDocPtr doc = xmlReadMemory(response.html, response.size, NULL, NULL, XML_PARSE_NOWARNING | XML_PARSE_NOERROR);
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
			if (current_node->type != XML_ELEMENT_NODE)
				continue;

			if (strcmp((char *) current_node->name, "Title") == 0)
				data->name_generic = (char *) xmlNodeGetContent(current_node);
			if (strcmp((char *) current_node->name, "MolecularFormula") == 0)
				data->molecular_formula = (char *) xmlNodeGetContent(current_node);
			if (strcmp((char *) current_node->name, "MolecularWeight") == 0)
				data->molecular_weight = (char *) xmlNodeGetContent(current_node);
			if (strcmp((char *) current_node->name, "IUPACName") == 0)
				data->name_systematic = (char *) xmlNodeGetContent(current_node);
			if (strcmp((char *) current_node->name, "CID") == 0)
				data->id = (char *) xmlNodeGetContent(current_node);
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
int structure_pubchem(struct data_generic * data)
{
	CURL *curl_handle = curl_easy_init();

	size_t fetch_len = PUBCHEM_URL_LEN + strlen((char *) data->id) + sizeof("/rest/pug/compound/cid/" "/PNG?image_size=large");
	char * fetch = malloc(fetch_len);
	strcpy(fetch, PUBCHEM_URL "/rest/pug/compound/cid/");
	strcat(fetch, data->id);
	strcat(fetch, "/PNG?image_size=large");

	struct CURLResponse response = GetRequest(curl_handle, fetch);
	free(fetch);
	data->structure_type = PNG;
	data->structure = response.html;
	data->structure_size = response.size;

	curl_easy_cleanup(curl_handle);
	return 0;
}
int fetch_pubchem(struct data_generic * data, char * cas_num)
{
	CURL *curl_handle = curl_easy_init();


	size_t pubchem_fetch_len = PUBCHEM_URL_LEN + strlen(data->id) + sizeof("/rest/pug_view/data/compound/" "/XML");
	char * pubchem_fetch = malloc(pubchem_fetch_len);

	strcpy(pubchem_fetch, PUBCHEM_URL "/rest/pug_view/data/compound/");
	strcat(pubchem_fetch, data->id);
	strcat(pubchem_fetch, "/XML");

	struct CURLResponse response = GetRequest(curl_handle, pubchem_fetch);
	free(pubchem_fetch);

	//if (debug)
	//	printf("xml: %s\n", response.html);

	xmlDocPtr doc = xmlReadMemory(response.html, response.size, NULL, NULL, XML_PARSE_NOWARNING | XML_PARSE_NOERROR);
	free(response.html);
	if (doc == NULL) {
		return 1;
	}
	
	xmlXPathContextPtr xpath_ctx = xmlXPathNewContext(doc);
	if (xpath_ctx == NULL) {
		xmlFreeDoc(doc);
		curl_easy_cleanup(curl_handle);
		return 1;
	}

	xmlXPathRegisterNs(xpath_ctx, BAD_CAST "ns", BAD_CAST "http://pubchem.ncbi.nlm.nih.gov/pug_view");

	xmlXPathObjectPtr xpath_melt = xmlXPathEvalExpression(BAD_CAST "/ns:Record/ns:Section/ns:TOCHeading[text()='Chemical and Physical Properties']/../ns:Section/ns:TOCHeading[text()='Experimental Properties']/../ns:Section/ns:TOCHeading[text()='Melting Point']/../ns:Information/ns:Value", xpath_ctx);
	if (xpath_melt != NULL && xpath_melt->nodesetval != NULL) {
		data->melting_points = malloc(xpath_melt->nodesetval->nodeNr * sizeof(char *));
		data->melting_point_count = 0;
		for (int i = 0; i < xpath_melt->nodesetval->nodeNr; ++i) {
			xmlNodePtr value = xpath_melt->nodesetval->nodeTab[i];
			data->melting_points[i] = (char *) xmlNodeGetContent(value);
			++data->melting_point_count;
		}
	}
	xmlXPathFreeObject(xpath_melt);
	xmlXPathObjectPtr xpath_boil = xmlXPathEvalExpression(BAD_CAST "/ns:Record/ns:Section/ns:TOCHeading[text()='Chemical and Physical Properties']/../ns:Section/ns:TOCHeading[text()='Experimental Properties']/../ns:Section/ns:TOCHeading[text()='Boiling Point']/../ns:Information/ns:Value", xpath_ctx);
	if (xpath_boil != NULL && xpath_boil->nodesetval != NULL) {
		data->boiling_points = malloc(xpath_boil->nodesetval->nodeNr * sizeof(char *));
		data->boiling_point_count = 0;
		for (int i = 0; i < xpath_boil->nodesetval->nodeNr; ++i) {
			xmlNodePtr value = xpath_boil->nodesetval->nodeTab[i];
			data->boiling_points[i] = (char *) xmlNodeGetContent(value);
			++data->boiling_point_count;
		}
	}
	xmlXPathFreeObject(xpath_boil);
	xmlXPathObjectPtr xpath_sol = xmlXPathEvalExpression(BAD_CAST "/ns:Record/ns:Section/ns:TOCHeading[text()='Chemical and Physical Properties']/../ns:Section/ns:TOCHeading[text()='Experimental Properties']/../ns:Section/ns:TOCHeading[text()='Solubility']/../ns:Information/ns:Value", xpath_ctx);
	if (xpath_sol != NULL && xpath_sol->nodesetval != NULL) {
		data->solubilities = malloc(xpath_sol->nodesetval->nodeNr * sizeof(char *));
		data->solubility_count = 0;
		for (int i = 0; i < xpath_sol->nodesetval->nodeNr; ++i) {
			xmlNodePtr value = xpath_sol->nodesetval->nodeTab[i];
			data->solubilities[i] = (char *) xmlNodeGetContent(value);
			++data->solubility_count;
		}
	}
	xmlXPathFreeObject(xpath_sol);
	xmlXPathObjectPtr xpath_cas = xmlXPathEvalExpression(BAD_CAST "/ns:Record/ns:Section/ns:TOCHeading[text()='Names and Identifiers']/../ns:Section/ns:TOCHeading[text()='Other Identifiers']/../ns:Section/ns:TOCHeading[text()='CAS']/../ns:Information/ns:Value", xpath_ctx);
	if (xpath_cas != NULL && xpath_cas->nodesetval != NULL) {
		xmlNodePtr value = xpath_cas->nodesetval->nodeTab[0];
		char * value_content = (char *) xmlNodeGetContent(value);
		char * cas_num_stripped = strip_property(value_content);
		int cas[3];
		if (sscanf(cas_num_stripped, "%d-%d-%d", &cas[0], &cas[1], &cas[2]) == 3)
			snprintf(cas_num, 32, "%d-%d-%d", cas[0], cas[1], cas[2]);
		
		free(value_content);
	}
	xmlXPathFreeObject(xpath_cas);

	xmlXPathFreeContext(xpath_ctx);

	xmlFreeDoc(doc);
	curl_easy_cleanup(curl_handle);
	return 0;
}
int search_pubchem(char * encoded_search_term, const size_t encoded_search_len, char * cas_num, cJSON ** json_ptr)
{
	struct data_generic data;
	memset(&data, 0, sizeof(data));

	data.id = NULL;

	query_pubchem(&data, encoded_search_term, encoded_search_len);

	if (data.id) {
		fetch_pubchem(&data, cas_num);
		structure_pubchem(&data);
	}

	FLUSH(mode, DATABASE_NAME, &data, true, json_ptr);

	return 0;
}
