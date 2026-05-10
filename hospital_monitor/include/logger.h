#ifndef LOGGER_H
#define LOGGER_H

#include "vitals.h"

int  logger_open(const char *path);
void logger_write(const VitalRecord *rec);
void logger_update_header(int total_records);
void logger_close(void);

#endif /* LOGGER_H */
