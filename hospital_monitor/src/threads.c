/*
 * threads.c — Reader, Display, and Logger threads
 *
 * reader_thread  : reads VitalRecord structs from the pipe (IPC 1),
 *                  writes them into shared memory (IPC 2), signals display.
 * display_thread : waits on condition variable, prints a live vitals table.
 * logger_thread  : wakes every 2 seconds, flushes new records to log file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include "../include/vitals.h"
#include "../include/ipc_utils.h"
#include "../include/logger.h"

/* ---------- Shared state between threads ---------- */
static pthread_mutex_t display_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  display_cond  = PTHREAD_COND_INITIALIZER;

/* Latest snapshot for display (protected by display_mutex) */
static VitalRecord display_buf[NUM_PATIENTS];
static int         display_ready = 0;

/* Total records logged (protected by display_mutex) */
static int total_logged = 0;

/* Set to 1 by signal handler in main.c to stop all threads */
extern volatile int g_running;

/* ---------- ANSI helpers ---------- */
#define GREEN   "\033[0;32m"
#define RESET   "\033[0m"

static void print_vitals_table(const VitalRecord *recs)
{
    printf("\n" GREEN "━━━ ICU Vital Signs (%s) ━━━" RESET "\n",
           "live");
    printf("  %-6s  %-10s  %-8s  %-10s  %-10s\n",
           "Pat.", "Heart Rate", "SpO2", "Temp(°C)", "BP(mmHg)");
    printf("  %-6s  %-10s  %-8s  %-10s  %-10s\n",
           "------", "----------", "--------", "----------", "----------");

    for (int i = 0; i < NUM_PATIENTS; i++) {
        printf("  P%-5d  %-10d  %-8d  %-10.1f  %-10d\n",
               recs[i].patient_id,
               recs[i].heart_rate,
               recs[i].spo2,
               recs[i].temperature,
               recs[i].systolic_bp);
    }
    printf("\n");
    fflush(stdout);
}

/* ---------- Reader thread ---------- */
void *reader_thread(void *arg)
{
    (void)arg;
    VitalRecord rec;

    while (g_running) {
        ssize_t n = read(pipe_fd[0], &rec, sizeof(VitalRecord));
        if (n <= 0) break;   /* pipe closed or error */

        /* Write into shared memory (IPC 2) under semaphore */
        sem_wait(shm_sem);
        int idx = rec.patient_id - 1;
        if (idx >= 0 && idx < NUM_PATIENTS) {
            shm_data->patients[idx] = rec;
        }
        shm_data->record_count++;
        sem_post(shm_sem);

        /* Copy to display buffer and signal display thread */
        pthread_mutex_lock(&display_mutex);
        if (rec.patient_id == NUM_PATIENTS) {
            /* All patients updated in this round — refresh display */
            memcpy(display_buf, shm_data->patients, sizeof(display_buf));
            display_ready = 1;
            pthread_cond_signal(&display_cond);
        }
        pthread_mutex_unlock(&display_mutex);
    }

    return NULL;
}

/* ---------- Display thread ---------- */
void *display_thread(void *arg)
{
    (void)arg;
    VitalRecord local_buf[NUM_PATIENTS];

    while (g_running) {
        pthread_mutex_lock(&display_mutex);

        /* Wait until reader signals new data (or running flag drops) */
        while (!display_ready && g_running)
            pthread_cond_wait(&display_cond, &display_mutex);

        if (!g_running) {
            pthread_mutex_unlock(&display_mutex);
            break;
        }

        memcpy(local_buf, display_buf, sizeof(local_buf));
        display_ready = 0;
        pthread_mutex_unlock(&display_mutex);

        print_vitals_table(local_buf);
    }

    return NULL;
}

/* ---------- Logger thread ---------- */
void *logger_thread(void *arg)
{
    (void)arg;

    while (g_running) {
        sleep(2);
        if (!g_running) break;

        /* Snapshot shared memory under semaphore */
        VitalRecord snap[NUM_PATIENTS];
        sem_wait(shm_sem);
        memcpy(snap, shm_data->patients, sizeof(snap));
        int count = shm_data->record_count;
        sem_post(shm_sem);

        /* Write each patient record to log file */
        for (int i = 0; i < NUM_PATIENTS; i++) {
            if (snap[i].patient_id != 0)
                logger_write(&snap[i]);
        }

        /* Update the header with current total record count (uses lseek) */
        pthread_mutex_lock(&display_mutex);
        total_logged = count;
        pthread_mutex_unlock(&display_mutex);

        logger_update_header(total_logged);
    }

    return NULL;
}

/* Called by main after g_running = 0 to wake blocked display thread */
void threads_wake_display(void)
{
    pthread_mutex_lock(&display_mutex);
    pthread_cond_broadcast(&display_cond);
    pthread_mutex_unlock(&display_mutex);
}
