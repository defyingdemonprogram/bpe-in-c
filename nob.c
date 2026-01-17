#define NOB_IMPLEMENTATION
#include "nob.h"

#define BUILD_DIR "build/"

bool build_tool(Nob_Cmd *cmd, const char *src_file_path, const char *target_file_path) {
    nob_cmd_append(cmd, "cc");
    nob_cmd_append(cmd, "-Wall", "-Wextra", "-ggdb");
    nob_cmd_append(cmd, "-o", target_file_path, src_file_path);
    if (!nob_cmd_run_sync_and_reset(cmd)) return false;
    return true;
}

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (!nob_mkdir_if_not_exists(BUILD_DIR)) return 1;

    Nob_Cmd cmd = {0};
    if (!build_tool(&cmd, "text2bpe.c", BUILD_DIR"text2bpe")) return 1;
    if (!build_tool(&cmd, "bpe2dot.c", BUILD_DIR"bpe2dot")) return 1;
    return 0;
}
