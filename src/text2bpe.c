#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#include "bpe.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#undef swap
#define swap(Type, x, y)         \
    do {                         \
        Type temp = (x);         \
        (x) = (y);               \
        (y) = temp;              \
    } while(0)

typedef struct {
    Pair key;
    size_t value;
} Freq;

typedef struct {
    Freq *items;
    size_t count;
    size_t capacity;
} Freqs;

void usage(const char *program_name) {
    fprintf(stderr, "Usage: %s <input.txt> <output.bpe>\n", program_name);
}

double get_secs(void) {
    struct timespec tp = {0};
    int ret = clock_gettime(CLOCK_MONOTONIC, &tp);
    assert(ret == 0);
    return (double)tp.tv_sec + (double)tp.tv_nsec*1e-9;
}

double begin_secs;
#if 1
    #define PROFILE_BEGIN() begin_secs = get_secs();
    #define PROFILE_END(label) printf("%s: %lfsecs\n", (label), get_secs() - begin_secs);
#else
    #define PROFILE_BEGIN(...)
    #define PROFILE_END(...)
#endif

#define ENABLE_THREADS
#define THREAD_COUNT 16
#define REPORT_FREQ 1
#define FREQ_COLLECTION_CHUNK_SIZE (4*1024)
Tokens tokens_in = {0};
size_t tokens_in_cursor = 0;
pthread_mutex_t tokens_in_cursor_mutex = {0};

Freq *freqs[THREAD_COUNT] = {0};
pthread_t threads[THREAD_COUNT] = {0};
sem_t collect_freqs_start = {0};
pthread_barrier_t collect_freqs_stop = {0};

void *freq_collector(void *arg) {
    size_t id = (size_t)(uintptr_t)(arg);

    while (true) {
        int ret = sem_wait(&collect_freqs_start);
        if (ret == -1) {
            fprintf(stderr, "ERROR: could not wait on semaphore: %s\n", strerror(errno));
            exit(1);
        }    

        hmfree(freqs[id]);

        while (true) {
            size_t begin, end;
            pthread_mutex_lock(&tokens_in_cursor_mutex);
            if (tokens_in_cursor + FREQ_COLLECTION_CHUNK_SIZE <= tokens_in.count) {
                begin = tokens_in_cursor;
                tokens_in_cursor += FREQ_COLLECTION_CHUNK_SIZE;
                end = tokens_in_cursor;
            } else {
                begin = tokens_in_cursor;
                tokens_in_cursor = tokens_in.count;
                end = tokens_in_cursor;
            }
            if (begin >= tokens_in.count) {
                pthread_mutex_unlock(&tokens_in_cursor_mutex);
                break;
            }
            pthread_mutex_unlock(&tokens_in_cursor_mutex);

            for (size_t i = begin; i < end; ++i) {
                if (i + 1 >= tokens_in.count) break;
                Pair pair = {
                    .l = tokens_in.items[i],
                    .r = tokens_in.items[i + 1]
                };
                ptrdiff_t idx = hmgeti(freqs[id], pair);
                if (idx < 0) hmput(freqs[id], pair, 1);
                else freqs[id][idx].value += 1;
            }
        }

        pthread_barrier_wait(&collect_freqs_stop);
    }
    UNREACHABLE("freq_collector");
}

void report_progress(size_t iteration, Tokens tokens_in, Pairs pairs) {
    printf("INFO: iteration %zu\n", iteration);
    printf("    Text tokens count: %zu\n", tokens_in.count);
    printf("    BPE table size: %zu\n", pairs.count);
}

int compare_freqs(const void *a, const void *b) {
    const Freq *af = a;
    const Freq *bf = b;
    return ((int) bf->value - (int) af->value);
}

int main(int argc, char **argv) {
    const char *program_name = shift(argv, argc);

    if (argc <= 0) {
        usage(program_name);
        fprintf(stderr, "ERROR: no input text file is provided\n");
        return 1;
    }
    const char *input_file_path = shift(argv, argc);

    if (argc <= 0) {
        usage(program_name);
        fprintf(stderr, "ERROR: no output is provided\n");
        return 1;
    }
    const char *output_file_path = shift(argv, argc);

    String_Builder sb = {0};
    if(!nob_read_entire_file(input_file_path, &sb)) return 1;
    sb_append_null(&sb);

    Freq *merged_freq = NULL;
    Pairs pairs = {0};

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

#ifdef ENABLE_THREADS
    int ret = pthread_mutex_init(&tokens_in_cursor_mutex, NULL);
    if (ret != 0) {
        fprintf(stderr, "ERROR: could not initialize tokens_in_cursor_mutex: %s\n", strerror(ret));
        return 1;
    }

    ret = sem_init(&collect_freqs_start, 0, 0);
    if (ret != 0) {
        fprintf(stderr, "ERROR: could not initialize collect_freqs_start: %s\n", strerror(ret));
        return 1;
    }

    ret = pthread_barrier_init(&collect_freqs_stop, NULL, THREAD_COUNT + 1);
    if (ret != 0) {
        fprintf(stderr, "ERROR: could not initialize collect_freqs_stop: %s\n", strerror(ret));
        return 1;
    }

    for (size_t id = 0; id < THREAD_COUNT; ++id) {
        ret = pthread_create(&threads[id], NULL, freq_collector, (void*)(uintptr_t)id);
        if (ret != 0) {
            fprintf(stderr, "ERROR: could not create thread: %s\n", strerror(ret));
            return 1;
        }
    }
#endif // ENABLE_THREADS

    size_t iteration = 0;
    for(;; ++iteration) {
        if (iteration % REPORT_FREQ == 0) report_progress(iteration, tokens_in, pairs);
        PROFILE_BEGIN();
        hmfree(merged_freq);
#ifndef ENABLE_THREADS
        for (size_t i = 0; i < tokens_in.count - 1; ++i) {
            Pair pair = {
                .l = tokens_in.items[i], 
                .r = tokens_in.items[i + 1]
            };
            ptrdiff_t idx = hmgeti(merged_freq, pair);
            if (idx < 0) hmput(merged_freq, pair, 1);
            else merged_freq[idx].value += 1;
        }
#else
        pthread_mutex_lock(&tokens_in_cursor_mutex);
        tokens_in_cursor = 0;
        tokens_in_cursor = 0;
        pthread_mutex_unlock(&tokens_in_cursor_mutex);

        for (size_t i = 0; i < THREAD_COUNT; ++i) sem_post(&collect_freqs_start);
        pthread_barrier_wait(&collect_freqs_stop);

        for (size_t id = 0; id < THREAD_COUNT; ++id ) {
            size_t n = hmlen(freqs[id]);
            for (size_t i = 0; i < n; ++i) {
                Pair key = freqs[id][i].key;
                ptrdiff_t idx = hmgeti(merged_freq, key);
                if (idx < 0) hmputs(merged_freq, freqs[id][i]);
                else merged_freq[idx].value += freqs[id][i].value;
            }
        }
#endif // ENABLE_THREADS
        PROFILE_END("Collecting stats");

        //Freqs sorted_freqs = {0};

        //for (ptrdiff_t i = 0; i < hmlen(merged_freq); ++i) {
        //    da_append(&sorted_freqs, freq[i]);
        // }

        PROFILE_BEGIN();
        ptrdiff_t max_index = 0;
        size_t n = hmlen(merged_freq);
        if (n == 0) break;
        for (size_t i = 1; i < n; ++i) {
            if (merged_freq[i].value > merged_freq[max_index].value) {
                max_index = i;
            }
        }
        PROFILE_END("Finding most frequent pairs");

        if (merged_freq[max_index].value <= 1) break; // Compression is finished

        // printf("(%d, %d) => %zu\n", freq[max_index].key.l, freq[max_index].key.r, freq[max_index].value);

        da_append(&pairs, merged_freq[max_index].key);

        PROFILE_BEGIN();
        tokens_out.count = 0;  // Clear the garbage before feeding to token out
        for (size_t i = 0; i < tokens_in.count; ) {
            if (i + 1 >= tokens_in.count) {
                da_append(&tokens_out, tokens_in.items[i]);
                i += 1;
            } else {
                Pair pair = {.l = tokens_in.items[i], .r = tokens_in.items[i + 1]};
                if (memcmp(&pair, &merged_freq[max_index].key, sizeof(pair)) == 0) {
                    da_append(&tokens_out, pairs.count - 1);
                    i += 2;
                } else {
                    da_append(&tokens_out, tokens_in.items[i]);
                    i += 1;
                }
            }
        }
        PROFILE_END("Replacing the frequent pair");
        swap(Tokens, tokens_in, tokens_out);
    }
    report_progress(iteration, tokens_in, pairs);

    // printf("\n\n\n");
    // printf("%zu\n", tokens_in.count);
    // printf("%zu\n", tokens_out.count);
    // qsort(sorted_freqs.items, sorted_freqs.count, sizeof(*sorted_freqs.items), compare_freqs);
    // for (size_t i = 0; i < 10; ++i) {
    //   printf("(%d, %d) => %zu\n", sorted_freqs.items[i].key.l, sorted_freqs.items[i].key.r, sorted_freqs.items[i].value);
    // }
    //
    if (!dump_pairs(output_file_path, pairs)) return 1;
    printf("INFO: Generated %s file for BPE.\n", output_file_path);
    return 0;
}
