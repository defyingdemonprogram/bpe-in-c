#ifndef BPE_H_
#define BPE_H_

#include <stdint.h>
#include <stdbool.h>

#define NOB_STRIP_PREFIX
#include "nob.h"

#define BPE_PRELUDE_SIZE 256

typedef struct {
    uint32_t *items;
    size_t count;
    size_t capacity;
} Tokens;

typedef struct {
    uint32_t l, r;
    uint64_t freq;
} Pair;

typedef struct {
    Pair *items;
    size_t count;
    size_t capacity;
} Pairs;

bool dump_pairs(const char *file_path, Pairs pairs);
bool dump_tokens(const char *file_path, Tokens tokens);
bool load_pairs(const char *file_path, Pairs *pairs, String_Builder *sb);
bool load_tokens(const char *file_path, Tokens *tokens, String_Builder *sb);
void render_token(Pairs pairs, uint32_t token, String_Builder *sb);
void c_strlit_escape_bytes(const char *bytes, size_t bytes_size, String_Builder *sb_out);

#endif // BPE_H_
