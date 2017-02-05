#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <inttypes.h>
//#include <sys/mman.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <wiringPi.h>

#define RECORD_SIGNAL_BUFFER        (500)       /* size of recording buffer */
#define RECORD_SIGNAL_ROUND         (5)         /* round duration (in usec) to multiples of this value */
#define RECORD_TIMEOUT_MIN_ENTRIES  (10)        /* minimum entries in buffer before recording timeout */
#define RECORD_TIMEOUT_TIME         (1000000)   /* recording timeout (in usec) 1 sec */
#define RECORD_SAMPLES_COUNT        (5)         /* number of samples collected before assuming it's correct */
#define RECORD_SAMPLES_FAILS        (2)
#define PATTERN_MATCH_SETS_HIGH     (5)         /* number of sets to look ahead when finding gap between signal blobs */
#define PATTERN_MATCH_TOLERANCE     (75)        /* tolerance in usec when comparing signal durations */
#define PATTERN_MIN_APPEARANCES     (2)         /* minimum number of appearances of a pattern in the buffer for it to be accepted */

#define ABS(a) ((a) < 0 ? -(a) : (a)) /* get absolute value */

uint32_t
    *buffer,
    *buffer_end,
    *buffer_top;

long time_elapsed(struct timespec before, struct timespec after) {
    return (after.tv_sec - before.tv_sec) * 1000000000L +
           (after.tv_nsec - before.tv_nsec);
}

uint8_t pattern_near_match(uint32_t a, uint32_t b) {
    return ABS((int32_t) a - (int32_t) b) < PATTERN_MATCH_TOLERANCE;
}

uint8_t pattern_near_match_pair(uint32_t *a, uint32_t *b) {
    return pattern_near_match(a[0], b[0]) && pattern_near_match(a[1], b[1]);
}

uint8_t pattern_near_match_blob(uint32_t *a, uint32_t a_len, uint32_t *b, uint32_t b_len) {
    uint32_t i;

    if(a_len != b_len) {
        return 0;
    }

    for (i = 0; i < a_len; i++) {
        if(!pattern_near_match(a[i], b[i])) {
            return 0;
        }
    }

    return 1;
}

uint8_t pattern_match_buffer(uint32_t **pattern) {
    uint32_t
        i,
        *current_search,
        *candidate,
        **starts_buffer,
        **starts_top,
        **starts_current,
        **starts_check,
        blob_len,
        blob_best_len,
        blob_appearances,
        blob_best_appearances,
        **blob_best_start;

    uint8_t ok;

    /* alloc starts buffer */
    starts_buffer = (uint32_t **) malloc(sizeof(uint32_t *) * RECORD_SIGNAL_BUFFER);

    /*
     * Start searching for start point. Assuming there is a significantly long low signal before same blob repeats
     * again. Start halfway trough the buffer to avoid the signal being send late by the user. Stop looking before
     * lookahead space is no longer available.
     */
    for (current_search = buffer + (buffer_top - buffer) / 4 * 2;
         current_search + 1 < buffer_top - PATTERN_MATCH_SETS_HIGH * 2;
         current_search += 2) {

        /*
         * Look ahead in buffer. If there are any high signal with a longer or equally long off signal in front of it,
         * this value is not the starting point.
         */
        ok = 1;
        for (i = 1; i < PATTERN_MATCH_SETS_HIGH; i++) {
            uint32_t c = current_search[i * 2 + 1];
            if (c > current_search[1] || pattern_near_match(current_search[1], c)) {
                ok = 0;
                break;
            }
        }

        if (!ok) {
            continue;
        }

        /* Find all the same values in the buffer. */
        starts_top = starts_buffer;
        for (candidate = buffer; candidate + 1 < buffer_top; candidate += 2) {
            /* Check if candidate is similar to current search value. */
            if (!pattern_near_match_pair(current_search, candidate)) {
                continue;
            }

            *starts_top = candidate;
            starts_top++;
        }

        /* Find most common blob length. */
        blob_best_len = 0;
        blob_best_appearances = 0;
        for (starts_current = starts_buffer; starts_current < starts_top - 1; starts_current++) {
            blob_len = starts_current[1] - starts_current[0];

            if (blob_len == blob_best_len) {
                continue;
            }

            blob_appearances = 1;
            for(starts_check = starts_current + 1; starts_check < starts_top - 1; starts_check++) {
                if(blob_len == starts_check[1] - starts_check[0]) {
                    blob_appearances++;
                }
            }

            if(blob_appearances > blob_best_appearances) {
                blob_best_appearances = blob_appearances;
                blob_best_len = blob_len;
            }
        }

        /* Blobs should appear at least a few times. */
        if(blob_best_appearances < PATTERN_MIN_APPEARANCES) {
            free(starts_buffer);
            return 0;
        }

        /* Find the blob which matches most other blobs of the best length */
        blob_best_start = 0;
        blob_best_appearances = 0;
        for (starts_current = starts_buffer; starts_current < starts_top - 1; starts_current++) {
            if(starts_current[1] - starts_current[0] != blob_best_len) {
                continue;
            }

            if(!blob_best_start) {
                blob_best_start = starts_current;
                continue;
            }

            /* Count appearances of blob in buffer*/
            blob_appearances = 0;
            for(starts_check = starts_buffer; starts_check < starts_top - 1; starts_check++) {
                if (pattern_near_match_blob(*starts_current, blob_best_len,
                                            *starts_check, starts_check[1] - starts_check[0])) {
                    blob_appearances++;
                }
            }

            if(blob_appearances > blob_best_appearances) {
                blob_best_appearances = blob_appearances;
                blob_best_start = starts_current;
            }
        }

        /* If no blob was found, clean up and return. */
        if(!blob_best_start) {
            free(starts_buffer);
            return 0;
        }

        /* Copy the pattern */
        if(pattern) {
            *pattern = (uint32_t *) malloc(sizeof(uint32_t) * blob_best_len);

            for (i = 0; i < blob_best_len; i++) {
                /*
                 * The start we have been looking for is actually the last bit (because it was easy to find). Move the
                 * first pair to the end while copying.
                 */
                (*pattern)[i] = (*blob_best_start)[(i + 2) % blob_best_len];
            }
        }

        free(starts_buffer);
        return blob_best_len;
    }

    free(starts_buffer);
    return 0;
}

void printUsage() {

}

int main(int argc, char **argv) {
    printf("Setting up GPIO...\n");
    if (wiringPiSetup() < 0) {
        printf("Unable to setup wiringPi: %s\n", strerror(errno));
        return 1;
    }

    pinMode(2, INPUT);

    printf("Started...\n");
    struct timespec last_change, now;
    long elapsed;
    int last = 0, waiting = 0;
    uint32_t *samples[RECORD_SAMPLES_COUNT];
    uint32_t sample_idx = 0;
    uint32_t sample_length = 0;
    uint32_t sample_fails = 0;
    uint32_t i;

    assert(RECORD_SIGNAL_BUFFER % 2 == 0);

    /* alloc pattern search buffer */
    buffer = (uint32_t *) malloc(sizeof(uint32_t) * RECORD_SIGNAL_BUFFER);
    buffer_end = buffer + RECORD_SIGNAL_BUFFER;
    buffer_top = buffer;

    printf("Record start...\n");
    while (sample_idx != RECORD_SAMPLES_COUNT) {
        /* read */
        int on = digitalRead(2);

        /* time */
        clock_gettime(CLOCK_REALTIME, &now);
        elapsed = time_elapsed(last_change, now);
        elapsed /= 1000;

        /* round signal duration */
        elapsed /= RECORD_SIGNAL_ROUND;
        elapsed *= RECORD_SIGNAL_ROUND;

        /* On change, add time to buffer */
        if (on != last) {
            last = on;
            last_change = now;

            /* Skip when waiting for start or when it's the first entry and the signal just turned on */
            if ((waiting && !on) || (buffer_top == buffer && on)) {
                waiting = 0;
                continue;
            }

            /* store value to buffer */
            *buffer_top = (uint32_t) elapsed;
            buffer_top++;

            if (buffer_top == buffer_end) {
                uint32_t *pattern;
                uint32_t match = pattern_match_buffer(&pattern);
                if (match) {
                    if(sample_idx == 0) {
                        samples[sample_idx++] = pattern;
                        sample_length = match;

                        printf("Received sample (%d) %d/%d...\n", match, sample_idx, RECORD_SAMPLES_COUNT);
                    }
                    else if(sample_length != match) {
                        sample_fails++;
                        printf("Received invalid sample (%d) %d/%d...\n", match, sample_idx, RECORD_SAMPLES_COUNT);
                    }
                    else {
                        samples[sample_idx++] = pattern;
                        printf("Received sample (%d) %d/%d...\n", match, sample_idx, RECORD_SAMPLES_COUNT);
                    }

                    if(sample_fails > RECORD_SAMPLES_FAILS) {
                        for(i = 0; i < sample_idx; i++) {
                            free(samples[i]);
                        }
                        sample_fails = 0;
                        sample_idx = 0;

                        printf("Failed! Restarting...\n");
                    }
                }

                buffer_top = buffer;
                if (last) { waiting = 1; }
            }
        }

        /* Reset on timeout */
        if (!on && elapsed > RECORD_TIMEOUT_TIME && buffer_top - buffer >= RECORD_TIMEOUT_MIN_ENTRIES * 2) {
            buffer_top = buffer;
            if (last) { waiting = 1; }
        }
    }

    printf("Done! Pattern:\n");
    // TODO: Match all samples and use the best. Probably can use a part of matching script above.
    for (i = 0; i < sample_length; i+= 2) {
        printf(">>> ON for %d usec, OFF for %d usec\n", samples[0][i], samples[0][i + 1]);
    }

    char fname[128];

    time_t nowt = time(NULL);
    struct tm *t = localtime(&nowt);
    strftime(fname, sizeof(fname), "%Y%m%d%H%M%S", t);
    sprintf(fname, "%s.rfdat", fname);

    FILE *pFile = fopen(fname,"wb");

    if (pFile){
        fwrite(samples[0], sizeof(uint32_t), sample_length, pFile);
    }

    fclose(pFile);

    printf("Saved to %s!\n", fname);

    for(i = 0; i < sample_idx; i++) {
        free(samples[i]);
    }
    free(buffer);
    return 0;
}