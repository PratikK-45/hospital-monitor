#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "../include/logger.h"
#include "../include/vitals.h"

static int log_fd = -1;

int logger_open(const char *path)
{
    log_fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (log_fd == -1) {
        perror("logger open");
        return -1;
    }

    /* Write a 64-byte header placeholder at the start of the file.
       We will update the record count in-place using lseek later. */
    char header[LOG_HEADER_SIZE];
    memset(header, 0, sizeof(header));
    snprintf(header, sizeof(header), "ICU_LOG records=%-8d\n", 0);
    write(log_fd, header, LOG_HEADER_SIZE);

    return 0;
}

void logger_write(const VitalRecord *rec)
{
    if (log_fd == -1) return;

    char buf[256];
    char timebuf[32];
    struct tm *tm_info = localtime(&rec->timestamp);
    strftime(timebuf, sizeof(timebuf), "%H:%M:%S", tm_info);

    int len = snprintf(buf, sizeof(buf),
        "[%s] P%d | HR=%3d bpm | SpO2=%3d%% | Temp=%.1f°C | BP=%3d mmHg\n",
        timebuf,
        rec->patient_id,
        rec->heart_rate,
        rec->spo2,
        rec->temperature,
        rec->systolic_bp);

    /* Seek to end to append */
    lseek(log_fd, 0, SEEK_END);
    write(log_fd, buf, len);
}

void logger_update_header(int total_records)
{
    if (log_fd == -1) return;

    char header[LOG_HEADER_SIZE];
    memset(header, 0, sizeof(header));
    snprintf(header, sizeof(header), "ICU_LOG records=%-8d\n", total_records);

    /* Seek back to the beginning to overwrite the header in-place */
    lseek(log_fd, 0, SEEK_SET);
    write(log_fd, header, LOG_HEADER_SIZE);
}

void logger_close(void)
{
    if (log_fd != -1) {
        close(log_fd);
        log_fd = -1;
    }
}
