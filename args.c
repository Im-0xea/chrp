#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "search.h"

bool debug = false;

enum {
	WEB,
	TUI
} mode = TUI;

int main(const int c, char ** v)
{
	const char * short_options = "hVc:d";
	const struct option long_options[] = {
		{"help",    no_argument,       0, 'h'},
		{"version", no_argument,       0, 'V'},
		{"config",  required_argument, 0, 'c'},
		{"debug",   no_argument,       0, 'd'},
		{"mode",    required_argument, 0, 'm'},
		{"port",    required_argument, 0, 'p'},
		{0, 0, 0, 0},
	};

	//char * config = NULL;
	int port = 0;
	do {
		const int opt = getopt_long(c, v, short_options, long_options, NULL);

		if (opt == -1)
			break;

		switch (opt) {
			default:
			case '?': // Unrecognized option
				return 1;
			case 'h':
				printf("Usage: chrp [OPTION] [SEARCH TERM] / [SERVER ADDRESS]\n\n"
						" -h, --help    -> prints this\n"
						" -V, --version -> prints version\n"
						" -c, --config  -> set config file\n"
						" -m, --mode    -> set operation mode:\n"
						"                  * \"tui\" (prints results as text and sixels\n"
						"                  * \"web\" (runs as api server)\n"
						" -p, --port    -> set port for api server\n");
			return 0;
		case 'V':
			printf("Chrp Version %.1f\n", 0.2);
			return 0;
		case 'c':
			//config = optarg;
			break;
		case 'd':
			debug = true;
			break;
		case 'm':
			if (strcmp(optarg, "web") == 0) {
				mode = WEB;
			} else if (strcmp(optarg, "tui") == 0) {
				mode = TUI;
			} else {
				printf("entered invalid operation mode: %s\n", optarg);
				return 1;
			}
			break;
		case 'p':
			port = atoi(optarg);
			if (!port) {
				printf("invalid port: %s\n", optarg);
				return 1;
			}
			break;
		}
	} while (1);

	if (mode == TUI)
		return search_term(optind, c, v);
	else if (mode == WEB)
		return search_daemon(optind, c, v, port);

	printf("invalid mode: %d\n", mode);
	return 1;
}
