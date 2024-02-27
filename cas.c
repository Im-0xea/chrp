#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <curl/curl.h>

#include "args.h"
#include "curl_boilerplate.h"


#define CAS_URL "https://commonchemistry.cas.org/detail?cas_rn="

int search_cas(const char * cas_num)
{
	return 0;
}
