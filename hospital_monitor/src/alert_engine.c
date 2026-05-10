/*
 * alert_engine.c — Child 2: Alert Engine
 *
 * Opens the shared memory segment and semaphore created by the parent.
 * Every second it reads the latest vitals for all patients,
 * checks against thresholds, and prints colour-coded alerts to stdout.
 *
 * Exits on SIGTERM from the parent.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <signal.h>
#include <string.h>
#include "../include/vitals.h"

static volatile int running = 1;

static void handle_sigterm(int sig)
{
    (void)sig;
    running = 0;
}

/* ANSI colour helpers */
#define RED     "\033[1;31m"
#define YELLOW  "\033[1;33m"
#define RESET   "\033[0m"

static void check_and_alert(const VitalRecord *rec)
{
    int alerted = 0;

    if (rec->heart_rate < HR_LOW || rec->heart_rate > HR_HIGH) {
        printf(RED "  [ALERT] Patient %d: Heart Rate %d bpm (normal %d–%d)\n" RESET,
               rec->patient_id, rec->heart_rate, HR_LOW, HR_HIGH);
        alerted = 1;
    }
    if (rec->spo2 < SPO2_LOW) {
        printf(RED "  [ALERT] Patient %d: SpO2 %d%% (normal >=%d%%)\n" RESET,
               rec->patient_id, rec->spo2, SPO2_LOW);
        alerted = 1;
    }
    if (rec->temperature < TEMP_LOW || rec->temperature > TEMP_HIGH) {
        printf(YELLOW "  [WARN]  Patient %d: Temp %.1f°C (normal %.1f–%.1f)\n" RESET,
               rec->patient_id, rec->temperature, TEMP_LOW, TEMP_HIGH);
        alerted = 1;
    }
    if (rec->systolic_bp < BP_LOW || rec->systolic_bp > BP_HIGH) {
        printf(YELLOW "  [WARN]  Patient %d: BP %d mmHg (normal %d–%d)\n" RESET,
               rec->patient_id, rec->systolic_bp, BP_LOW, BP_HIGH);
        alerted = 1;
    }
    (void)alerted;
}

int main(void)
{
    signal(SIGTERM, handle_sigterm);
    signal(SIGINT,  handle_sigterm);

    /* Open existing shared memory (created by parent) */
    int fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (fd == -1) {
        perror("alert_engine: shm_open");
        return 1;
    }

    SharedData *shm = mmap(NULL, sizeof(SharedData),
                           PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    if (shm == MAP_FAILED) {
        perror("alert_engine: mmap");
        return 1;
    }

    sem_t *sem = sem_open(SEM_NAME, 0);
    if (sem == SEM_FAILED) {
        perror("alert_engine: sem_open");
        return 1;
    }

    int last_record_count = 0;

    while (running) {
        sleep(1);

        sem_wait(sem);
        int current_count = shm->record_count;

        if (current_count != last_record_count) {
            last_record_count = current_count;
            for (int i = 0; i < NUM_PATIENTS; i++) {
                check_and_alert(&shm->patients[i]);
            }
            fflush(stdout);
        }
        sem_post(sem);
    }

    sem_close(sem);
    munmap(shm, sizeof(SharedData));
    return 0;
}
