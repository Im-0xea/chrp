#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <curl/curl.h>

#include "args.h"
#include "curl_boilerplate.h"
#include "sixel_boilerplate.h"
#include "xml_boilerplate.h"

#define CHEMSPIDER_URL "https://www.chemspider.com"
#define CHEMSPIDER_URL_LEN sizeof(CHEMSPIDER_URL)

static struct chemspider_data {
	char * name_generic;
	char * id;
	char * structure_2d;
	size_t structure_2d_size;
	char * molecular_formula;
	char * molecular_weight;
	char * density;
	size_t melting_point_count;
	char ** melting_points;
	size_t boiling_point_count;
	char ** boiling_points;
} data;

int query_chemspider(char * encoded_search_term, const size_t encoded_search_len, char ** redirect_url)
{
	CURL *curl_handle = curl_easy_init();

	size_t chemspider_search_len = CHEMSPIDER_URL_LEN + sizeof("/Search.aspx?q=") + encoded_search_len;
	char * chemspider_search = malloc(chemspider_search_len);
	strcpy(chemspider_search, CHEMSPIDER_URL "/Search.aspx?q=");
	strcat(chemspider_search, encoded_search_term);
	if (debug)
		printf("curling: %s\n", chemspider_search);
	struct CURLResponse response = GetRequest(curl_handle, chemspider_search);
	free(chemspider_search);
	//if (debug)
	//	printf("xml: %s\n", response.html);

	htmlDocPtr doc = htmlReadMemory(response.html, response.size, NULL, NULL, HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
	free(response.html);

	xmlXPathContextPtr xpath_ctx = xmlXPathNewContext(doc);
	if (xpath_ctx == NULL) {
		xmlFreeDoc(doc);
		curl_easy_cleanup(curl_handle);
		return 1;
	}

	xmlXPathObjectPtr xpath_query = xmlXPathEvalExpression(BAD_CAST "/html/body/h2[contains(text(),'Object moved to')]/a", xpath_ctx);
	if (xpath_query != NULL && xpath_query->nodesetval != NULL) {
		xmlNodePtr query_link = xpath_query->nodesetval->nodeTab[0];
		*redirect_url = (char *) xmlGetProp(query_link, (const xmlChar *)"href");
	}
	xmlXPathFreeObject(xpath_query);

	if (!*redirect_url) {
		xmlXPathObjectPtr xpath_multi_query = xmlXPathEvalExpression(BAD_CAST "/html/body[@class='rsc-ui']/form[@id='aspnetForm']/div[@id='pnlMainContent']/div[@class='content-wrapper']/div[@id='result']/div[@id='ctl00_ctl00_ContentSection_ContentPlaceHolder1_UpdatePanelResults']/div[@id='ctl00_ctl00_ContentSection_ContentPlaceHolder1_ResultViewControl1_result']/div[@class='results-wrapper table']/div/table[@class='zebra1 view-grid']/tbody/tr[not(@class='alt-row')]/td[@class='search-id-column']/a[not(@title)]", xpath_ctx);
		if (xpath_multi_query != NULL && xpath_multi_query->nodesetval != NULL) {
			xmlNodePtr multi_query_link = xpath_multi_query->nodesetval->nodeTab[0];
			*redirect_url = (char *) xmlGetProp(multi_query_link, (const xmlChar *)"href");
		}
		xmlXPathFreeObject(xpath_multi_query);
	}

	xmlXPathFreeContext(xpath_ctx);
	xmlFreeDoc(doc);
	curl_easy_cleanup(curl_handle);
	return 0;
}
int fetch_chemspider(char * redirect_url)
{
	CURL *curl_handle = curl_easy_init();

	size_t fetch_len = CHEMSPIDER_URL_LEN + strlen((char *) redirect_url);
	char * fetch_url = malloc(fetch_len);
	strcpy(fetch_url, CHEMSPIDER_URL);
	strcat(fetch_url, (char *) redirect_url);
	xmlFree(redirect_url);
	if (debug)
		printf("curling: %s\n", fetch_url);
	struct CURLResponse response = GetRequest(curl_handle, fetch_url);
	free(fetch_url);
	//if (debug)
	//	printf("xml: %s\n", response.html);

	htmlDocPtr doc = htmlReadMemory(response.html, response.size, NULL, NULL, HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);

	free(response.html);
	if (doc == NULL) {
		curl_easy_cleanup(curl_handle);
		return 1;
	}

	xmlXPathContextPtr xpath_ctx = xmlXPathNewContext(doc);
	if (xpath_ctx == NULL) {
		xmlFreeDoc(doc);
		curl_easy_cleanup(curl_handle);
		return 1;
	}
	xmlXPathObjectPtr xpath_mol_form = xmlXPathEvalExpression(BAD_CAST "/html/body[@class='rsc-ui']/form[@id='aspnetForm']/div[@class='main-content viewport']/div[@class='content-wrapper']/div[@id='ctl00_ctl00_ContentSection_ContentPlaceHolder1_RecordViewDetails_rptDetailsView_ctl00_pnlDetailsView']/div[@class='structure-head']/ul[@class='struct-props']/li/span[@id='ctl00_ctl00_ContentSection_ContentPlaceHolder1_RecordViewDetails_rptDetailsView_ctl00_prop_MF']", xpath_ctx);
	xmlXPathObjectPtr xpath_mol_weight = xmlXPathEvalExpression(BAD_CAST "/html/body[@class='rsc-ui']/form[@id='aspnetForm']/div[@class='main-content viewport']/div[@class='content-wrapper']/div[@id='ctl00_ctl00_ContentSection_ContentPlaceHolder1_RecordViewDetails_rptDetailsView_ctl00_pnlDetailsView']/div[@class='structure-head']/ul[@class='struct-props']/li/span[text()='Average mass']/parent::*/text()", xpath_ctx);
	xmlXPathObjectPtr xpath_melt_a = xmlXPathEvalExpression(BAD_CAST "/html/body[@class='rsc-ui']/form[@id='aspnetForm']/div[@class='main-content viewport']/div[@class='content-wrapper']/div[@id='ctl00_ctl00_ContentSection_ContentPlaceHolder1_RecordViewTabDetailsControl_updatepanel_prop_ctl']/div[@class='hidden-tab-container']/div[@class='info-tabs']/div[@id='pred_ib_ph']/div[@id='pred_tabs']/div[@id='suppInfoTab']/div[@class='supp-info']/ul[@class='user_data_categories_list']/li/ul[@class='expand-list']/li/h2/a/span[text()='Experimental Melting Point:']/../../../table[@style='display:none']/tr/td/text()", xpath_ctx);
	xmlXPathObjectPtr xpath_melt_b = xmlXPathEvalExpression(BAD_CAST "/html/body[@class='rsc-ui']/form[@id='aspnetForm']/div[@id='pnlMainContent']/div[@class='content-wrapper']/div[@id='ctl00_ctl00_ContentSection_ContentPlaceHolder1_RecordViewTabDetailsControl_updatepanel_prop_ctl']/div[@class='hidden-tab-container']/div[@class='info-tabs']/div[@id='pred_ib_ph']/div[@id='pred_tabs']/div[@id='acdLabsTab']/div/div[@class='AspNet-FormView']/div[@class='AspNet-FormView-Data']/div/table/tr/td[@class='prop_title' and text()='Melting Point:']/../td[@class='prop_value_nowrap']/text()", xpath_ctx);
	xmlXPathObjectPtr xpath_boil_a = xmlXPathEvalExpression(BAD_CAST "/html/body[@class='rsc-ui']/form[@id='aspnetForm']/div[@class='main-content viewport']/div[@class='content-wrapper']/div[@id='ctl00_ctl00_ContentSection_ContentPlaceHolder1_RecordViewTabDetailsControl_updatepanel_prop_ctl']/div[@class='hidden-tab-container']/div[@class='info-tabs']/div[@id='pred_ib_ph']/div[@id='pred_tabs']/div[@id='suppInfoTab']/div[@class='supp-info']/ul[@class='user_data_categories_list']/li/ul[@class='expand-list']/li/h2[@class='section-head']/a/span[text()='Experimental Boiling Point:']/../../../table/tr/td/text()", xpath_ctx);
	xmlXPathObjectPtr xpath_boil_b = xmlXPathEvalExpression(BAD_CAST "/html/body[@class='rsc-ui']/form[@id='aspnetForm']/div[@id='pnlMainContent']/div[@class='content-wrapper']/div[@id='ctl00_ctl00_ContentSection_ContentPlaceHolder1_RecordViewTabDetailsControl_updatepanel_prop_ctl']/div[@class='hidden-tab-container']/div[@class='info-tabs']/div[@id='pred_ib_ph']/div[@id='pred_tabs']/div[@id='acdLabsTab']/div[not(@style='display: none')]/div[@class='AspNet-FormView']/div[@class='AspNet-FormView-Data']/div/table/tr/td[@class='prop_title' and text()='Boiling Point:']/../td[@class='prop_value_nowrap']/text()", xpath_ctx);
	xmlXPathObjectPtr xpath_flash_b = xmlXPathEvalExpression(BAD_CAST "/html/body[@class='rsc-ui']/form[@id='aspnetForm']/div[@id='pnlMainContent']/div[@class='content-wrapper']/div[@id='ctl00_ctl00_ContentSection_ContentPlaceHolder1_RecordViewTabDetailsControl_updatepanel_prop_ctl']/div[@class='hidden-tab-container']/div[@class='info-tabs']/div[@id='pred_ib_ph']/div[@id='pred_tabs']/div[@id='acdLabsTab']/div[not(@style='display: none')]/div[@class='AspNet-FormView']/div[@class='AspNet-FormView-Data']/div/table/tr/td[@class='prop_title' and text()='Flash Point:']/../td[@class='prop_value_nowrap']/text()", xpath_ctx);
	xmlXPathObjectPtr xpath_density = xmlXPathEvalExpression(BAD_CAST "/html/body[@class='rsc-ui']/form[@id='aspnetForm']/div[@id='pnlMainContent']/div[@class='content-wrapper']/div[@id='ctl00_ctl00_ContentSection_ContentPlaceHolder1_RecordViewTabDetailsControl_updatepanel_prop_ctl']/div[@class='hidden-tab-container']/div[@class='info-tabs']/div[@id='pred_ib_ph']/div[@id='pred_tabs']/div[@id='acdLabsTab']/div[not(@style='display: none')]/div[@class='AspNet-FormView']/div[@class='AspNet-FormView-Data']/div/table/tr/td[@class='prop_title' and text()='Density:']/../td[@class='prop_value_nowrap']/text()", xpath_ctx);

	if (xpath_mol_form != NULL && xpath_mol_form->nodesetval != NULL) {
		xmlNodePtr mol_form = xpath_mol_form->nodesetval->nodeTab[xpath_mol_form->nodesetval->nodeNr - 1];
		data.molecular_formula = (char *) xmlNodeGetContent(mol_form);
	}
	if (xpath_mol_weight != NULL && xpath_mol_weight->nodesetval != NULL) {
		xmlNodePtr mol_weight = xpath_mol_weight->nodesetval->nodeTab[xpath_mol_weight->nodesetval->nodeNr - 1];
		data.molecular_weight = (char *) xmlNodeGetContent(mol_weight);
	}
	data.melting_points = malloc(2 * sizeof(char *));
	data.melting_point_count = 0;
	data.boiling_points = malloc(2 * sizeof(char *));
	data.boiling_point_count = 0;
	if (xpath_melt_a != NULL && xpath_melt_a->nodesetval != NULL) {
		xmlNodePtr melt_a = xpath_melt_a->nodesetval->nodeTab[0];
		data.melting_points[data.melting_point_count++] = (char *) xmlNodeGetContent(melt_a);
	}
	if (xpath_melt_b != NULL && xpath_melt_b->nodesetval != NULL) {
		xmlNodePtr melt_b = xpath_melt_b->nodesetval->nodeTab[0];
		data.melting_points[data.melting_point_count++] = (char *) xmlNodeGetContent(melt_b);
	}
	if (xpath_boil_a != NULL && xpath_boil_a->nodesetval != NULL) {
		xmlNodePtr boil_a = xpath_boil_a->nodesetval->nodeTab[0];
		data.boiling_points[data.boiling_point_count++] = (char *) xmlNodeGetContent(boil_a);
	}
	if (xpath_boil_b != NULL && xpath_boil_b->nodesetval != NULL && xpath_boil_b->nodesetval->nodeNr != 0) {
		xmlNodePtr boil_b = xpath_boil_b->nodesetval->nodeTab[0];
		data.boiling_points[data.boiling_point_count++] = (char *) xmlNodeGetContent(boil_b);
	}
	//if (xpath_flash_b != NULL && xpath_flash_b->nodesetval != NULL) {
	//	xmlNodePtr flash_b = xpath_flash_b->nodesetval->nodeTab[0];
	//	printf("FlashPoint: %s\n", strip_property(xmlNodeGetContent(flash_b)));
	//}
	if (xpath_density != NULL && xpath_density->nodesetval != NULL) {
		xmlNodePtr density = xpath_density->nodesetval->nodeTab[0];
		data.density = (char *) xmlNodeGetContent(density);
	}

	xmlXPathFreeObject(xpath_mol_form);
	xmlXPathFreeObject(xpath_mol_weight);
	xmlXPathFreeObject(xpath_melt_a);
	xmlXPathFreeObject(xpath_melt_b);
	xmlXPathFreeObject(xpath_boil_a);
	xmlXPathFreeObject(xpath_boil_b);
	xmlXPathFreeObject(xpath_flash_b);
	xmlXPathFreeObject(xpath_density);
	xmlXPathFreeContext(xpath_ctx);

	xmlFreeDoc(doc);
	curl_easy_cleanup(curl_handle);
	return 0;
}
int structure_chemspider(char * redirect_url)
{
	unsigned long id_num;
	sscanf(redirect_url, "/Chemical-Structure.%lu", &id_num);
	data.id = malloc(64 * sizeof(char));
	if (snprintf(data.id, 64, "%lu", id_num) < 1)
		return 1;

	CURL *curl_handle = curl_easy_init();
	size_t fetch_len = CHEMSPIDER_URL_LEN + strlen((char *) data.id) + sizeof("/ImagesHandler.ashx?id=" "&w=300&h=300");
	char * fetch = malloc(fetch_len);
	strcpy(fetch, CHEMSPIDER_URL "/ImagesHandler.ashx?id=");
	strcat(fetch, data.id);
	strcat(fetch, "&w=300&h=300");

	if (debug)
		printf("curling: %s\n", fetch);
	struct CURLResponse response = GetRequest(curl_handle, fetch);
	free(fetch);
	data.structure_2d = response.html;
	data.structure_2d_size = response.size;
	
	curl_easy_cleanup(curl_handle);
	return 0;
}
int flush_chemspider()
{
	printf("ChemSpider:\n");

	if (!data.id) {
		printf("not found\n\n");
		return 1;
	}

	if (print_png((unsigned char *) data.structure_2d, data.structure_2d_size, 300, 300)) {
		printf("failed to render structure\n");
	}
	free(data.structure_2d);
	printf("ChemSpider Id: %s\n", data.id);
	free(data.id);
	if (data.molecular_formula) {
		printf("Molecular Formula: %s\n", strip_property(data.molecular_formula));
		free(data.molecular_formula);
	}
	if (data.molecular_weight) {
		printf("Molecular Weight: %s\n", strip_property(data.molecular_weight));
		free(data.molecular_weight);
	}
	if (data.density) {
		printf("Density: %s\n", strip_property(data.density));
		free(data.density);
	}

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

	printf("\n");
	return 0;
}
int search_chemspider(char * encoded_search_term, const size_t encoded_search_len)
{
	char * redirect_url = NULL;
	query_chemspider(encoded_search_term, encoded_search_len, &redirect_url);

	if (redirect_url) {
		structure_chemspider(redirect_url);
		fetch_chemspider(redirect_url);
	}

	flush_chemspider();

	return 0;
}
