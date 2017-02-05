#include "rfcommon.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <wiringPi.h>

#define RECORD_PIN                  (2)         /* pin number used by default */
#define RECORD_SIGNAL_ROUND         (5)         /* round duration (in usec) to multiples of this value */

uint32_t
    *buffer,
    *buffer_end,
    *buffer_top,
    config_pin = RECORD_PIN;

void print_usage() {
    printf("Usage: rfscanner [options]\n");
    printf("Options:\n");
    printf("  --help                        Print this help message\n");
    printf("  -p <number>, --pin <number>   The input (wiringPi) pin number (see http://pinout.xyz\n");
    printf("                                for details, default: %d)\n", RECORD_PIN);

    exit(1);
}

void parse_args(int argc, char **argv) {
    uint32_t i;

    for (i = 0; i < argc; i++) {
        if(!strcmp(argv[i], "--help")) {
            print_usage();
        }
        else if(!strcmp(argv[i], "--pin") || !strcmp(argv[i], "-p")) {
            config_pin = parse_args_uint32(i, argc, argv);

            if(config_pin >= 64){
                print_usage();
            }
        }
    }
}

int main(int argc, char **argv) {
    struct timespec last_change, now;
    long elapsed;
    int last = 0, first = 1;


    parse_args(argc, argv);

    if (wiringPiSetup() < 0) {
        printf("Unable to setup wiringPi: %s\n", strerror(errno));
        return 1;
    }

    pinMode(config_pin, INPUT);

    printf("Scanning...\n");
    for (;;) {
        /* read */
        int on = digitalRead(config_pin);

        /* time */
        clock_gettime(CLOCK_REALTIME, &now);
        elapsed = time_elapsed(last_change, now);
        elapsed /= 1000;

        /* round signal duration */
        elapsed /= RECORD_SIGNAL_ROUND;
        elapsed *= RECORD_SIGNAL_ROUND;

        if (on != last) {
            last = on;
            last_change = now;

            if (first && on) {
                first = 0;
                continue;
            }

            printf("%s for %lu usec\n", on ? "ON " : "OFF", (unsigned long)(elapsed));
        }
    }

    return 0;
}