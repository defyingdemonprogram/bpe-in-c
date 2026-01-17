#include <stdio.h>
#include <string.h>

#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"
#include "bpe.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#undef swap
#define swap(Type, x, y)        \
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

typedef struct {
    uint32_t *items;
    size_t count;
    size_t capacity;
} Tokens;


bool dump_pairs(const char *file_path, Pairs pairs) {
    return write_entire_file(file_path, pairs.items, pairs.count*sizeof(*pairs.items));
}

int compare_freqs(const void *a, const void *b) {
    const Freq *af = a;
    const Freq *bf = b;
    return ((int) bf->value - (int) af->value);
}

void render_tokens(Pairs pairs, Tokens tokens) {
    for (size_t i = 0; i < tokens.count; ++i) {
        uint32_t token = tokens.items[i];
        assert(token < pairs.count);
        if (pairs.items[token].l == token) {
            printf("%c", token);
        } else {
            printf("[%u]", token);
        }
    }
    printf("\n-----------------------------\n");
}

int main() {
    const char *text = "The original BPE algorithm operates by iteratively replacing the most common contiguous sequences of characters in a target text with unused 'placeholder' bytes. The iteration ends when no sequences can be found, leaving the target text effectively compressed. Decompression can be performed by reversing this process, querying known placeholder terms against their corresponding denoted sequence, using a lookup table. In the original paper, this lookup table is encoded and stored alongside the compressed text.";

    int text_size = strlen(text);

    Freq *freq = NULL;
    Pairs pairs = {0};

    Tokens tokens_in = {0};
    Tokens tokens_out = {0};
    for (int i = 0; i < text_size; ++i) {
        da_append(&tokens_in, text[i]);
    }

    // 0   => { .l = 0, .r = ??? }
    // 1   => {" .l = 1, .r = ??? }
    // ....
    // 69  => { .l = 69, .r = ??? }
    // ....
    // 255 => { .l = 255, .r = ??? }
    for (uint32_t i = 0; i < 256; ++i) {
        da_append(&pairs, ((Pair) { .l = i }));
    }

    for(;;) {
        // printf("%zu: \n", tokens_in.count);
        render_tokens(pairs, tokens_in);
        hmfree(freq);
        for (size_t i = 0; i < tokens_in.count - 1; ++i) {
            Pair pair = {
                .l = tokens_in.items[i], 
                .r = tokens_in.items[i + 1]
            };
            ptrdiff_t i = hmgeti(freq, pair);
            if (i < 0) hmput(freq, pair, 1);
            else freq[i].value += 1;
        }

        Freqs sorted_freqs = {0};

        for (ptrdiff_t i = 0; i < hmlen(freq); ++i) {
            da_append(&sorted_freqs, freq[i]);
        }

        ptrdiff_t max_index = 0;
        for (ptrdiff_t i = 1; i < hmlen(freq); ++i) {
            if (freq[i].value > freq[max_index].value) {
                max_index = i;
            }
        }

        if (freq[max_index].value <= 1) break; // Compression is finished

        // printf("(%d, %d) => %zu\n", freq[max_index].key.l, freq[max_index].key.r, freq[max_index].value);

        da_append(&pairs, freq[max_index].key);

        tokens_out.count = 0;  // Clear the garbage before feeding to token out
        for (size_t i = 0; i < tokens_in.count - 1;) {
            if (i + 1 >= tokens_in.count) {
                da_append(&tokens_out, tokens_in.items[i]);
            } else {
                Pair pair = {.l = tokens_in.items[i], .r = tokens_in.items[i + 1]};
                if (memcmp(&pair, &freq[max_index].key, sizeof(pair)) == 0) {
                    da_append(&tokens_out, pairs.count - 1);
                    i += 2;
                } else {
                    da_append(&tokens_out, tokens_in.items[i]);
                    i += 1;
                }
            }
        }
        swap(Tokens, tokens_in, tokens_out);
    }
    printf("Generated %zu tokens\n", pairs.count);

    // render_tokens(pairs, tokens_in);
    // printf("\n\n\n");
    // render_tokens(pairs, tokens_out);
    // printf("%zu\n", tokens_in.count);
    // printf("%zu\n", tokens_out.count);
    // qsort(sorted_freqs.items, sorted_freqs.count, sizeof(*sorted_freqs.items), compare_freqs);
    // for (size_t i = 0; i < 10; ++i) {
    //   printf("(%d, %d) => %zu\n", sorted_freqs.items[i].key.l, sorted_freqs.items[i].key.r, sorted_freqs.items[i].value);
    // }
    //
    if (!dump_pairs("pairs.bin", pairs)) return 1;
    return 0;
}
