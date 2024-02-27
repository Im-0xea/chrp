#include <ctype.h>
#include <string.h>

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
