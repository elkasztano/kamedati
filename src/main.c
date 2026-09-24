#include <stdio.h>
#include <string.h>
#include "cli.h"
#include "lib.h"

#define OUT_BUF_SIZE 4096 /* Fits in 4KB OS memory page */

int main(int argc, char **argv) {
	Cli cli = cliParse(argc, argv);
	const char *template = cli.template;

	if (cli.syll_idtfr != NULL) {
		kame_map_syll_idtfr(cli.syll_idtfr);
		if (kame_errno == KAME_ERR_SYLLABLE_NA) {
			fprintf(stderr, "\x1b[93mWarning:\x1b[0m "
					"Specified syllable set not found. "
					"Default to Linear B.\n");
		}
	}

	kame_init(cli.seed, cli.flags);

	if (cli.flags & KAME_VERBOSE) {
		kame_print_combinations(template);
	}

	static char out_buf[OUT_BUF_SIZE];
	size_t out_buf_len = 0;

	for (int j = 0; j < cli.n; j++) {
		char password[256];

		kame_generate(template, password, sizeof(password));

		if (kame_errno != KAME_SUCCESS) {
			return 1;
		}

		size_t pass_len = strlen(password);

		/* Flush to stdout when full */
		if (out_buf_len + pass_len + 1 >= OUT_BUF_SIZE) {
			fwrite(out_buf, 1, out_buf_len, stdout);
			out_buf_len = 0;
		}

		memcpy(out_buf + out_buf_len, password, pass_len);
		out_buf_len += pass_len;
		out_buf[out_buf_len++] = '\n';
	}

	/* Flush any remaining output */
	if (out_buf_len > 0) {
		fwrite(out_buf, 1, out_buf_len, stdout);
	}

	/* safe destructor in case syllables have been loaded from file */
	kame_free_syllables();

	return 0;
}
