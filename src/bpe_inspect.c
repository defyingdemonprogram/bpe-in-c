#include <stdio.h>

#include "bpe.h"

void usage(const char *program_name) {
    fprintf(stderr, "Usage: %s <input.bpe>\n", program_name);
}

int main(int argc, char **argv) {
    const char *program_name = shift(argv, argc);
    const char *input_file_path = NULL;
    bool no_ids = false;

    while (argc > 0) {
        const char *arg = shift(argv, argc);
        if (strcmp(arg, "--no-ids") == 0) {
            no_ids = true;
        } else {
            if (input_file_path != NULL) {
                fprintf(stderr, "ERROR: %s supports inspecting only single file\n", program_name);
                return 1;
            }
            input_file_path = arg;
        }
    }
    if (input_file_path == NULL) {
        usage(program_name);
        fprintf(stderr, "ERROR: no input is provided\n");
        return 1;
    }

    Pairs pairs = {0};
    String_Builder sb_raw = {0};
    String_Builder sb_out = {0};

    if (!load_pairs(input_file_path, &pairs, &sb_raw)) return 1;

    for (uint32_t token = 0; token < pairs.count; ++token) {
        sb_raw.count = 0;
        sb_out.count = 0;

        render_token(pairs, token, &sb_raw);

        if (!no_ids) {
            sb_appendf(&sb_out, "%u => (%zu) ", token, pairs.items[token].freq);
        }

        sb_append_cstr(&sb_out, "\"");
        c_strlit_escape_bytes(sb_raw.items, sb_raw.count, &sb_out);
        sb_append_cstr(&sb_out, "\"\n");
        sb_append_null(&sb_out);

        printf("%s", sb_out.items);
    }

    return 0;
}
