#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include "lib.h"

/* Hidden state for the engine */
static uint64_t xorshift_state;
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

const char digits[] = "0123456789";
const char specials[] = "!@#$%^&*()-_=+[]{}";

int num_syllables_avail = sizeof(linear_b_syllables) / sizeof(linear_b_syllables[0]);
int num_digits_avail = sizeof(digits) - 1;
int num_specials_avail = sizeof(specials) - 1;

/* Xorshift64 implementation */
static uint64_t xorshift64(void) {
        uint64_t x = xorshift_state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        return xorshift_state = x;
}

static uint64_t get_entropy64(void) {
	/* DEFAULT PATH: Read from CSPRNG pool unless Xorshift is enabled */
	if (!(engine_flags & KAME_XORSHIFT)) {
		uint64_t fresh_entropy = 0;
		FILE *urand = fopen("/dev/urandom", "rb");

		if (urand == NULL) {
			kame_errno = KAME_ERR_URANDOM_OPEN;
			return 0; /* Safe fallback */
		}

		if (fread(&fresh_entropy, sizeof(fresh_entropy), 1, urand) != 1) {
			kame_errno = KAME_ERR_URANDOM_READ;
			fclose(urand);
			return 0; /* Safe fallback */
		}

		fclose(urand);
		return fresh_entropy;
	}

	/* OPT-IN PATH: Deterministic Xorshift PRNG */
	return xorshift64();
}

/* Lemire's fastrange algorithm: fast uniform range reduction
 * Lemire, D. (2018). Fast Random Integer Generation in an Interval.
 * arXiv:1805.10941 */
static uint64_t rand_range(uint64_t range) {
	if (range <= 1) {
		return 0;
	}

	uint64_t x = get_entropy64();
	if (kame_errno != KAME_SUCCESS) {
		return 0;
	}

	unsigned __int128 multi = (unsigned __int128)x * range;
	uint64_t leftover = (uint64_t)multi;

	/* Lemire rejection sampling branch for exact unbiased reduction */
	if (leftover < range) {
		uint64_t threshold = -range % range; /* Equivalent to (2^64) % range */
		while (leftover < threshold) {
			x = get_entropy64();
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

	if (engine_flags & KAME_XORSHIFT) {
		if (seed == 0) {
			xorshift_state = 0xFFFFFFFFFFFFFFFFULL;
		} else {
			xorshift_state = seed;
		}
	}
}

void kame_generate(const char *template, char *out_password, size_t max_len) {
        int t_idx = 0;
        out_password[0] = '\0';

        while (template[t_idx] != '\0') {

                /* handle escape character */
                if (template[t_idx] == '\\') {
                        t_idx++;
                        /* handle trailing backslash */
                        if (template[t_idx] == '\0')
                                break;
                        size_t len = strlen(out_password);
                        if (len < max_len - 1) {
                                out_password[len] = template[t_idx];
                                out_password[len + 1] = '\0';
                        }
                        t_idx++;
                        continue;
                }

                char token = template[t_idx];

                if (token == 's' || token == 'S' || token == 'U') {
                        uint64_t index = rand_range(num_syllables_avail);
                        const char *syllable = linear_b_syllables[index];
                        char temp_syl[16];

                        strncpy(temp_syl, syllable, sizeof(temp_syl) - 1);
                        temp_syl[sizeof(temp_syl) - 1] = '\0';

                        if (token == 'S' && temp_syl[0] != '\0') {
                                temp_syl[0] = (char)toupper((unsigned char)temp_syl[0]);
                        } else if (token == 'U') {
                                /* Convert the entire syllable string to uppercase */
                                for (int i = 0; temp_syl[i] != '\0'; i++) {
                                        temp_syl[i] = (char)toupper((unsigned char)temp_syl[i]);
                                }
                        }

                        strncat(out_password, temp_syl, max_len - strlen(out_password) - 1);

                } else if (token == 'n') {
                        uint64_t index = rand_range(num_digits_avail);
                        size_t len = strlen(out_password);
                        if (len < max_len - 1) {
                                out_password[len] = digits[index];
                                out_password[len + 1] = '\0';
                        }

                } else if (token == 'x') {
                        uint64_t index = rand_range(num_specials_avail);
                        size_t len = strlen(out_password);
                        if (len < max_len - 1) {
                                out_password[len] = specials[index];
                                out_password[len + 1] = '\0';
                        }
                } else {
                        size_t len = strlen(out_password);
                        if (len < max_len - 1) {
                                out_password[len] = token;
                                out_password[len + 1] = '\0';
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
                fprintf(stderr, "[Kamedati Entropy Engine] Unique combinations for template \"%s\": %.0f (%.2e)\n",
                        template, combinations, combinations);
		fprintf(stderr, "Entropy: %.2f bits\n", entropy_bits);
        }
}
