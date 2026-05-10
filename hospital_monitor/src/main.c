/*
 * main.c — Parent Process: ICU Vital Signs Monitor
 *
 * Responsibilities:
 *   1. Set up pipe (IPC 1) and shared memory + semaphore (IPC 2)
 *   2. fork + execv  Child 1 (sensor simulator)
 *   3. fork + execv  Child 2 (alert engine)
 *   4. Start three pthreads: reader, display, logger
 *   5. Install signal handlers (SIGINT, SIGUSR1, SIGALRM)
 *   6. Wait for shutdown, then clean up everything (no zombies, no leaks)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/wait.h>
#include <time.h>
#include "../include/vitals.h"
#include "../include/ipc_utils.h"
#include "../include/logger.h"

/* Declared in threads.c */
void *reader_thread(void *arg);
void *display_thread(void *arg);
void *logger_thread(void *arg);
void  threads_wake_display(void);

/* Global running flag — threads and signal handlers read this */
volatile int g_running = 1;

/* Child PIDs so signal handlers can send SIGTERM */
static pid_t pid_sensor = -1;
static pid_t pid_alert  = -1;

/* ---------- Signal handlers ---------- */

static void handle_sigint(int sig)
{
    (void)sig;
    write(STDOUT_FILENO, "\n[monitor] Shutting down...\n", 28);
    g_running = 0;
}

static void handle_sigusr1(int sig)
{
    (void)sig;
    /* Print a snapshot header — actual values printed by display thread */
    write(STDOUT_FILENO, "\n[monitor] *** MANUAL SNAPSHOT (SIGUSR1) ***\n", 45);
}

static void handle_sigalrm(int sig)
{
    (void)sig;
    write(STDOUT_FILENO, "[monitor] Periodic summary written to log.\n", 43);
    /* Re-arm for next period */
    alarm(10);
}

/* ---------- fork + exec helpers ---------- */

static pid_t spawn_sensor(void)
{
    char pipe_write_str[16];
    char seed_str[16];

    snprintf(pipe_write_str, sizeof(pipe_write_str), "%d", pipe_fd[1]);
    snprintf(seed_str,       sizeof(seed_str),       "%u", (unsigned)time(NULL));

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork sensor");
        return -1;
    }
    if (pid == 0) {
        /* Child: close the read end — we only write */
        close(pipe_fd[0]);
        execl("./sensor", "sensor", pipe_write_str, seed_str, NULL);
        perror("execl sensor");
        _exit(1);
    }
    return pid;
}

static pid_t spawn_alert_engine(void)
{
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork alert_engine");
        return -1;
    }
    if (pid == 0) {
        /* Child: close pipe fds — alert engine uses shared memory only */
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        execl("./alert_engine", "alert_engine", NULL);
        perror("execl alert_engine");
        _exit(1);
    }
    return pid;
}

/* ---------- main ---------- */

int main(void)
{
    printf("╔══════════════════════════════════════════╗\n");
    printf("║      ICU Patient Vital Signs Monitor     ║\n");
    printf("║  Press Ctrl+C to stop. SIGUSR1=snapshot  ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    /* --- IPC setup --- */
    if (ipc_init_pipe() != 0)  return 1;
    if (ipc_init_shm()  != 0)  return 1;

    /* --- Log file setup --- */
    if (logger_open(LOG_FILE) != 0) {
        fprintf(stderr, "Could not open log file '%s'\n", LOG_FILE);
        return 1;
    }

    /* --- Signal handlers --- */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_RESTART;

    sa.sa_handler = handle_sigint;
    sigaction(SIGINT, &sa, NULL);

    sa.sa_handler = handle_sigusr1;
    sigaction(SIGUSR1, &sa, NULL);

    sa.sa_handler = handle_sigalrm;
    sigaction(SIGALRM, &sa, NULL);

    alarm(10);   /* first periodic summary in 10 s */

    /* --- Spawn children --- */
    pid_sensor = spawn_sensor();
    if (pid_sensor < 0) return 1;

    /* Small delay so sensor can generate first data before alert engine opens shm */
    usleep(100000);

    pid_alert = spawn_alert_engine();
    if (pid_alert < 0) {
        kill(pid_sensor, SIGTERM);
        return 1;
    }

    /* Parent no longer needs the write end of the pipe */
    close(pipe_fd[1]);
    pipe_fd[1] = -1;

    /* --- Start threads --- */
    pthread_t t_reader, t_display, t_logger;
    pthread_create(&t_reader,  NULL, reader_thread,  NULL);
    pthread_create(&t_display, NULL, display_thread, NULL);
    pthread_create(&t_logger,  NULL, logger_thread,  NULL);

    printf("[monitor] PID=%d | sensor PID=%d | alert PID=%d\n\n",
           getpid(), pid_sensor, pid_alert);

    /* --- Main loop: sleep until shutdown flag is set --- */
    while (g_running)
        pause();   /* woken by any signal */

    /* --- Graceful shutdown --- */
    alarm(0);   /* cancel pending SIGALRM */

    /* Stop children */
    if (pid_sensor > 0) kill(pid_sensor, SIGTERM);
    if (pid_alert  > 0) kill(pid_alert,  SIGTERM);

    /* Close write end so reader_thread's read() returns 0 */
    if (pipe_fd[1] != -1) close(pipe_fd[1]);

    /* Wake display thread in case it is blocked on cond_wait */
    threads_wake_display();

    /* Join threads */
    pthread_join(t_reader,  NULL);
    pthread_join(t_display, NULL);
    pthread_join(t_logger,  NULL);

    /* Reap children — no zombies */
    int status;
    if (pid_sensor > 0) waitpid(pid_sensor, &status, 0);
    if (pid_alert  > 0) waitpid(pid_alert,  &status, 0);

    /* Final log header update and cleanup */
    logger_update_header(shm_data ? shm_data->record_count : 0);
    logger_close();
    ipc_cleanup();

    printf("[monitor] Clean shutdown complete. Log saved to %s\n", LOG_FILE);
    return 0;
}
