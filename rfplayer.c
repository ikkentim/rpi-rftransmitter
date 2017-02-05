#include "rfcommon.h"
#include <wiringPi.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define PLAY_PIN            (0)     /* default pin */
#define PLAY_TIME           (500)  /* default play time in ms */

char *config_filename;
uint32_t config_pin = PLAY_PIN;
uint32_t config_playtime = PLAY_TIME;

/* blocking call for defined microseconds */
void block_usec(uint32_t usec) {
    struct timespec before, now;
    long elapsed = 0;

    clock_gettime(CLOCK_REALTIME, &before);
    while (elapsed < usec) {
        clock_gettime(CLOCK_REALTIME, &now);
        elapsed = time_elapsed(before, now);
        elapsed /= 1000;
    }
}

void print_usage() {
    printf("Usage: rfplayer <filename> [options]\n");
    printf("Options:\n");
    printf("  --help                        Print this help message\n");
    printf("  -p <number>, --pin <number>   The input (wiringPi) pin number (see http://pinout.xyz\n");
    printf("                                for details, default: %d)\n", PLAY_PIN);
    printf("  -t <ms>, --playtime <ms>      The total playtime in milliseconds (default: %d)\n", PLAY_TIME);

    exit(1);
}

void parse_args(int argc, char **argv) {
    uint32_t i;

    if (argc < 2 || !strcmp(argv[0], "--help")) {
        print_usage();
    }

    config_filename = argv[1];

    for (i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "--help")) {
            print_usage();
        } else if (!strcmp(argv[i], "--pin") || !strcmp(argv[i], "-p")) {
            config_pin = parse_args_uint32(i, argc, argv);

            if (config_pin >= 64) {
                print_usage();
            }
        } else if (!strcmp(argv[i], "--playtime") || !strcmp(argv[i], "-t")) {
            config_playtime = parse_args_uint32(i, argc, argv);

            if (config_pin >= 64) {
                print_usage();
            }
        }
    }
}

int main(int argc, char **argv) {
    parse_args(argc, argv);

    FILE *pFile = fopen(config_filename, "rb");

    if (!pFile) {
        printf("Could not open file!\n");
        return 1;
    }

    fseek(pFile, 0L, SEEK_END);
    uint32_t sz = ftell(pFile);
    fseek(pFile, 0L, SEEK_SET);

    uint32_t *buffer;

    buffer = (uint32_t *) malloc(sz);

    if (!buffer) {
        printf("Could not allocate memory!\n");
        fclose(pFile);
        return 1;
    }
    fread(buffer, sz, 1, pFile);

    fclose(pFile);

    if (wiringPiSetup() < 0) {
        printf("Unable to setup wiringPi: %s\n", strerror(errno));
        return 1;
    }

    pinMode(config_pin, OUTPUT);

    uint32_t pairs = sz / sizeof(uint32_t) / 2;
    uint32_t i, on, off;
    struct timespec before, now;
    clock_gettime(CLOCK_REALTIME, &before);
    now = before;

    while(time_elapsed(before, now) / 1000000 < config_playtime) {
        for (i = 0; i < pairs; i++) {
            on = buffer[i * 2];
            off = buffer[i * 2 + 1];

            digitalWrite(config_pin, HIGH);
            block_usec(on);
            digitalWrite(config_pin, LOW);
            block_usec(off);
        }

        clock_gettime(CLOCK_REALTIME, &now);
    }

    return 0;
}