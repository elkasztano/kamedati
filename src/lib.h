#ifndef LIB_H
#define LIB_H

#include <stdint.h>
#include <stddef.h>

/* Configuration Flags */
#define KAME_PRNG     (1 << 0)
#define KAME_VERBOSE  (1 << 1)

/* Kamedati Error Codes */
typedef enum {
	KAME_SUCCESS = 0,
	KAME_ERR_URANDOM_OPEN,  /* /dev/urandom could not be opened */
	KAME_ERR_URANDOM_READ,	/* /dev/urandom read operation failed */
	KAME_ERR_SYLLABLE_NA    /* syllable identifier not available */
} KameError;

/* Global error status variable, similar to errno */
extern int kame_errno;

extern const char *linear_b_syllables[];
extern const char *alt_syllables[];
extern const char digits[];
extern const char specials[];
extern int num_syllables_avail;
extern int num_digits_avail;
extern int num_specials_avail;

void kame_init(uint64_t seed, uint32_t flags);
void kame_generate(const char *template, char *out_password, size_t max_len, FILE *urand);
void kame_print_combinations(const char *template);
void kame_map_syll_idtfr(const char *identifier);
void kame_free_syllables(void);

#endif /* LIB_H */
