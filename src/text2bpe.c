#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>

#include "bpe.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#define FLAG_IMPLEMENTATION
#include "flag.h"

#undef swap
#define swap(Type, x, y)         \
    do {                         \
        Type temp = (x);         \
        (x) = (y);               \
        (y) = temp;              \
    } while(0)

typedef struct {
    uint32_t l, r;
} PairKey;

typedef struct {
    PairKey key;
    size_t value;
} Freq;

typedef struct {
    Freq *items;
    size_t count;
    size_t capacity;
} Freqs;

void usage() {
    fprintf(stderr, "Usage: %s [OPTIONS]\n", flag_program_name());
    fprintf(stderr, "OPTIONS:\n");
    flag_print_options(stderr);
}

double get_secs(void) {
    struct timespec tp = {0};
    int ret = clock_gettime(CLOCK_MONOTONIC, &tp);
    assert(ret == 0);
    return (double)tp.tv_sec + (double)tp.tv_nsec*1e-9;
}

double begin_secs;
#if 0
    #define PROFILE_BEGIN() begin_secs = get_secs();
    #define PROFILE_END(label) printf("%s: %lfsecs\n", (label), get_secs() - begin_secs);
#else
    #define PROFILE_BEGIN(...)
    #define PROFILE_END(...)
#endif

Freq *collect_freqs(Tokens tokens_in) {
    Freq *freq = NULL;
    for (size_t i = 0; i < tokens_in.count - 1; ++i) {
        PairKey key = {
            .l = tokens_in.items[i], 
            .r = tokens_in.items[i + 1]
        };
        ptrdiff_t idx = hmgeti(freq, key);
        if (idx < 0) hmput(freq, key, 1);
        else freq[idx].value += 1;
    }
    return freq;
}

void report_progress(size_t iteration, Tokens tokens_in, Pairs pairs, double *profile_samples, size_t profile_samples_count) {
    double average_profile_samples = 0.0f;
    for (size_t i = 0; i < profile_samples_count; ++i) {
        average_profile_samples += profile_samples[i];
    }
    average_profile_samples /= profile_samples_count;
    printf("INFO: -- ITERATION %zu --\n", iteration);
    printf("    Tokens count: %zu\n", tokens_in.count);
    printf("    Pair Count:   %zu\n", pairs.count);
    printf("    Time:         %lfsecs (avg. of %zu iter.)\n", average_profile_samples, profile_samples_count);
}

int compare_freqs(const void *a, const void *b) {
    const Freq *af = a;
    const Freq *bf = b;
    return ((int) bf->value - (int) af->value);
}

bool dump_state(size_t iteration, const char *output_dir_path, Pairs pairs, Tokens tokens) {
    const char *output_file_path = temp_sprintf("%s/%zu.bpe", output_dir_path, iteration);
    if (!dump_pairs(output_file_path, pairs)) return false;
    printf("INFO: Generated pairs file %s\n", output_file_path);

    output_file_path = temp_sprintf("%s/%zu.tkn", output_dir_path, iteration);
    if (!dump_tokens(output_file_path, tokens)) return false;
    printf("INFO: Generated token file %s\n", output_file_path);

    return true;
}

int main(int argc, char **argv) {
    uint64_t *report_freq = flag_uint64("report-freq", 10, "Per how many iterations report the progress");
    uint64_t *dump_freq = flag_uint64("dump-freq", 10, "Per how many iterations dump state of the progress");
    uint64_t *term_freq = flag_uint64("term-freq", 1, "Termination pair frequency");
    uint64_t *max_iterations = flag_uint64("max-iterations", 0, "Maximum amount of iterations. 0 means no limit");
    uint64_t *threads_count = flag_uint64("threads-count", 16, "Threads count");
    bool *help = flag_bool("help", false, "Print this help");
    char **input_file = flag_str("input-file", NULL, "Input text file (MANDATORY)");
    char **output_dir = flag_str("output-dir", NULL, "Output directory (MANDATORY)");\

    if (!flag_parse(argc, argv)) {
        usage();
        flag_print_error(stderr);
        return 1;
    }

    if (*help) {
        usage();
        return 0;
    }

    if (*input_file == NULL) {
        usage();
        fprintf(stderr, "ERROR: no %s is provided\n", flag_name(input_file));
        return 1;
    }
    const char *input_file_path = *input_file;

    if (*output_dir == NULL) {
        usage();
        fprintf(stderr, "ERROR: no %s is provided\n", flag_name(output_dir));
    }

    const char *output_dir_path = *output_dir;

    if (*threads_count <= 0) *threads_count = 1;
    int output_dir_exists = file_exists(output_dir_path);
    if (output_dir_exists < 0) return 1;
    if (output_dir_exists) {
        fprintf(stderr, "ERROR: Directory %s already exists, delete it or rename it to not lose any data in it\n", output_dir_path);
        return 1;
    }

    if (!mkdir_if_not_exists(output_dir_path)) return 1;

    String_Builder sb = {0};
    if(!nob_read_entire_file(input_file_path, &sb)) return 1;
    sb_append_null(&sb);

    Freq *freq = NULL;
    Pairs pairs = {0};

    Tokens tokens_in = {0};
    Tokens tokens_out = {0};

    // 0   => { .l = 0, .r = ??? }
    // 1   => { .l = 1, .r = ??? }
    // ....
    // 69  => { .l = 69, .r = ??? }
    // ....
    // 255 => { .l = 255, .r = ??? }
    for (uint32_t i = 0; i < BPE_PRELUDE_SIZE; ++i) {
        da_append(&pairs, ((Pair) { .l = i }));
    }

    for (size_t i = 0; i < sb.count; ++i) {
        da_append(&tokens_in, (uint8_t)sb.items[i]);
    }

    freq = collect_freqs(tokens_in);

    double *profile_samples = malloc((*report_freq)*sizeof(*profile_samples));
    assert(profile_samples != NULL);

    size_t iteration = 0;
    for(;(*max_iterations == 0 || iteration < *max_iterations); ++iteration) {
        if (iteration > 0 && iteration % (*report_freq) == 0) report_progress(iteration, tokens_in, pairs, profile_samples, *report_freq);
        if (iteration % (*dump_freq)   == 0) if (!dump_state(iteration, output_dir_path, pairs, tokens_in)) return 1;
        double start_time = get_secs();

        ptrdiff_t max_index = 0;
        size_t n = hmlen(freq);
        if (n == 0) break;
        for (size_t i = 1; i < n; ++i) {
            if (freq[i].value > freq[max_index].value || (freq[i].value == freq[max_index].value && memcmp(&freq[i].key, &freq[max_index].key, sizeof(freq[i].key)) >0)) {
                max_index = i;
            }
        }

        if (freq[max_index].value <= (*term_freq)) break; // Compression is finished
        PairKey max_pair_key = freq[max_index].key;
        uint32_t max_token = pairs.count;
        Pair max_pair = {
            .l = max_pair_key.l,
            .r = max_pair_key.r,
            .freq = freq[max_index].value
        };

        da_append(&pairs, max_pair);

        tokens_out.count = 0;  // Clear the garbage before feeding to token out
        for (size_t i = 0; i < tokens_in.count; ) {
            if (i + 1 >= tokens_in.count) {
                da_append(&tokens_out, tokens_in.items[i]);
                i += 1;
            } else {
                ptrdiff_t place;
                PairKey key = {.l = tokens_in.items[i], .r = tokens_in.items[i + 1]};
                if (key.l == max_pair_key.l && key.r == max_pair_key.r) {
                    if (tokens_out.count > 0) {
                        PairKey pair_key = {
                            .l = tokens_out.items[tokens_out.count - 1],
                            .r = tokens_in.items[i]
                        };

                        place = hmgeti(freq, pair_key);
                        assert(place >= 0);
                        assert(freq[place].value > 0);
                        freq[place].value -= 1;

                        pair_key.r = max_token;
                        place = hmgeti(freq, pair_key);
                        if (place < 0) hmput(freq, pair_key, 1);
                        else freq[place].value += 1;
                    }

                    PairKey self_key = max_pair_key;
                    place = hmgeti(freq, self_key);
                    assert(place >= 0);
                    assert(freq[place].value > 0);
                    freq[place].value -= 1;

                    da_append(&tokens_out, max_token);
                    i += 2;

                    if (i < tokens_in.count) {
                        PairKey pair_key = {
                            .l = tokens_in.items[i-1],
                            .r = tokens_in.items[i]
                        };
                        place = hmgeti(freq, pair_key);
                        assert(place >= 0);
                        assert(freq[place].value > 0);
                        freq[place].value -= 1;

                        pair_key.l = max_token;
                        place = hmgeti(freq, pair_key);
                        if (place < 0) hmput(freq, pair_key, 1);
                        else freq[place].value += 1;
                    }
                } else {
                    da_append(&tokens_out, tokens_in.items[i]);
                    i += 1;
                }
            }
        }
        profile_samples[iteration%(*report_freq)] = get_secs() - start_time;
        swap(Tokens, tokens_in, tokens_out);
    }
    size_t remainder_iterations = iteration%(*report_freq);
    if (remainder_iterations == 0 && iteration > 0) remainder_iterations = *report_freq;
    if (remainder_iterations > 0) {
        report_progress(iteration, tokens_in, pairs, profile_samples, remainder_iterations);
    }

    // printf("\n\n\n");
    // printf("%zu\n", tokens_in.count);
    // printf("%zu\n", tokens_out.count);
    // qsort(sorted_freqs.items, sorted_freqs.count, sizeof(*sorted_freqs.items), compare_freqs);
    // for (size_t i = 0; i < 10; ++i) {
    //   printf("(%d, %d) => %zu\n", sorted_freqs.items[i].key.l, sorted_freqs.items[i].key.r, sorted_freqs.items[i].value);
    // }
    //
    if (!dump_state(iteration, output_dir_path, pairs, tokens_in)) return 1;
    return 0;
}
