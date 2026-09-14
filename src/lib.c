#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include "lib.h"

#define KAME_ENTROPY_BUF_SIZE 512 /* 4096 bytes */

/* Hidden state for the engine */
static uint64_t prng_state;
static uint32_t engine_flags = 0;
int kame_errno = KAME_SUCCESS;

const char *linear_b_syllables[] = {
        "a", "e", "i", "o", "u",
        "da", "de", "di", "do", "du",
        "ta", "te", "ti", "to", "tu",
        "pa", "pe", "pi", "po", "pu",
        "ka", "ke", "ki", "ko", "ku",
        "ra", "re", "ri", "ro", "ru",
        "ma", "me", "mi", "mo", "mu",
        "na", "ne", "ni", "no", "nu",
        "sa", "se", "si", "so", "su",
        "wa", "we", "wi", "wo",
        "za", "ze", "zo"
};

const char *alt_syllables[] = {
	"ba", "be", "bi", "bo", "bu", "bee",
	"ca", "ce", "ci", "co", "cu", "coo",
	"da", "de", "di", "do", "du", "dee",
	"fa", "fe", "fi", "fo", "fu", "foo",
	"ga", "ge", "gi", "go", "gu", "gee",
	"ha", "he", "hi", "ho", "hu", "hoo",
	"ja", "je", "ji", "jo", "ju", "joe",
	"la", "le", "li", "lo", "lu", "lee",
	"na", "ne", "nie", "no", "nu", "noo",
	"ma", "me", "mi", "mo", "mu", "mee",
	"pa", "pe", "pi", "po", "pu", "pie",
	"qua", "que", "qu", "quo",
	"ra", "re", "ri", "ro", "ru", "roo",
	"sa", "se", "si", "so", "su", "see",
	"ta", "te", "ti", "to", "tu", "too",
	"va", "ve", "vi", "vo", "vu", "vee",
	"wa", "we", "wi", "wo", "wu", "woo",
	"xa", "xe", "xi", "xo", "xu", "xee",
	"za", "ze", "zi", "zo", "zu", "zoo",
	"ya", "ye", "yi", "yo", "yu", "yee"
};

const char digits[] = "0123456789";
const char specials[] = "!@#$%^&*()-_=+[]{}";

/* default values */
const char **syllables = linear_b_syllables;
int num_syllables_avail = sizeof(linear_b_syllables) / sizeof(linear_b_syllables[0]);

int num_digits_avail = sizeof(digits) - 1;
int num_specials_avail = sizeof(specials) - 1;

/* Splitmix64 by Sebastiano Vigna
*  https://prng.di.unimi.it/splitmix64.c
*/
uint64_t prng() {
	uint64_t x = (prng_state += 0x9e3779b97f4a7c15);
	x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
	x = (x ^ (x >> 27)) * 0x94d049bb133111eb;
	return x ^ (x >> 31);
}

static uint64_t get_entropy64(FILE *urand) {
	static uint64_t buffer[KAME_ENTROPY_BUF_SIZE];
	static size_t buf_idx = KAME_ENTROPY_BUF_SIZE;

	if (!(engine_flags & KAME_PRNG)) {
		/* Refill internal buffer when exhausted */
		if (buf_idx >= KAME_ENTROPY_BUF_SIZE) {
			if (urand == NULL) {
				kame_errno = KAME_ERR_URANDOM_OPEN;
				return 0;
			}

			if (fread(buffer, sizeof(uint64_t), KAME_ENTROPY_BUF_SIZE, urand) != KAME_ENTROPY_BUF_SIZE) {
				kame_errno = KAME_ERR_URANDOM_READ;
				return 0;
			}
			buf_idx = 0;
		}

		return buffer[buf_idx++];
	}

	/* OPT-IN PATH: Deterministic PRNG */
	return prng();
}

/* Lemire's fastrange algorithm: fast uniform range reduction
 * Lemire, D. (2018). Fast Random Integer Generation in an Interval.
 * arXiv:1805.10941 */
static uint64_t rand_range(uint64_t range, FILE *urand) {
	if (range <= 1) {
		return 0;
	}

	uint64_t x = get_entropy64(urand);
	if (kame_errno != KAME_SUCCESS) {
		return 0;
	}

	unsigned __int128 multi = (unsigned __int128)x * range;
	uint64_t leftover = (uint64_t)multi;

	/* Lemire rejection sampling branch for exact unbiased reduction */
	if (leftover < range) {
		uint64_t threshold = -range % range; /* Equivalent to (2^64) % range */
		while (leftover < threshold) {
			x = get_entropy64(urand);
			if (kame_errno != KAME_SUCCESS) {
				return 0;
			}
			multi = (unsigned __int128)x * range;
			leftover = (uint64_t)multi;
		}
	}

	return (uint64_t)(multi >> 64);
}

void kame_init(uint64_t seed, uint32_t flags) {
	engine_flags = flags;
	kame_errno = KAME_SUCCESS; /* Reset error state on re-initialization */

	if (engine_flags & KAME_PRNG) {
		prng_state = seed;
	}
}

void kame_generate(const char *template, char *out_password, size_t max_len, FILE *urand) {
	size_t out_len = 0;
	size_t t_idx = 0;

	if (out_password == NULL || max_len == 0) {
		return;
	}

	out_password[0] = '\0';

	while (template[t_idx] != '\0') {

		/* handle escape character */
		if (template[t_idx] == '\\') {
			t_idx++;
			/* handle trailing backslash */
			if (template[t_idx] == '\0') {
				break;
			}
			if (out_len < max_len - 1) {
				out_password[out_len++] = template[t_idx];
				out_password[out_len] = '\0';
			}
			t_idx++;
			continue;
		}

		char token = template[t_idx];

		if (token == 's' || token == 'S' || token == 'U') {
			uint64_t index = rand_range(num_syllables_avail, urand);
			const char *syllable = syllables[index];

			/* Inline copy & transform 1-2 ASCII characters */
			for (size_t i = 0; syllable[i] != '\0'; i++) {
				if (out_len >= max_len - 1) {
					break;
				}

				char c = syllable[i];
				if (token == 'U' || (token == 'S' && i == 0)) {
					c = (char)toupper((unsigned char)c);
				}

				out_password[out_len++] = c;
			}
			out_password[out_len] = '\0';

		} else if (token == 'n') {
			uint64_t index = rand_range(num_digits_avail, urand);
			if (out_len < max_len - 1) {
				out_password[out_len++] = digits[index];
				out_password[out_len] = '\0';
			}

		} else if (token == 'x') {
			uint64_t index = rand_range(num_specials_avail, urand);
			if (out_len < max_len - 1) {
				out_password[out_len++] = specials[index];
				out_password[out_len] = '\0';
			}

		} else {
			if (out_len < max_len - 1) {
				out_password[out_len++] = token;
				out_password[out_len] = '\0';
			}
		}

		t_idx++;
	}
}

void kame_print_combinations(const char *template) {
        double combinations = 1.0;
        int t_idx = 0;
        int active_tokens = 0;

        while (template[t_idx] != '\0') {

                /* handle escape character */
                if (template[t_idx] == '\\') {
                        t_idx++;
                        if (template[t_idx] == '\0')
                                break;
                        t_idx++;
                        continue;
                }

		char token = template[t_idx];

                if (token == 's' || token == 'S' || token == 'U') {
                        combinations *= num_syllables_avail;
                        active_tokens++;
                } else if (token == 'n') {
                        combinations *= num_digits_avail;
                        active_tokens++;
                } else if (token == 'x') {
                        combinations *= num_specials_avail;
                        active_tokens++;
                }
                t_idx++;
        }

        /* Only print stats if the template actually contains structural tokens */
        if (active_tokens > 0) {
		double entropy_bits = log2(combinations);
                fprintf(stderr, "Unique combinations for template \"%s\": %.0f (%.2e)\n",
                        template, combinations, combinations);
		fprintf(stderr, "Syllables: %d, Digits: %d, Special characters: %d\n",
				num_syllables_avail, num_digits_avail, num_specials_avail);
		fprintf(stderr, "Entropy: %.2f bits\n", entropy_bits);
        }
}

/* static buffers for dynamically loaded syllables */
static char *custom_file_buffer = NULL;
static char **custom_syllables = NULL;

void kame_free_syllables(void) {
	if (custom_syllables != NULL) {
		free(custom_syllables);
		custom_syllables = NULL;
	}
	if (custom_file_buffer != NULL) {
		free(custom_file_buffer);
		custom_file_buffer = NULL;
	}

	/* reset defaults */
	syllables = linear_b_syllables;
	num_syllables_avail = sizeof(linear_b_syllables) / sizeof(linear_b_syllables[0]);
}

void kame_map_syll_idtfr(const char *identifier) {
	kame_free_syllables();

	if (!strncmp(identifier, "linearB", 7)) {
		syllables = linear_b_syllables;
		num_syllables_avail = sizeof(linear_b_syllables) / sizeof(linear_b_syllables[0]);
	} else if (!strncmp(identifier, "alt", 3)) {
		syllables = alt_syllables;
		num_syllables_avail = sizeof(alt_syllables) / sizeof(alt_syllables[0]);
	} else if (!strncmp(identifier, "file:", 5)) {
		const char *filepath = identifier + 5;
		FILE *f = fopen(filepath, "r");
		if (f == NULL) {
			kame_errno = KAME_ERR_SYLLABLE_NA;
			return;
		}

		fseek(f, 0, SEEK_END);
		long fsize = ftell(f);
		fseek(f, 0, SEEK_SET);

		if (fsize <= 0) {
			fclose(f);
			kame_errno = KAME_ERR_SYLLABLE_NA;
			return;
		}

		custom_file_buffer = malloc(fsize + 1);
		if (custom_file_buffer == NULL) {
			fclose(f);
			kame_errno = KAME_ERR_SYLLABLE_NA;
			return;
		}

		size_t read_bytes = fread(custom_file_buffer, 1, fsize, f);
		fclose(f);
		custom_file_buffer[read_bytes] = '\0';

		/* count whitespace-delimited tokens */
		size_t count = 0;
		int in_token = 0;
		for (size_t i = 0; i < read_bytes; i++) {
			if (isspace((unsigned char)custom_file_buffer[i])) {
				in_token = 0;
			} else if (!in_token) {
				in_token = 1;
				count++;
			}
		}

		if (count == 0) {
			kame_free_syllables();
			kame_errno = KAME_ERR_SYLLABLE_NA;
			return;
		}

		custom_syllables = malloc(count * sizeof(char *));
		if (custom_syllables == NULL) {
			kame_free_syllables();
			kame_errno = KAME_ERR_SYLLABLE_NA;
			return;
		}

		/* extract pointers into custom_file_buffer in-place */
		size_t idx = 0;
		in_token = 0;
		for (size_t i = 0; i < read_bytes; i++) {
			if (isspace((unsigned char)custom_file_buffer[i])) {
				custom_file_buffer[i] = '\0';
				in_token = 0;
			} else if (!in_token) {
				custom_syllables[idx++] = &custom_file_buffer[i];
				in_token = 1;
			}
		}

		syllables = (const char **)custom_syllables;
		num_syllables_avail = (int)count;
	} else {
		kame_errno = KAME_ERR_SYLLABLE_NA;
	}
}
