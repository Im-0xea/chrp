#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>

bool debug = false;

int main(const int c, char * const * v)
{
	const char * short_options = "hVc:d";
	const struct option long_options[] = {
		{"help",    no_argument,       0, 'h'},
		{"version", no_argument,       0, 'V'},
		{"config",  required_argument, 0, 'c'},
		{"debug",   no_argument,       0, 'd'},
		{0, 0, 0, 0},
	};

	char * config = NULL;
	do {
		const int opt = getopt_long(c, v, short_options, long_options, NULL);

		if (opt == -1)
			break;

		switch (opt) {
		default:
		case '?': // Unrecognized option
			return 1;
		case 'h':
			printf("Usage: chrp [OPTION] [EXECUTABLES]\n\n"
			     " -h, --help    -> prints this\n"
			     " -V, --version -> prints version\n"
			     " -c, --config  -> set config file\n");
			return 0;
		case 'V':
			printf("Chrp Version %.1f\n", 0.1);
			return 0;
		case 'c':
			config = optarg;
			break;
		case 'd':
			debug = true;
			break;
		}
	} while (1);

	return search(optind, c, v);
}
