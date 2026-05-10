# ICU Patient Vital Signs Monitor

A Linux systems programming project written in C that simulates an ICU patient monitoring system using:

- Processes (`fork`, `execl`)
- Threads (`pthread`)
- IPC (`pipe`, shared memory, semaphores)
- Signals
- File I/O

The system continuously generates patient vitals, displays them live, logs them to a file, and raises alerts when readings go outside safe ranges.

---

# Features

- Real-time patient vital simulation
- Multi-process architecture
- Multi-threaded monitoring
- Shared memory communication
- Alert system for dangerous vitals
- Continuous log file updates
- Clean shutdown handling with signals

---

# Architecture

```text
sensor process
      │
      ▼
   pipe()
      │
      ▼
reader thread ──► shared memory ──► alert_engine process
      │
      ├──► display thread
      └──► logger thread
```

---

# Components

## Processes

| Process | Role |
|---|---|
| `monitor` | Main controller process |
| `sensor` | Generates random patient vitals |
| `alert_engine` | Detects abnormal readings |

---

## Threads (inside monitor)

| Thread | Role |
|---|---|
| Reader | Reads data from pipe and updates shared memory |
| Display | Prints live patient vitals |
| Logger | Writes vitals to `logs/vitals.log` |

---

# IPC Used

| IPC Mechanism | Purpose |
|---|---|
| `pipe()` | Sensor → monitor communication |
| Shared Memory | Share latest vitals |
| POSIX Semaphore | Synchronize shared memory access |

---

# Signals

| Signal | Action |
|---|---|
| `SIGINT` | Clean shutdown |
| `SIGUSR1` | Print snapshot |
| `SIGALRM` | Periodic log summary |

---

# Project Structure

```text
hospital_monitor/
├── Makefile
├── README.md
├── include/
│   ├── vitals.h
│   ├── ipc_utils.h
│   └── logger.h
└── src/
    ├── main.c
    ├── sensor.c
    ├── alert_engine.c
    ├── threads.c
    ├── ipc_utils.c
    └── logger.c
```

---

# Build & Run

## Install dependencies

```bash
sudo apt install gcc make build-essential
```

## Build

```bash
make all
```

## Run

```bash
make run
```

## Test

```bash
make test
```

## Clean

```bash
make clean
```

---

# Sample Output

```text
━━━ ICU Vital Signs (live) ━━━

Pat.   HR   SpO2   Temp   BP
P1     78   98     36.8   118
P2     112  91     38.2   155

[ALERT] Patient 2: Heart Rate High
[WARN]  Patient 2: High Temperature
```

---

# System Calls Used

## File I/O
`open`, `read`, `write`, `lseek`, `close`

## Processes
`fork`, `execl`, `waitpid`

## Threads
`pthread_create`, `pthread_join`

## IPC
`pipe`, `shm_open`, `mmap`, `sem_open`

## Signals
`sigaction`, `kill`, `alarm`

---

# Normal Vital Ranges

| Vital | Normal Range |
|---|---|
| Heart Rate | 60–100 bpm |
| SpO2 | ≥95% |
| Temperature | 36.5–37.5°C |
| Blood Pressure | 90–140 mmHg |

---

# Learning Outcomes

This project helped practice:

- Linux system calls
- POSIX threads
- IPC mechanisms
- Signal handling
- Synchronization
- Concurrent programming

---

