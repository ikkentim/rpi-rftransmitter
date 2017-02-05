#include "rfcommon.h"
#include <stdlib.h>

long time_elapsed(struct timespec before, struct timespec after) {
    return (after.tv_sec - before.tv_sec) * 1000000000L +
           (after.tv_nsec - before.tv_nsec);
}

/* defined in specific .c */
void print_usage();

uint32_t parse_args_uint32(uint32_t i, int argc, char **argv) {
    if(i + 1 >= argc) {
        print_usage();
    }

    return (uint32_t)atoi(argv[i + 1]);
}

char *parse_args_string(uint32_t i, int argc, char **argv) {
    if(i + 1 >= argc) {
        print_usage();
    }

    return argv[i + 1];
}
