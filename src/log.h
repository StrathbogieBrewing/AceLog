#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

void log_begin(const char *logPath, int dataSize);
void log_commit(void* data);
void log_read(struct timeval *tv, void* data);
void log_end(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
