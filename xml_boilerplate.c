#include <ctype.h>
#include <string.h>

#include <libxml/HTMLparser.h>

int html_format(const char * in, char ** out)
{
	xmlDocPtr doc = htmlReadMemory(in, strlen(in), NULL, NULL, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
	if (!doc)
		return 1;

	xmlNodePtr root = xmlDocGetRootElement(doc);
	if (!root) {
		xmlFreeDoc(doc);
		return 1;
	}

	*out = (char *) xmlNodeGetContent(root);
	xmlFreeDoc(doc);
	return 0;
}

char * strip_property(char * text)
{
	if (!text)
		return text;

	while (isspace(*text))
		++text;

	size_t len = strlen((char *) text);
	for (int x = len - 1; x > 0; --x) {
		if (!isspace(text[x]))
			break;

		text[x] = '\0';
	}

	int c = 0;
	for (int x = 0; x < len; ++x) {
		if (text[x] == '\n')
			text[x] = ' ';
		if (isspace(text[x])) {
			++c;
		} else {
			if (c > 1)
				memmove(&text[x - (c - 1)], &text[x], len - x);
			c = 0;
		}
	}

	return text;
}
