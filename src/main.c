#include <stdio.h>
#include <string.h>
#include "cli.h"
#include "lib.h"

int main(int argc, char **argv) {
	Cli cli = cliParse(argc, argv);
	const char *template = cli.template;

	kame_init(cli.seed, cli.flags);

	if (cli.flags & KAME_VERBOSE) {
		kame_print_combinations(template);
	}

	for (int j = 0; j < cli.n; j++) {
		char password[256];
		
		kame_generate(template, password, sizeof(password));

		/* Check our custom errno variable for failures */
		if (kame_errno != KAME_SUCCESS) {
			if (kame_errno == KAME_ERR_URANDOM_OPEN) {
				fprintf(stderr, "Runtime Error: Could not open entropy pool.\n");
			} else if (kame_errno == KAME_ERR_URANDOM_READ) {
				fprintf(stderr, "Runtime Error: Failed to read from entropy pool.\n");
			}
			return 1; /* Exit main gracefully rather than crashing in the library */
		}

		printf("%s\n", password);
	}

	return 0;
}
