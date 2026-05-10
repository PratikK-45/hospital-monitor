#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include "../include/ipc_utils.h"
#include "../include/vitals.h"

/* Global definitions */
int         pipe_fd[2];
SharedData *shm_data = NULL;
sem_t      *shm_sem  = NULL;

int ipc_init_pipe(void)
{
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        return -1;
    }
    return 0;
}

int ipc_init_shm(void)
{
    /* Remove any leftover segment from a previous run */
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_NAME);

    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        return -1;
    }

    if (ftruncate(fd, sizeof(SharedData)) == -1) {
        perror("ftruncate");
        close(fd);
        return -1;
    }

    shm_data = mmap(NULL, sizeof(SharedData),
                    PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    if (shm_data == MAP_FAILED) {
        perror("mmap");
        return -1;
    }

    memset(shm_data, 0, sizeof(SharedData));

    shm_sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (shm_sem == SEM_FAILED) {
        perror("sem_open");
        return -1;
    }

    return 0;
}

void ipc_cleanup(void)
{
    if (shm_data && shm_data != MAP_FAILED)
        munmap(shm_data, sizeof(SharedData));

    shm_unlink(SHM_NAME);

    if (shm_sem && shm_sem != SEM_FAILED)
        sem_close(shm_sem);

    sem_unlink(SEM_NAME);

    close(pipe_fd[0]);
    close(pipe_fd[1]);
}
