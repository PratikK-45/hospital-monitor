# ICU Patient Vital Signs Monitor

A simulation-based Linux systems project demonstrating process control,
IPC, threads, signals, and file I/O — no hardware required.

## Scenario

Three ICU patients are monitored continuously. A sensor simulator
generates random vitals (heart rate, SpO2, temperature, blood pressure)
every second. A parent process reads and displays them live. An alert
engine watches for out-of-range values and prints warnings. Everything
is logged to `logs/vitals.log`.

## Architecture

```
Parent Process (monitor)
│
├── Child 1: sensor          — generates vitals → writes to pipe
├── Child 2: alert_engine    — reads shared memory → prints alerts
│
├── Thread: reader           — reads pipe → writes shared memory
├── Thread: display          — prints live vitals table (mutex + condvar)
└── Thread: logger           — writes to vitals.log every 2s (lseek)
```

IPC mechanisms:
- **pipe()** — sensor child → reader thread (unidirectional byte stream)
- **shared memory + semaphore** — reader thread → alert_engine child

## Build & Run

```bash
make all       # compile all three binaries
make run       # start the monitor (Ctrl+C to stop)
make test      # 15-second automated run + print log
make clean     # remove binaries and logs
```

Requires: gcc, POSIX shared memory (`-lrt`), pthreads (`-lpthread`).

## Signals

| Signal   | How to send            | Effect                        |
|----------|------------------------|-------------------------------|
| SIGINT   | Ctrl+C                 | Graceful shutdown             |
| SIGUSR1  | `kill -USR1 <PID>`     | Print manual snapshot header  |
| SIGALRM  | auto every 10 s        | Log periodic summary          |

## System Calls Used

| Category  | Calls                                              |
|-----------|----------------------------------------------------|
| File I/O  | open, write, read, lseek, close                    |
| Process   | fork, execl, waitpid, exit, getpid                 |
| Threads   | pthread_create/join, mutex_lock/unlock, cond_wait/signal |
| IPC       | pipe, shm_open, mmap, munmap, sem_open/wait/post   |
| Signals   | sigaction, alarm, kill, pause                      |

## Sample Output

```
╔══════════════════════════════════════════╗
║      ICU Patient Vital Signs Monitor     ║
║  Press Ctrl+C to stop. SIGUSR1=snapshot  ║
╚══════════════════════════════════════════╝

[monitor] PID=12345 | sensor PID=12346 | alert PID=12347

━━━ ICU Vital Signs (live) ━━━
  Pat.   Heart Rate  SpO2      Temp(°C)   BP(mmHg)
  ------ ----------  --------  ---------- ----------
  P1     78          98        36.8       118
  P2     112         91        38.2       155
  P3     55          97        37.1       102

  [ALERT] Patient 2: Heart Rate 112 bpm (normal 60–100)
  [ALERT] Patient 2: SpO2 91% (normal >=95%)
  [WARN]  Patient 2: Temp 38.2°C (normal 36.5–37.5)
  [WARN]  Patient 2: BP 155 mmHg (normal 90–140)
  [ALERT] Patient 3: Heart Rate 55 bpm (normal 60–100)
```

## Log File Format

The first 64 bytes of `logs/vitals.log` are a header updated in-place
via `lseek(fd, 0, SEEK_SET)` each flush cycle:

```
ICU_LOG records=42
[14:05:01] P1 | HR= 78 bpm | SpO2= 98% | Temp=36.8°C | BP=118 mmHg
...
```

## Project Structure

```
hospital_monitor/
├── Makefile
├── README.md
├── include/
│   ├── vitals.h         ← shared structs, constants, thresholds
│   ├── ipc_utils.h      ← IPC globals and function declarations
│   └── logger.h         ← log file API
├── src/
│   ├── main.c           ← parent process, signal handling, lifecycle
│   ├── sensor.c         ← Child 1: random vitals generator
│   ├── alert_engine.c   ← Child 2: threshold checker
│   ├── threads.c        ← reader / display / logger threads
│   ├── ipc_utils.c      ← pipe, shm_open, semaphore helpers
│   └── logger.c         ← open / write / lseek / close
└── logs/
    └── vitals.log       ← generated at runtime
```
