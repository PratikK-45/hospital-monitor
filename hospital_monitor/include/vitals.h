#ifndef VITALS_H
#define VITALS_H

#include <time.h>

#define NUM_PATIENTS     3
#define SHM_NAME         "/icu_vitals_shm"
#define SEM_NAME         "/icu_vitals_sem"
#define LOG_FILE         "logs/vitals.log"
#define LOG_HEADER_SIZE  64   /* bytes reserved at start of log for metadata */

/* Vital signs for one patient */
typedef struct {
    int     patient_id;
    int     heart_rate;      /* bpm */
    int     spo2;            /* % */
    float   temperature;     /* Celsius */
    int     systolic_bp;     /* mmHg */
    time_t  timestamp;
} VitalRecord;

/* Shared memory layout: one record per patient */
typedef struct {
    VitalRecord patients[NUM_PATIENTS];
    int         record_count;   /* total records written so far */
} SharedData;

/* Alert thresholds */
#define HR_LOW    60
#define HR_HIGH   100
#define SPO2_LOW  95
#define TEMP_LOW  36.5f
#define TEMP_HIGH 37.5f
#define BP_LOW    90
#define BP_HIGH   140

#endif /* VITALS_H */
