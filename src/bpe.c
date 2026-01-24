#include "bpe.h"

void render_token(Pairs pairs, uint32_t token, String_Builder *sb) {
    if (token == pairs.items[token].l) {
        da_append(sb, (char)token);
        return;
    }

    render_token(pairs, pairs.items[token].l, sb);
    render_token(pairs, pairs.items[token].r, sb);
}

bool dump_tokens(const char *file_path, Tokens tokens) {
    return write_entire_file(file_path, tokens.items, tokens.count*sizeof(*tokens.items));
}

bool dump_pairs(const char *file_path, Pairs pairs) {
    return write_entire_file(file_path, pairs.items, pairs.count*sizeof(*pairs.items));
}

bool load_pairs(const char *file_path, Pairs *pairs, String_Builder *sb) {
    sb->count = 0;
    if (!read_entire_file(file_path, sb)) return false;
    if (sb->count % sizeof(*pairs->items) != 0) {
        nob_log(ERROR, "%s: file size in bytes (%zu) must be divisible by %zu\n", file_path, sb->count, sizeof(*pairs->items));
    }

    Pair *items = (void*)sb->items;
    size_t items_count = sb->count / sizeof(*pairs->items);

    if (items_count < BPE_PRELUDE_SIZE) {
        nob_log(ERROR, "%s: pair count %zu is too small. It must be at least %u", file_path, items_count, BPE_PRELUDE_SIZE);
        return false;
    }

    for (uint32_t i = 0; i < BPE_PRELUDE_SIZE; ++i) {
        if (items[i].l != i) {
            nob_log(ERROR, "%s: pair %u: Left subtoken is equal to %u instead of itself", file_path, i, items[i].l);
            return false;
        }
        if (items[i].r != 0) {
            nob_log(ERROR, "%s: pair %u: Right subtoken is equal to %u instead of 0", file_path, i, items[i].r);
            return false;
        }
        da_append(pairs, items[i]);
    }
    for (uint32_t i = BPE_PRELUDE_SIZE; i < items_count; ++i) {
        if (items[i].l >= i) {
            nob_log(ERROR, "%s: pair %u: Left subtoken is %u >= %u", file_path, i, items[i].l, i);
            return false;
        }
        if (items[i].r >= i) {
            nob_log(ERROR, "%s: pair %u: Right subtoken is %u >= %u", file_path, i, items[i].r, i);
            return false;
        }
        da_append(pairs, items[i]);
    }
    return true;
}

bool load_tokens(const char *file_path, Tokens *tokens, String_Builder *sb) {
    sb->count = 0;
    if (!read_entire_file(file_path, sb)) return false;
    if (sb->count % sizeof(*tokens->items) != 0) {
        nob_log(ERROR, "%s: file size in bytes (%zu) must be divisible by %zu", file_path, sb->count, sizeof(*tokens->items));
        return false;
    }
    uint32_t *items = (void*)sb->items;
    size_t items_count = sb->count / sizeof(*tokens->items);
    for (size_t i = 0; i < items_count; ++i) {
        da_append(tokens, items[i]);
    }
    return true;
}

#define NOB_IMPLEMENTATION
#include "nob.h"
