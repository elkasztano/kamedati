#ifndef CLI_H
#define CLI_H
#include <stdint.h>

typedef struct {
	int n;
	char *template;
	char *syll_idtfr;
	uint64_t seed;
	uint32_t flags;
} Cli;

Cli cliParse( int argc, char **argv );

#endif
