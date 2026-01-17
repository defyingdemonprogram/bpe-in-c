#define NOB_IMPLEMENTATION
#include "nob.h"

#define BUILD_DIR "build/"

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (!nob_mkdir_if_not_exists(BUILD_DIR)) return 1;

    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "cc");
    nob_cmd_append(&cmd, "-Wall", "-Wextra", "-ggdb");
    nob_cmd_append(&cmd, "-o", BUILD_DIR"bpe", "bpe.c");
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;

    nob_cmd_append(&cmd, "cc", "-Wall", "-Wextra", "-ggdb", "-o", BUILD_DIR"bpe2dot", "bpe2dot.c");
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;

    // Run the executable
    nob_cmd_append(&cmd, "./build/bpe");
    // if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    return 0;
}
