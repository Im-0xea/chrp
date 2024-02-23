#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "curl_boilerplate.h"

size_t WriteHTMLCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct CURLResponse *mem = (struct CURLResponse *)userp;
	char *ptr = realloc(mem->html, mem->size + realsize + 1);
	if (!ptr)
	{
		printf("Not enough memory available (realloc returned NULL)\n");
		return 0;
	}
	mem->html = ptr;
	memcpy(&(mem->html[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->html[mem->size] = 0;
	return realsize;
}

struct CURLResponse GetRequest(CURL *curl_handle, const char *url)
{
	CURLcode res;
	struct CURLResponse response;
	response.html = malloc(1);
	response.size = 0;
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteHTMLCallback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&response);
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/117.0.0.0 Safari/537.36");
	res = curl_easy_perform(curl_handle);

	if (res != CURLE_OK)
		fprintf(stderr, "GET request failed: %s\n", curl_easy_strerror(res));

	return response;
}
