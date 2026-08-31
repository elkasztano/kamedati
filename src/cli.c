#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include "cli.h"
#include "lib.h"

#ifndef VERSION
#define VERSION "0.0"
#endif

#ifndef TRIPLET
#define TRIPLET "unknown"
#endif

void print_help_text(char *progname);

Cli cliParse(int argc, char **argv) {
        int c, option_index = 0;
        Cli cli;
        
        /* default values */
        cli.n = 5;        
        cli.template = "Sssnnx";
        cli.seed = 0ULL;
        cli.flags = 0;
	cli.syll_idtfr = NULL;

        while (1) {
                static struct option long_options[] = {
                        { "number",    required_argument, 0, 'n' },
                        { "template",  required_argument, 0, 't' },
			{ "syllables", required_argument, 0, 'b' },
			{ "seed",      required_argument, 0, 's' },
			{ "verbose",   no_argument, 	  0, 'v' },
			{ "help",      no_argument,       0, 'h' },
                        { "version",   no_argument,       0, 'V' },
                        { 0, 0, 0, 0 }
                };

                c = getopt_long(argc, argv, "hVvn:t:b:s:", long_options, &option_index);
                
                if (c == -1)
                        break;

                switch (c) {
                case 'n':
                        cli.n = (int)strtol(optarg, NULL, 10);
                        if (cli.n <= 0) {
                                fprintf(stderr, "Error: Number of passwords must be greater than 0.\n");
                                exit(1);
                        }
                        break;
                case 't':
                        cli.template = optarg;
                        break;
		case 'b':
			cli.syll_idtfr = optarg;
			break;
                case 's':
                        cli.seed = strtoull(optarg, NULL, 10);
			cli.flags |= KAME_PRNG;
                        break;
		case 'v':
			cli.flags |= KAME_VERBOSE;
			break;
                case 'h':
                        print_help_text(argv[0]);
                        exit(0);
                case 'V':
                        printf("Kamedati Password Generator %s\n%s\n", VERSION, TRIPLET);
                        exit(0);
                case '?':
                        print_help_text(argv[0]);
                        exit(1);
                default:
                        print_help_text(argv[0]);
                        exit(1);
                }
        }

        return cli;
}

void print_help_text(char *progname) {
        /* Strip the path from the program name if present for cleaner output */
        char *base_name = strrchr(progname, '/');
        if (base_name != NULL) {
                base_name++;
        } else {
                base_name = progname;
        }

        printf("Kamedati - A Pronounceable Linear B Script Password Generator\n\n");
        printf("Usage: %s [options]\n\n", base_name);
        printf("Options:\n");
        printf("  -n, --number <count>     Number of passwords to generate (default: 5)\n");
        printf("  -t, --template <string>  Structure pattern for the password (default: \"Sssnnx\")\n");
        printf("  -b, --syllables          Select syllable set, possible values: 'linearB' (default), 'alt'\n");
	printf("  -s, --seed <num>         switch to deterministic mode using specified Xorshift seed\n");
	printf("                           (Omit this flag for secure default /dev/urandom generation)\n");
	printf("  -v, --verbose            print template combination space to stderr\n");
	printf("  -h, --help               Show this help information text and exit\n");
        printf("  -V, --version            Display application version details and exit\n\n");
        printf("Template Tokens:\n");
        printf("  s : Generates a lowercase Linear B transliterated syllable (e.g., \"da\", \"ko\")\n");
        printf("  S : Generates a capitalized Linear B transliterated syllable (e.g., \"Da\", \"Ko\")\n");
	printf("  U : Generates a fully capitalized Linear B transliterated syllable (e.g., \"DA\", \"KO\")\n");
        printf("  n : Appends a numeric digit (0-9)\n");
        printf("  x : Appends a special symbol character (e.g., !, @, #, $, %%)\n");
	printf("  * : Any other character in the template is treated as a literal fallback string\n\n");
        printf("  \\ : Escape character, treats next character literally\n\n");
        printf("Examples:\n");
        printf("  %s -n 10 -t Sssnnx\n", base_name);
        printf("  %s --template SsSsnxnx\n", base_name);
        printf("  %s -s 1234567890 -t Sss-Sss-nnnn\n", base_name);
        printf("  %s -t 'Pa\\s\\sword: SssxnnSsssnx' -n 5\n", base_name);
	printf("  %s -t SssnnxSssnnx -b alt\n", base_name);
}
