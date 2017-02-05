#ifndef RFCOMMON_H
#define RFCOMMON_H

#include <inttypes.h>
#include <time.h>

long time_elapsed(struct timespec before, struct timespec after);

uint32_t parse_args_uint32(uint32_t i, int argc, char **argv);

char *parse_args_string(uint32_t i, int argc, char **argv);

#endif