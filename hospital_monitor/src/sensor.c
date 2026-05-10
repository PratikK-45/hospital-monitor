/*
 * sensor.c — Child 1: Sensor Simulator
 *
 * Receives the pipe write-end fd as argv[1] and a random seed as argv[2].
 * Generates random vital signs for NUM_PATIENTS patients every second
 * and writes VitalRecord structs into the pipe.
 *
 * Exits cleanly when the pipe write end is broken (parent closed read end).
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include "../include/vitals.h"

static volatile int running = 1;

static void handle_sigterm(int sig)
{
    (void)sig;
    running = 0;
}

/* Generate a random vital record for patient id */
static VitalRecord generate_vitals(int patient_id, unsigned int *seed)
{
    VitalRecord rec;
    rec.patient_id  = patient_id;
    rec.heart_rate  = 50  + rand_r(seed) % 81;    /* 50–130 */
    rec.spo2        = 88  + rand_r(seed) % 13;    /* 88–100 */
    rec.temperature = 35.0f + (rand_r(seed) % 56) * 0.1f; /* 35.0–40.5 */
    rec.systolic_bp = 80  + rand_r(seed) % 101;   /* 80–180 */
    rec.timestamp   = time(NULL);
    return rec;
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "sensor: usage: sensor <pipe_write_fd> <seed>\n");
        return 1;
    }

    int pipe_write_fd = atoi(argv[1]);
    unsigned int seed = (unsigned int)atoi(argv[2]);

    signal(SIGTERM, handle_sigterm);
    signal(SIGINT,  handle_sigterm);

    while (running) {
        for (int i = 0; i < NUM_PATIENTS; i++) {
            VitalRecord rec = generate_vitals(i + 1, &seed);
            ssize_t written = write(pipe_write_fd, &rec, sizeof(VitalRecord));
            if (written <= 0) {
                /* Pipe broken — parent exited */
                running = 0;
                break;
            }
        }
        sleep(1);
    }

    close(pipe_write_fd);
    return 0;
}
