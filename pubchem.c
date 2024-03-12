#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <curl/curl.h>

#include "args.h"
#include "curl_boilerplate.h"
#include "sixel_boilerplate.h"
#include "xml_boilerplate.h"

#define PUBCHEM_URL "https://pubchem.ncbi.nlm.nih.gov"
#define PUBCHEM_URL_LEN sizeof(PUBCHEM_URL)

struct pubchem_data {
	char * name_generic;
	char * name_IUPAC;
	char * cid;
	char * structure_2d;
	size_t structure_2d_size;
	char * molecular_formula;
	char * molecular_weight;
	size_t melting_point_count;
	char ** melting_points;
	size_t boiling_point_count;
	char ** boiling_points;
	size_t solubility_count;
	char ** solubilities;
};
static struct pubchem_data data;

int query_pubchem(char * encoded_search_term, const size_t encoded_search_len)
{
	CURL *curl_handle = curl_easy_init();

	size_t pubchem_search_len = PUBCHEM_URL_LEN + encoded_search_len + sizeof("/rest/pug" "/compound" "/name/" "/property/MolecularFormula,MolecularWeight,IUPACName/XML");

	char * pubchem_search = malloc(pubchem_search_len);

	strcpy(pubchem_search, PUBCHEM_URL "/rest/pug");
	strcat(pubchem_search, "/compound");
	strcat(pubchem_search, "/name/");
	strcat(pubchem_search, encoded_search_term);
	strcat(pubchem_search, "/property/MolecularFormula,MolecularWeight,IUPACName/XML");

	if (debug)
		printf("curling: %s\n", pubchem_search);
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

			if (strcmp((char *) current_node->name, "MolecularFormula") == 0)
				data.molecular_formula = (char *) xmlNodeGetContent(current_node);
			if (strcmp((char *) current_node->name, "MolecularWeight") == 0)
				data.molecular_weight = (char *) xmlNodeGetContent(current_node);
			if (strcmp((char *) current_node->name, "IUPACName") == 0)
				data.name_IUPAC = (char *) xmlNodeGetContent(current_node);
			if (strcmp((char *) current_node->name, "CID") == 0)
				data.cid = (char *) xmlNodeGetContent(current_node);
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
int structure_pubchem()
{
	CURL *curl_handle = curl_easy_init();

	size_t fetch_len = PUBCHEM_URL_LEN + strlen((char *) data.cid) + sizeof("/rest/pug/compound/cid/" "/PNG?image_size=large");
	char * fetch = malloc(fetch_len);
	strcpy(fetch, PUBCHEM_URL "/rest/pug/compound/cid/");
	strcat(fetch, data.cid);
	strcat(fetch, "/PNG?image_size=large");

	if (debug)
		printf("curling: %s\n", fetch);
	struct CURLResponse response = GetRequest(curl_handle, fetch);
	free(fetch);
	data.structure_2d = response.html;
	data.structure_2d_size = response.size;

	curl_easy_cleanup(curl_handle);
	return 0;
}
int fetch_pubchem(char * cas_num)
{
	CURL *curl_handle = curl_easy_init();


	size_t pubchem_fetch_len = PUBCHEM_URL_LEN + strlen(data.cid) + sizeof("/rest/pug_view/data/compound/" "/XML");
	char * pubchem_fetch = malloc(pubchem_fetch_len);

	strcpy(pubchem_fetch, PUBCHEM_URL "/rest/pug_view/data/compound/");
	strcat(pubchem_fetch, data.cid);
	strcat(pubchem_fetch, "/XML");

	if (debug)
		printf("curling: %s\n", pubchem_fetch);
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
		data.melting_points = malloc(xpath_melt->nodesetval->nodeNr * sizeof(char *));
		data.melting_point_count = 0;
		for (int i = 0; i < xpath_melt->nodesetval->nodeNr; ++i) {
			xmlNodePtr value = xpath_melt->nodesetval->nodeTab[i];
			data.melting_points[i] = (char *) xmlNodeGetContent(value);
			++data.melting_point_count;
		}
	}
	xmlXPathFreeObject(xpath_melt);
	xmlXPathObjectPtr xpath_boil = xmlXPathEvalExpression(BAD_CAST "/ns:Record/ns:Section/ns:TOCHeading[text()='Chemical and Physical Properties']/../ns:Section/ns:TOCHeading[text()='Experimental Properties']/../ns:Section/ns:TOCHeading[text()='Boiling Point']/../ns:Information/ns:Value", xpath_ctx);
	if (xpath_boil != NULL && xpath_boil->nodesetval != NULL) {
		data.boiling_points = malloc(xpath_boil->nodesetval->nodeNr * sizeof(char *));
		data.boiling_point_count = 0;
		for (int i = 0; i < xpath_boil->nodesetval->nodeNr; ++i) {
			xmlNodePtr value = xpath_boil->nodesetval->nodeTab[i];
			data.boiling_points[i] = (char *) xmlNodeGetContent(value);
			++data.boiling_point_count;
		}
	}
	xmlXPathFreeObject(xpath_boil);
	xmlXPathObjectPtr xpath_sol = xmlXPathEvalExpression(BAD_CAST "/ns:Record/ns:Section/ns:TOCHeading[text()='Chemical and Physical Properties']/../ns:Section/ns:TOCHeading[text()='Experimental Properties']/../ns:Section/ns:TOCHeading[text()='Solubility']/../ns:Information/ns:Value", xpath_ctx);
	if (xpath_sol != NULL && xpath_sol->nodesetval != NULL) {
		data.solubilities = malloc(xpath_sol->nodesetval->nodeNr * sizeof(char *));
		data.solubility_count = 0;
		for (int i = 0; i < xpath_sol->nodesetval->nodeNr; ++i) {
			xmlNodePtr value = xpath_sol->nodesetval->nodeTab[i];
			data.solubilities[i] = (char *) xmlNodeGetContent(value);
			++data.solubility_count;
		}
	}
	xmlXPathFreeObject(xpath_sol);
	xmlXPathObjectPtr xpath_cas = xmlXPathEvalExpression(BAD_CAST "/ns:Record/ns:Section/ns:TOCHeading[text()='Names and Identifiers']/../ns:Section/ns:TOCHeading[text()='Other Identifiers']/../ns:Section/ns:TOCHeading[text()='CAS']/../ns:Information/ns:Value", xpath_ctx);
	if (xpath_cas != NULL && xpath_cas->nodesetval != NULL) {
		xmlNodePtr value = xpath_cas->nodesetval->nodeTab[0];
		char * value_content = (char *) xmlNodeGetContent(value);
		strcpy(cas_num, strip_property(value_content));
		free(value_content);
	}
	xmlXPathFreeObject(xpath_cas);

	xmlXPathFreeContext(xpath_ctx);

	xmlFreeDoc(doc);
	curl_easy_cleanup(curl_handle);
	return 0;
}
int flush_pubchem()
{
	printf("PubChem:\n");

	if (print_png((unsigned char *) data.structure_2d, data.structure_2d_size, 300, 300)) {
		printf("failed to render structure\n");
	}
	free(data.structure_2d);
	printf("pubchem Id: %s\n", data.cid);
	free(data.cid);
	printf("IUPAC name: %s\n", strip_property(data.name_IUPAC));
	free(data.name_IUPAC);
	printf("Molecular Formula: %s\n", strip_property(data.molecular_formula));
	free(data.molecular_formula);
	printf("Molecular Weight: %s\n", strip_property(data.molecular_weight));
	free(data.molecular_weight);

	if (data.melting_point_count) {
		printf("Melting Point(s):\n");
		for (int x = 0; x < data.melting_point_count; ++x) {
			printf("\t%s\n", strip_property(data.melting_points[x]));
			free(data.melting_points[x]);
		}
	}
	if (data.melting_points)
		free(data.melting_points);

	if (data.boiling_point_count) {
		printf("Boiling Point(s):\n");
		for (int x = 0; x < data.boiling_point_count; ++x) {
			printf("\t%s\n", strip_property(data.boiling_points[x]));
			free(data.boiling_points[x]);
		}
	}
	if (data.boiling_points)
		free(data.boiling_points);

	if (data.solubility_count) {
		printf("Solubility:\n");
		for (int x = 0; x < data.solubility_count; ++x) {
			printf("\t%s\n", strip_property(data.solubilities[x]));
			free(data.solubilities[x]);
		}
	}
	if (data.solubilities)
		free(data.solubilities);

	return 0;
}
int search_pubchem(char * encoded_search_term, const size_t encoded_search_len, char * cas_num)
{
	data.cid = NULL;

	query_pubchem(encoded_search_term, encoded_search_len);

	if (!data.cid)
		return 1;

	fetch_pubchem(cas_num);
	structure_pubchem();

	flush_pubchem();

	return 0;
}
