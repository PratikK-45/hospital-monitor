#ifndef IPC_UTILS_H
#define IPC_UTILS_H

#include <semaphore.h>
#include "vitals.h"

/* Pipe file descriptors (set up in main, used by child + reader thread) */
extern int pipe_fd[2];

/* Shared memory pointer and semaphore (set up in main) */
extern SharedData *shm_data;
extern sem_t      *shm_sem;

/* Setup and teardown */
int  ipc_init_pipe(void);
int  ipc_init_shm(void);
void ipc_cleanup(void);

#endif /* IPC_UTILS_H */
